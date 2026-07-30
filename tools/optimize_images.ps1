param(
    [string]$HeadSource = "",
    [string]$TailSource = "",
    [ValidateRange(32, 512)]
    [int]$CoinSize = 135,
    [ValidateRange(1, 100)]
    [long]$CoinQuality = 68,
    [ValidateRange(1, 100)]
    [long]$DefaultQuality = 72
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot

Add-Type -AssemblyName System.Drawing

$codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() |
  Where-Object { $_.MimeType -eq "image/jpeg" }

function Save-Jpeg($image, $path, [long]$quality) {
  $parameters = New-Object System.Drawing.Imaging.EncoderParameters(1)
  $parameters.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter(
    [System.Drawing.Imaging.Encoder]::Quality,
    $quality
  )
  $image.Save($path, $codec, $parameters)
  $parameters.Dispose()
}

function Convert-Square($source, $destination, [int]$size, [long]$quality) {
  if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Image source not found: $source"
  }

  $image = [System.Drawing.Image]::FromFile($source)
  $bitmap = New-Object System.Drawing.Bitmap($size, $size)
  $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
  $graphics.Clear([System.Drawing.Color]::FromArgb(18, 18, 18))
  $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
  $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
  $graphics.DrawImage($image, 0, 0, $size, $size)
  Save-Jpeg $bitmap $destination $quality
  $graphics.Dispose()
  $bitmap.Dispose()
  $image.Dispose()
}

if ([bool]$HeadSource -xor [bool]$TailSource) {
  throw "Pass both -HeadSource and -TailSource, or omit both."
}

if ($HeadSource -and $TailSource) {
  Convert-Square $HeadSource "data\img\coin_head.jpg" $CoinSize $CoinQuality
  Convert-Square $TailSource "data\img\coin_tail.jpg" $CoinSize $CoinQuality
}

$imageFiles = Get-ChildItem -LiteralPath "data\img" -Filter "*.jpg"
$before = ($imageFiles | Measure-Object Length -Sum).Sum

foreach ($file in $imageFiles) {
  $temporaryPath = "$($file.FullName).tmp"
  $image = [System.Drawing.Image]::FromFile($file.FullName)
  $bitmap = New-Object System.Drawing.Bitmap($image.Width, $image.Height)
  $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
  $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
  $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
  $graphics.DrawImage($image, 0, 0, $image.Width, $image.Height)
  $quality = if ($file.Name -like "coin_*") { $CoinQuality } else { $DefaultQuality }
  Save-Jpeg $bitmap $temporaryPath $quality
  $graphics.Dispose()
  $bitmap.Dispose()
  $image.Dispose()

  if ((Get-Item -LiteralPath $temporaryPath).Length -lt $file.Length -or $file.Name -like "coin_*") {
    Move-Item -LiteralPath $temporaryPath -Destination $file.FullName -Force
  } else {
    Remove-Item -LiteralPath $temporaryPath
  }
}

$after = (Get-ChildItem -LiteralPath "data\img" -Filter "*.jpg" | Measure-Object Length -Sum).Sum
Write-Output "image-bytes-before=$before after=$after saved=$($before - $after)"
