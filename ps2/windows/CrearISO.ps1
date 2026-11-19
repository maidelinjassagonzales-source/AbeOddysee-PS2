# Abe's Exoddus / Abe's Oddysee PS2 - ISO builder for Windows (no extra software needed).
# Finds your Steam copy of the game and builds a PS2 ISO with it and the port's ELF.
# The game is chosen by the ELF next to this script: ODDYSEE.ELF -> Abe's Oddysee, EXODDUS.ELF ->
# Abe's Exoddus (or force it with -Juego AO / -Juego AE).
# Usage: double-click CrearISO.bat, or:
#   powershell -ExecutionPolicy Bypass -File CrearISO.ps1 [-Juego AE|AO] [-Game <folder>] [-Extra EN=<folder>,FR=<folder>] [-Out <file.iso>]
param(
    [string]$Juego = "",
    [string]$Game = "",
    [string[]]$Extra = @(),
    [string]$Out = "",
    [string]$Elf = ""
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$games = @{
    AE = @{ Title = "Oddworld: Abe's Exoddus"; Elf = 'EXODDUS.ELF'; Id = 'SLUS_999.98'; Volume = 'ABE_EXODDUS'
            AppId = 15710; Folder = 'Oddworld Abes Exoddus'; Probe = 'st.lvl'; Iso = 'SLUS_999.98.ABE_EXODDUS_PS2.iso' }
    AO = @{ Title = "Oddworld: Abe's Oddysee"; Elf = 'ODDYSEE.ELF'; Id = 'SLUS_999.97'; Volume = 'ABE_ODDYSEE'
            AppId = 15700; Folder = 'Oddworld Abes Oddysee'; Probe = 's1.lvl'; Iso = 'SLUS_999.97.ABE_ODDYSEE_PS2.iso' }
}
if (-not $Juego) {
    if ($Elf) { $Juego = if ((Split-Path $Elf -Leaf) -ieq 'ODDYSEE.ELF') { 'AO' } else { 'AE' } }
    elseif (Test-Path (Join-Path $here 'ODDYSEE.ELF')) { $Juego = 'AO' }
    else { $Juego = 'AE' }
}
$g = $games[$Juego.ToUpper()]
if (-not $g) { throw "-Juego must be AE (Exoddus) or AO (Oddysee)." }
if (-not $Elf) { $Elf = Join-Path $here $g.Elf }
if (-not $Out) { $Out = Join-Path $here $g.Iso }
if (-not (Test-Path $Elf)) { throw "$($g.Elf) not found next to this script ($Elf)." }
Write-Host "$($g.Title) PS2 -> $($g.Id)"

$langCodes = @{ english = 'EN'; spanish = 'ES'; french = 'FR'; german = 'DE'; italian = 'IT' }

function Find-SteamGame {
    $roots = @()
    foreach ($k in 'HKCU:\Software\Valve\Steam', 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam') {
        try {
            $p = (Get-ItemProperty $k -ErrorAction Stop)
            if ($p.SteamPath) { $roots += $p.SteamPath }
            if ($p.InstallPath) { $roots += $p.InstallPath }
        } catch {}
    }
    $libs = @()
    foreach ($r in ($roots | Select-Object -Unique)) {
        $libs += $r
        $vdf = Join-Path $r 'steamapps\libraryfolders.vdf'
        if (Test-Path $vdf) {
            foreach ($m in [regex]::Matches((Get-Content $vdf -Raw), '"path"\s+"([^"]+)"')) { $libs += ($m.Groups[1].Value -replace '\\\\', '\') }
        }
    }
    foreach ($l in ($libs | Select-Object -Unique)) {
        $manifest = Join-Path $l "steamapps\appmanifest_$($g.AppId).acf"
        $dir = Join-Path $l "steamapps\common\$($g.Folder)"
        if ((Test-Path $manifest) -and (Test-Path $dir)) {
            $lang = 'english'
            $mm = [regex]::Match((Get-Content $manifest -Raw), '"language"\s+"([^"]+)"')
            if ($mm.Success) { $lang = $mm.Groups[1].Value }
            return @{ Dir = $dir; Lang = $lang }
        }
    }
    return $null
}

$sets = [ordered]@{}
if ($Game) {
    $sets['EN'] = $Game
} else {
    $found = Find-SteamGame
    if (-not $found) { throw "$($g.Title) (Steam) not found. Run with -Game <game folder>." }
    $code = $langCodes[$found.Lang]
    if (-not $code) { $code = 'EN' }
    $sets[$code] = $found.Dir
    Write-Host "Steam copy: $($found.Dir) (language: $($found.Lang) -> DATA\$code)"
}
foreach ($e in ($Extra | ForEach-Object { $_ -split ',' } | Where-Object { $_ })) {
    $kv = $e -split '=', 2
    $sets[$kv[0].ToUpper()] = $kv[1]
}

Add-Type -TypeDefinition (Get-Content (Join-Path $here 'Ps2Iso.cs') -Raw) -Language CSharp

$builder = New-Object Ps2Iso
$builder.VolumeId = $g.Volume
$builder.AddFile('', 'SYSTEM.CNF', [Text.Encoding]::ASCII.GetBytes("BOOT2 = cdrom0:\$($g.Id);1`r`nVER = 1.00`r`nVMODE = NTSC`r`n"))
$builder.AddFilePath('', $g.Id, (Resolve-Path $Elf).Path)
foreach ($code in $sets.Keys) {
    $src = $sets[$code]
    $files = Get-ChildItem -LiteralPath $src -File | Where-Object { $_.Extension -match '^\.(lvl|ddv)$' -or $_.Name -ieq 'sounds.dat' }
    if (-not ($files | Where-Object { $_.Name -ieq $g.Probe })) { throw "$src does not look like the $($g.Title) folder ($($g.Probe) missing)." }
    foreach ($f in $files) { $builder.AddFilePath("DATA/$code", $f.Name.ToUpper(), $f.FullName) }
    Write-Host "DATA\$code : $($files.Count) files"
}
Write-Host "Writing $Out ..."
$builder.Write($Out)
Write-Host ("Done: {0:N0} MB" -f ((Get-Item $Out).Length / 1MB))
