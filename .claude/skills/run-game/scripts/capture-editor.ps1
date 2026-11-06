# Saves a PNG of the Unreal editor window only (never the whole screen - other windows may be private).
# Works while the editor is behind other windows (PrintWindow with PW_RENDERFULLCONTENT).
# Usage: powershell -File capture-editor.ps1 -Out <path.png> [-Scale 0.5]
param(
    [Parameter(Mandatory = $true)][string]$Out,
    [double]$Scale = 1.0
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public static class KubeCraftCapture {
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$editor = Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $editor) { Write-Error "Unreal editor is not running"; exit 1 }

$rect = New-Object KubeCraftCapture+RECT
[KubeCraftCapture]::GetWindowRect($editor.MainWindowHandle, [ref]$rect) | Out-Null
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top

$bitmap = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$dc = $graphics.GetHdc()
[KubeCraftCapture]::PrintWindow($editor.MainWindowHandle, $dc, 2) | Out-Null   # 2 = PW_RENDERFULLCONTENT
$graphics.ReleaseHdc($dc)

if ($Scale -ne 1.0) {
    $bitmap = New-Object System.Drawing.Bitmap $bitmap, ([int]($width * $Scale)), ([int]($height * $Scale))
}
$bitmap.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
"Saved $Out ($width x $height, scale $Scale)"
