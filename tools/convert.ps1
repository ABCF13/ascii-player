# convert.ps1 - source\<video>.mp4  ->  clips\clip_<ID>.txt
# Usage:  .\tools\convert.ps1 video.mp4 001
param(
  [Parameter(Mandatory=$true)][string]$Video,
  [Parameter(Mandatory=$true)][string]$Id
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root
$Converter = Join-Path $PSScriptRoot 'ascii_conv.exe'
$ConverterSource = Join-Path $PSScriptRoot 'ascii_conv.c'

# pick up programs installed after this terminal was opened (ffmpeg, gcc, python)
$env:Path = [Environment]::GetEnvironmentVariable('Path','Machine') + ';' +
            [Environment]::GetEnvironmentVariable('Path','User')
if (-not (Get-Command ffmpeg -ErrorAction SilentlyContinue)) {
  throw 'ffmpeg not found. Install it with:  winget install Gyan.FFmpeg   then reopen VS Code.'
}

$W = 118; $H = 33          # characters per frame (about 1.4x the old 100x28 area)
$Id = $Id.PadLeft(3, '0')

$src = if (Test-Path $Video) { $Video } else { Join-Path 'source' $Video }
if (-not (Test-Path $src)) { throw "Can't find $Video (also looked in the source folder)" }
New-Item -ItemType Directory -Force clips | Out-Null

# (re)build the converter if it's missing or older than the .c file
if (-not (Test-Path $Converter) -or
    (Get-Item $ConverterSource).LastWriteTime -gt (Get-Item $Converter).LastWriteTime) {
  Write-Host 'Compiling ascii_conv...'
  gcc -O2 -o $Converter $ConverterSource -lm
  if ($LASTEXITCODE -ne 0) { throw 'Compile failed' }
}

$raw = "clips\temp_$Id.raw"
ffmpeg -y -loglevel error -i $src -vf "fps=12,scale=${W}:${H}" -f rawvideo -pix_fmt gray $raw
if ($LASTEXITCODE -ne 0) { throw 'ffmpeg failed' }
& $Converter $raw $Id $W $H "clips\clip_$Id.txt"
Remove-Item $raw
