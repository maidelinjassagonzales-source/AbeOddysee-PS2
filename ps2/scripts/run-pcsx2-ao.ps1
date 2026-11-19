# Lanza ODDYSEE.ELF (versión de pruebas, host:) en PCSX2 MINIMIZADO con los datos de Abe's Oddysee
# de Steam, espera, cierra SOLO el PCSX2 que ha lanzado (por PID) y muestra el log de la consola EE.
# Si ya hay un PCSX2 abierto (otra persona/agente lo está usando) no hace nada y sale con código 2.
param([int]$Seconds = 40, [string]$InputFile = '', [switch]$SkipFmv, [string]$Shots = '', [string]$Iso = '')

$ErrorActionPreference = 'Stop'
$root    = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent | Split-Path -Parent
$elf     = Join-Path $root 'out\ao\ODDYSEE.ELF'
$game    = 'C:\Program Files (x86)\Steam\steamapps\common\Oddworld Abes Oddysee'
$pcsx2   = 'C:\Program Files\PCSX2\pcsx2-qt.exe'
$docs    = [Environment]::GetFolderPath('MyDocuments')
$logFile = Join-Path $docs 'PCSX2\logs\emulog.txt'
if (-not $Shots) { $Shots = Join-Path $root 'out\ao\shots' }

if (Get-Process pcsx2-qt -ErrorAction SilentlyContinue) { 'PCSX2 ya está abierto: no se toca, reintenta luego'; exit 2 }

$target = Join-Path $game 'ODDYSEE_PS2.ELF'
Copy-Item $elf $target -Force
Remove-Item (Join-Path $game 'ps2input.txt'), (Join-Path $game 'ps2skipfmv.txt') -ErrorAction SilentlyContinue
if ($InputFile) { Copy-Item $InputFile (Join-Path $game 'ps2input.txt') -Force }
if ($SkipFmv) { Set-Content (Join-Path $game 'ps2skipfmv.txt') '1' }
Remove-Item (Join-Path $game 'ps2shot_*.bmp') -ErrorAction SilentlyContinue

if ($Iso) {
  # Disc test: boot the ISO like a real disc (no host:, no scripted input or frame dumps).
  $p = Start-Process -FilePath $pcsx2 -ArgumentList @('-fastboot', '--', "`"$Iso`"") -PassThru -WindowStyle Minimized
} else {
  $p = Start-Process -FilePath $pcsx2 -ArgumentList @('-fastboot', '-elf', "`"$target`"") -PassThru -WindowStyle Minimized
}
"PCSX2 PID $($p.Id)"
Start-Sleep -Seconds $Seconds
Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 800

New-Item -ItemType Directory -Force $Shots | Out-Null
Remove-Item (Join-Path $Shots 'ps2shot_*.png') -ErrorAction SilentlyContinue
Add-Type -AssemblyName System.Drawing
Get-ChildItem $game -Filter 'ps2shot_*.bmp' -ErrorAction SilentlyContinue | ForEach-Object {
  try {
    $img = [System.Drawing.Image]::FromFile($_.FullName)
    $img.Save((Join-Path $Shots ($_.BaseName + '.png')), [System.Drawing.Imaging.ImageFormat]::Png); $img.Dispose()
  } catch { }
  Remove-Item $_.FullName
}
$all = Get-Content $logFile
$i = ($all | Select-String -SimpleMatch '[relive-ps2] main reached' | Select-Object -Last 1).LineNumber
if (-not $i) { $all | Select-Object -Last 40; return }
$all[($i-1)..($all.Count-1)] | Where-Object { $_ -notmatch 'open (name|fd)|InitJoyStick|# Init|# Restart|UpdateVSync' } | Select-Object -Last 80
