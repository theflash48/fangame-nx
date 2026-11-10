# SPDX-License-Identifier: GPL-2.0-or-later
# Icono original de FangameNX. Solo usa geometría y colores del proyecto.
Add-Type -AssemblyName System.Drawing
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root 'mkxp-z-nx/nx/launcher-icon.jpg'
$bitmap = [System.Drawing.Bitmap]::new(256, 256)
$g = [System.Drawing.Graphics]::FromImage($bitmap)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$area = [System.Drawing.Rectangle]::new(0, 0, 256, 256)
$base = [System.Drawing.Drawing2D.LinearGradientBrush]::new($area, [System.Drawing.Color]::FromArgb(12,20,35), [System.Drawing.Color]::FromArgb(30,48,76), 45)
$g.FillRectangle($base, $area)
$ring = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(48,102,133), 5)
$g.DrawEllipse($ring, 22, 22, 212, 212)
$cyan = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(66,221,207))
$violet = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(143,127,248), 15)
$violet.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$violet.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$g.FillRectangle($cyan, 59, 54, 30, 151)
$g.FillRectangle($cyan, 59, 54, 108, 27)
$g.FillRectangle($cyan, 59, 113, 82, 25)
$g.DrawLine($violet, 159, 143, 203, 198)
$g.DrawLine($violet, 203, 143, 159, 198)
$codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object MimeType -eq 'image/jpeg' | Select-Object -First 1
$parameters = [System.Drawing.Imaging.EncoderParameters]::new(1)
$parameters.Param[0] = [System.Drawing.Imaging.EncoderParameter]::new([System.Drawing.Imaging.Encoder]::Quality, [long]92)
$bitmap.Save($out, $codec, $parameters)
$parameters.Dispose(); $violet.Dispose(); $cyan.Dispose(); $ring.Dispose(); $base.Dispose(); $g.Dispose(); $bitmap.Dispose()
Write-Output $out
