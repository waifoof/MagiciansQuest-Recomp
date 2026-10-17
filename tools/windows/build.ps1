# Build MagiciansQuest-Recomp on Windows.
#
# Run from PowerShell (not as administrator):
#     powershell -ExecutionPolicy Bypass -File tools\windows\build.ps1
# or, before you have the code, save this file anywhere and run it: it clones the repository next to itself.
#
# What it does:
#   1. Installs what is missing with winget: Git, CMake, Ninja, LLVM (clang), Python and the Visual Studio 2022 Build
#      Tools (the C++ workload, for the Windows headers and libraries). winget asks before each install.
#   2. Clones the repository (with the melonDS submodule) unless it is already run from inside it.
#   3. Builds the program with clang-cl. The program contains no game code.
#   4. Prints how to start it. On first launch it asks for your Magician's Quest ROM and builds the game from it
#      (about a minute), with the LLVM it finds in C:\Program Files\LLVM\bin.
param(
    [string]$Repo = "https://github.com/waifoof/MagiciansQuest-Recomp.git",
    [string]$Branch = "main",
    [string]$Rom = ""   # optional: also build the game into the build folder from this ROM (developer build)
)
$ErrorActionPreference = "Stop"

function Have($cmd) { return [bool](Get-Command $cmd -ErrorAction SilentlyContinue) }
function Refresh-Path {
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")
    foreach ($p in @("C:\Program Files\LLVM\bin", "C:\Program Files\CMake\bin", "C:\Program Files\Git\cmd")) {
        if ((Test-Path $p) -and -not ($env:Path -split ";" -contains $p)) { $env:Path = "$p;$env:Path" }
    }
}
function Install($id, $override = $null) {
    Write-Host "Installing $id with winget..." -ForegroundColor Cyan
    if ($override) { winget install --id $id -e --accept-package-agreements --accept-source-agreements --override $override }
    else { winget install --id $id -e --accept-package-agreements --accept-source-agreements }
    Refresh-Path
}

if (-not (Have winget)) { throw "winget is missing: install 'App Installer' from the Microsoft Store, then run this again." }
Refresh-Path

# 1. Tools
if (-not (Have git)) { Install "Git.Git" }
if (-not (Have cmake)) { Install "Kitware.CMake" }
if (-not (Have ninja)) { Install "Ninja-build.Ninja" }
if (-not (Have clang-cl)) { Install "LLVM.LLVM" }
if (-not (Have python) -or ((python --version 2>&1) -notmatch "Python 3")) { Install "Python.Python.3.12" }

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
function Find-VS {
    if (-not (Test-Path $vswhere)) { return $null }
    return & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}
$vs = Find-VS
if (-not $vs) {
    Install "Microsoft.VisualStudio.2022.BuildTools" "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive --wait"
    $vs = Find-VS
    if (-not $vs) { throw "The Visual Studio C++ Build Tools did not install. Install them from https://visualstudio.microsoft.com/visual-cpp-build-tools/ (workload: Desktop development with C++), then run this again." }
}

# Load the MSVC environment (headers, libraries) into this PowerShell session.
$vcvars = Join-Path $vs "VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" >nul && set" | ForEach-Object {
    if ($_ -match "^([^=]+)=(.*)$") { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process") }
}
Refresh-Path

# 2. Source
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = $null
try { $root = (git -C $here rev-parse --show-toplevel 2>$null) } catch {}
if (-not $root -or -not (Test-Path (Join-Path $root "game\config\ynne"))) {
    $root = Join-Path $here "MagiciansQuest-Recomp"
    if (-not (Test-Path $root)) {
        Write-Host "Cloning $Repo (a private repository asks you to sign in to GitHub)..." -ForegroundColor Cyan
        git clone --recursive --branch $Branch $Repo $root
    }
}
Set-Location $root
git submodule update --init --recursive

# 3. Build
$cfg = @("--preset", "release")
if ($Rom) { $cfg += "-DFFCC_ROM=$Rom" }
cmake @cfg
cmake --build --preset release

$exe = Join-Path $root "build\release\game\ffcc.exe"
Write-Host ""
Write-Host "Built: $exe" -ForegroundColor Green
Write-Host "Start it, choose your Magician's Quest (USA) ROM on the start screen, and it builds the game once (about a minute)."
Write-Host "Saves and settings go to %APPDATA%\MagiciansQuest-Recomp\MysteriousTimes."
