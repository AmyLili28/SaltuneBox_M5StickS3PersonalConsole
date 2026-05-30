from __future__ import annotations

import shutil
import subprocess
import sys
import time
import base64
import json
import os
import socket
import tempfile
import urllib.parse
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
DIST = ROOT / "dist"

INDEX = SRC / "index.html"
SINGLE_HTML = DIST / "manual_A6_single_source.html"
PRINT_HTML = DIST / "manual_A6_print_source.html"
SINGLE_PDF = DIST / "manual_A6_single_pages.pdf"
PRINT_PDF = DIST / "manual_A6_print_ready.pdf"
PREVIEW_PNG = DIST / "manual_A6_preview.png"


EDGE_CANDIDATES = [
    Path(r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"),
    Path(r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"),
]


def find_edge() -> Path:
    for candidate in EDGE_CANDIDATES:
        if candidate.exists():
            return candidate
    found = shutil.which("msedge") or shutil.which("microsoft-edge")
    if found:
        return Path(found)
    raise SystemExit("Microsoft Edge was not found. Install Edge or export the HTML manually from a browser.")


def write_sources() -> None:
    DIST.mkdir(exist_ok=True)
    html = INDEX.read_text(encoding="utf-8").replace('href="./styles.css"', 'href="../src/styles.css"')

    single = html.replace('<body class="single">', '<body class="single export">')
    SINGLE_HTML.write_text(single, encoding="utf-8")

    print_override = """
  <style>
    @page { size: 111mm 154mm; margin: 0; }
    @media print {
      .sheet { width: 111mm; height: 154mm; }
    }
  </style>
"""
    print_html = html.replace('<body class="single">', '<body class="print export">')
    print_html = print_html.replace("</head>", f"{print_override}</head>")
    PRINT_HTML.write_text(print_html, encoding="utf-8")


def run_edge(args: list[str], output: Path) -> None:
    if output.exists():
        output.unlink()
    edge = find_edge()
    cmd = [
        str(edge),
        "--headless=new",
        "--disable-gpu",
        "--disable-extensions",
        "--no-first-run",
        "--run-all-compositor-stages-before-draw",
        "--virtual-time-budget=2500",
        *args,
    ]
    result = subprocess.run(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"Edge export failed for {output.name}\n{result.stderr}")

    for _ in range(40):
        if output.exists() and output.stat().st_size > 0:
            return
        time.sleep(0.25)
    raise RuntimeError(f"Expected output was not created: {output}")


class CdpClient:
    def __init__(self, ws_url: str) -> None:
        parsed = urllib.parse.urlparse(ws_url)
        self.host = parsed.hostname or "127.0.0.1"
        self.port = parsed.port or 80
        self.path = parsed.path
        if parsed.query:
            self.path += "?" + parsed.query
        self.sock = socket.create_connection((self.host, self.port), timeout=10)
        self.sock.settimeout(30)
        self.next_id = 1
        self._handshake()

    def _handshake(self) -> None:
        key = base64.b64encode(os.urandom(16)).decode("ascii")
        request = (
            f"GET {self.path} HTTP/1.1\r\n"
            f"Host: {self.host}:{self.port}\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Key: {key}\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n"
        ).encode("ascii")
        self.sock.sendall(request)
        response = b""
        while b"\r\n\r\n" not in response:
            response += self.sock.recv(4096)
        if b" 101 " not in response.split(b"\r\n", 1)[0]:
            raise RuntimeError("WebSocket handshake failed")

    def close(self) -> None:
        try:
            self.sock.close()
        except OSError:
            pass

    def _send_frame(self, payload: bytes, opcode: int = 1) -> None:
        header = bytearray([0x80 | opcode])
        length = len(payload)
        if length < 126:
            header.append(0x80 | length)
        elif length < 65536:
            header.append(0x80 | 126)
            header.extend(length.to_bytes(2, "big"))
        else:
            header.append(0x80 | 127)
            header.extend(length.to_bytes(8, "big"))
        mask = os.urandom(4)
        masked = bytes(byte ^ mask[i % 4] for i, byte in enumerate(payload))
        self.sock.sendall(bytes(header) + mask + masked)

    def _read_exact(self, size: int) -> bytes:
        chunks = []
        remaining = size
        while remaining:
            chunk = self.sock.recv(remaining)
            if not chunk:
                raise RuntimeError("WebSocket closed")
            chunks.append(chunk)
            remaining -= len(chunk)
        return b"".join(chunks)

    def _recv_frame_payload(self) -> tuple[int, bytes, bool]:
        first, second = self._read_exact(2)
        fin = bool(first & 0x80)
        opcode = first & 0x0F
        masked = bool(second & 0x80)
        length = second & 0x7F
        if length == 126:
            length = int.from_bytes(self._read_exact(2), "big")
        elif length == 127:
            length = int.from_bytes(self._read_exact(8), "big")
        mask = self._read_exact(4) if masked else b""
        payload = self._read_exact(length)
        if masked:
            payload = bytes(byte ^ mask[i % 4] for i, byte in enumerate(payload))
        return opcode, payload, fin

    def recv_message(self, timeout: float = 30) -> dict:
        self.sock.settimeout(timeout)
        fragments: list[bytes] = []
        while True:
            opcode, payload, fin = self._recv_frame_payload()
            if opcode == 8:
                raise RuntimeError("WebSocket closed by browser")
            if opcode == 9:
                self._send_frame(payload, opcode=10)
                continue
            if opcode in (1, 0):
                fragments.append(payload)
                if fin:
                    return json.loads(b"".join(fragments).decode("utf-8"))

    def call(self, method: str, params: dict | None = None, timeout: float = 30) -> dict:
        msg_id = self.next_id
        self.next_id += 1
        payload = {"id": msg_id, "method": method}
        if params is not None:
            payload["params"] = params
        self._send_frame(json.dumps(payload, separators=(",", ":")).encode("utf-8"))
        while True:
            message = self.recv_message(timeout=timeout)
            if message.get("id") == msg_id:
                if "error" in message:
                    raise RuntimeError(f"CDP error from {method}: {message['error']}")
                return message.get("result", {})

    def wait_event(self, method: str, timeout: float = 10) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                message = self.recv_message(timeout=max(0.25, deadline - time.time()))
            except socket.timeout:
                return
            if message.get("method") == method:
                return


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def wait_for_json(port: int, path: str, timeout: float = 10) -> object:
    deadline = time.time() + timeout
    last_error: Exception | None = None
    while time.time() < deadline:
        try:
            with urllib.request.urlopen(f"http://127.0.0.1:{port}{path}", timeout=1) as response:
                return json.loads(response.read().decode("utf-8"))
        except Exception as exc:  # noqa: BLE001 - browser can take a moment to open the port.
            last_error = exc
            time.sleep(0.2)
    raise RuntimeError(f"Timed out waiting for Edge DevTools at {path}: {last_error}")


def new_page_ws(port: int) -> str:
    url = f"http://127.0.0.1:{port}/json/new?about:blank"
    request = urllib.request.Request(url, method="PUT")
    try:
        with urllib.request.urlopen(request, timeout=5) as response:
            target = json.loads(response.read().decode("utf-8"))
            return target["webSocketDebuggerUrl"]
    except Exception:
        targets = wait_for_json(port, "/json/list", timeout=5)
        for target in targets:
            if target.get("type") == "page" and target.get("webSocketDebuggerUrl"):
                return target["webSocketDebuggerUrl"]
    raise RuntimeError("Could not create a DevTools page target")


def export_pdf(source: Path, output: Path) -> None:
    if output.exists():
        output.unlink()
    port = free_port()
    user_data_dir = Path(tempfile.mkdtemp(prefix="a6-manual-edge-"))
    edge = find_edge()
    proc = subprocess.Popen(
        [
            str(edge),
            "--headless=new",
            "--disable-gpu",
            "--disable-extensions",
            "--no-first-run",
            "--no-default-browser-check",
            f"--user-data-dir={user_data_dir}",
            f"--remote-debugging-port={port}",
            "about:blank",
        ],
        cwd=ROOT,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    client: CdpClient | None = None
    try:
        wait_for_json(port, "/json/version", timeout=10)
        client = CdpClient(new_page_ws(port))
        client.call("Page.enable")
        client.call("Page.navigate", {"url": source.as_uri()})
        client.wait_event("Page.loadEventFired", timeout=10)
        result = client.call(
            "Page.printToPDF",
            {
                "printBackground": True,
                "displayHeaderFooter": False,
                "preferCSSPageSize": True,
                "scale": 1,
                "marginTop": 0,
                "marginBottom": 0,
                "marginLeft": 0,
                "marginRight": 0,
                "pageRanges": "",
            },
            timeout=60,
        )
        output.write_bytes(base64.b64decode(result["data"]))
    finally:
        if client is not None:
            try:
                client.call("Browser.close", timeout=2)
            except Exception:
                client.close()
        try:
            proc.terminate()
            proc.wait(timeout=5)
        except Exception:
            pass
        shutil.rmtree(user_data_dir, ignore_errors=True)


def export_preview(source: Path, output: Path) -> None:
    run_edge(
        [
            "--window-size=3300,1500",
            f"--screenshot={output}",
            source.as_uri(),
        ],
        output,
    )


def main() -> int:
    if not INDEX.exists():
        raise SystemExit(f"Missing source file: {INDEX}")
    write_sources()
    export_pdf(SINGLE_HTML, SINGLE_PDF)
    export_pdf(PRINT_HTML, PRINT_PDF)
    export_preview(SINGLE_HTML, PREVIEW_PNG)
    print("Generated:")
    print(f"  {SINGLE_PDF}")
    print(f"  {PRINT_PDF}")
    print(f"  {PREVIEW_PNG}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
