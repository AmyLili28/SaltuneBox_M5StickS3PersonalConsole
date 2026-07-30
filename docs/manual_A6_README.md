# A6 说明书源文件与导出说明

这套文件用于生成《独属于欢哥的小小音乐终端》的 A6 竖版说明书小册子。

## 文件位置

- 源文件：`src/index.html`
- 样式文件：`src/styles.css`
- 导出脚本：`src/generate.py`
- 视觉参考图仅保存在本地，不随公开仓库分发。
- 最终输出：`dist/manual_A6_single_pages.pdf`
- 打印文件：`dist/manual_A6_print_ready.pdf`
- 预览图：`dist/manual_A6_preview.png`

## 尺寸设置

- 成品 A6：105mm x 148mm
- 出血：四边各 3mm
- 打印画布：111mm x 154mm
- 安全边距：至少 7mm
- 页数：12 页，适合骑马钉印刷

## 如何修改文字

直接打开 `src/index.html`，每一页都是一个 `.sheet`：

```html
<section class="sheet">
  ...
</section>
```

按页修改里面的标题、正文、注释即可。正式说明稿保存在 `docs/manual_draft.md`，如果后续大段改文字，建议先改这个草稿，再把适合排版的版本放进 `src/index.html`。

## 如何替换封面手写字体

封面标题会优先使用：

```text
fonts/handwriting.ttf
```

如果没有这个文件，CSS 会使用本机手写感 fallback：

```css
"STXingkai", "STKaiti", "KaiTi", "Comic Sans MS", cursive
```

如果你有更合适的中文手写字体，把字体文件复制到 `fonts/handwriting.ttf`，然后重新导出即可。不要改文件名，脚本和 CSS 已经按这个路径引用。

## 如何替换图片

目前内页使用的是 CSS 线框、灰度占位框和几何图形，没有直接使用视觉参考图内容。

后续要替换真实图片时，可以在 `src/index.html` 的占位区域加入：

```html
<img src="../assets/manual/your-image.jpg" alt="">
```

然后在 `src/styles.css` 里给图片设置宽高、灰度或混合模式。

## 如何重新导出

在项目根目录运行：

```powershell
python src\generate.py
```

脚本会调用本机 Microsoft Edge 的 headless 模式生成 PDF 和 PNG，不需要联网，不会下载外部素材。

## 哪些文件用于打印

- `dist/manual_A6_single_pages.pdf`：单页顺序 PDF，适合先预览。
- `dist/manual_A6_print_ready.pdf`：111mm x 154mm，包含 3mm 出血和裁切标记，适合交给打印店。
- `dist/manual_A6_preview.png`：封面和若干内页的屏幕预览。

如果打印店需要拼版或骑马钉页序，可以把 `manual_A6_print_ready.pdf` 发给他们继续拼版。
