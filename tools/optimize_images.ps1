$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq 'image/jpeg' }

function Save-Jpeg($image, $path, [long]$quality) {
  $params = New-Object System.Drawing.Imaging.EncoderParameters(1)
  $params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, $quality)
  $image.Save($path, $codec, $params)
  $params.Dispose()
}

function Convert-Square($src, $dst, [int]$size, [long]$quality) {
  $img = [System.Drawing.Image]::FromFile($src)
  $bmp = New-Object System.Drawing.Bitmap($size, $size)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.Clear([System.Drawing.Color]::FromArgb(18, 18, 18))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
  $g.DrawImage($img, 0, 0, $size, $size)
  Save-Jpeg $bmp $dst $quality
  $g.Dispose()
  $bmp.Dispose()
  $img.Dispose()
}

$headSource = 'C:\Users\ccgg6\Downloads\Image_1780309471361_8.png'
$tailSource = 'C:\Users\ccgg6\Downloads\Image_1780309477100_391.png'

Convert-Square $headSource 'data\img\coin_head.jpg' 135 68
Convert-Square $tailSource 'data\img\coin_tail.jpg' 135 68

$before = (Get-ChildItem data\img\*.jpg | Measure-Object Length -Sum).Sum
foreach ($file in Get-ChildItem data\img\*.jpg) {
  $tmp = "$($file.FullName).tmp"
  $img = [System.Drawing.Image]::FromFile($file.FullName)
  $bmp = New-Object System.Drawing.Bitmap($img.Width, $img.Height)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
  $g.DrawImage($img, 0, 0, $img.Width, $img.Height)
  $quality = if ($file.Name -like 'coin_*') { 68 } else { 72 }
  Save-Jpeg $bmp $tmp $quality
  $g.Dispose()
  $bmp.Dispose()
  $img.Dispose()
  if ((Get-Item $tmp).Length -lt $file.Length -or $file.Name -like 'coin_*') {
    Move-Item -Force $tmp $file.FullName
  } else {
    Remove-Item $tmp
  }
}

$after = (Get-ChildItem data\img\*.jpg | Measure-Object Length -Sum).Sum
"image-bytes-before=$before after=$after saved=$($before - $after)"
