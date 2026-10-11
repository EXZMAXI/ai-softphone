# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

[CmdletBinding()]
param(
    [switch] $Force
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$PjprojectVersion = '2.17'
$PjprojectSha256 = '065fe06c06788d97c35f563796d59f00ce52fe9558a52d7b490a042a966facce'

$Url = "https://github.com/pjsip/pjproject/archive/refs/tags/$PjprojectVersion.tar.gz"

if ($env:SOFTPHONE_CACHE_DIR) {
    $CacheRoot = $env:SOFTPHONE_CACHE_DIR
} else {
    $CacheRoot = Join-Path $env:LOCALAPPDATA 'softphone-ai-deps'
}

if ($env:SOFTPHONE_PJPROJECT_PREFIX) {
    $Prefix = $env:SOFTPHONE_PJPROJECT_PREFIX
} else {
    $Prefix = Join-Path $CacheRoot "pjproject-$PjprojectVersion"
}

$DownloadDir = Join-Path $CacheRoot 'downloads'
$BuildDir = Join-Path $CacheRoot "build\pjproject-$PjprojectVersion"
$Tarball = Join-Path $DownloadDir "pjproject-$PjprojectVersion.tar.gz"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ConfigSite = Join-Path $ScriptDir 'pjproject-config_site.h'
$Patcher = Join-Path $ScriptDir 'patch-pjproject.py'

$Stamp = Join-Path $Prefix 'lib\Debug\libpjproject.lib'
$FingerprintFile = Join-Path $Prefix '.build-script-sha256'

function Write-Log([string] $Message) {
    Write-Host "[pjproject] $Message" -ForegroundColor White
}

$fingerprintSource = [System.IO.File]::ReadAllBytes($PSCommandPath) +
                     [System.IO.File]::ReadAllBytes($ConfigSite) +
                     [System.IO.File]::ReadAllBytes($Patcher)
$sha = [System.Security.Cryptography.SHA256]::Create()
try {
    $Fingerprint = ($sha.ComputeHash($fingerprintSource) | ForEach-Object { $_.ToString('x2') }) -join ''
} finally {
    $sha.Dispose()
}

if ((Test-Path $Stamp) -and -not $Force) {
    if ((Test-Path $FingerprintFile) -and
        ((Get-Content $FingerprintFile -Raw).Trim() -eq $Fingerprint)) {
        Write-Log "already installed at $Prefix"
        Write-Log 'pass -Force to rebuild'
        Write-Output $Prefix
        exit 0
    }
    Write-Log 'this script changed since the install was built; rebuilding'
}

New-Item -ItemType Directory -Force $DownloadDir | Out-Null

if (-not (Test-Path $Tarball)) {
    Write-Log "downloading $PjprojectVersion"
    Invoke-WebRequest $Url -OutFile $Tarball -UseBasicParsing
}

Write-Log 'verifying checksum'
$actual = (Get-FileHash $Tarball -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $PjprojectSha256) {
    Write-Error @"
Checksum mismatch for $Tarball.
  expected $PjprojectSha256
  got      $actual
Delete it and re-run, or update `$PjprojectSha256 if the pin changed.
"@
    exit 1
}

Write-Log "extracting into $BuildDir"
if (Test-Path $BuildDir) { Remove-Item $BuildDir -Recurse -Force }
New-Item -ItemType Directory -Force $BuildDir | Out-Null

$SystemTar = Join-Path $env:SystemRoot 'System32\tar.exe'
& $SystemTar -xzf $Tarball -C $BuildDir --strip-components=1
if ($LASTEXITCODE -ne 0) { Write-Error "tar failed with $LASTEXITCODE"; exit 1 }

Copy-Item $ConfigSite (Join-Path $BuildDir 'pjlib\include\pj\config_site.h') -Force

Write-Log 'correcting'
& python3 $Patcher $BuildDir
if ($LASTEXITCODE -ne 0) { Write-Error "patch-pjproject failed with $LASTEXITCODE"; exit 1 }

$VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $VsWhere)) {
    Write-Error 'vswhere.exe not found: no Visual Studio installer on this machine.'
    exit 1
}

$VsPath = & $VsWhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $VsPath) {
    Write-Error 'No Visual Studio with the C++ build tools. Install the "Desktop development with C++" workload.'
    exit 1
}

$VcVars = Join-Path $VsPath 'VC\Auxiliary\Build\vcvars64.bat'

$SdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include'
$Sdk = Get-ChildItem $SdkRoot -Directory |
    Where-Object { $_.Name -match '^10\.' } |
    Sort-Object { [version]$_.Name } |
    Select-Object -Last 1
if (-not $Sdk) { Write-Error "No Windows 10 SDK under $SdkRoot"; exit 1 }

Write-Log "building with $(Split-Path -Leaf $VsPath), toolset v143, SDK $($Sdk.Name)"

foreach ($configuration in @('Debug-Dynamic', 'Release-Dynamic')) {
    Write-Log "building $configuration"

    $log = Join-Path $BuildDir "msbuild-$configuration.log"

    $installerDir = Split-Path -Parent $VsWhere
    $command = "set `"PATH=$installerDir;%PATH%`" && " +
               "call `"$VcVars`" >nul && msbuild pjproject-vs14.sln " +
               "/t:libpjproject " +
               "/p:Configuration=$configuration /p:Platform=x64 " +
               "/p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=$($Sdk.Name) " +
               "/m /v:minimal"

    Push-Location $BuildDir
    try {
        & cmd.exe /c $command > $log 2>&1
        $failed = $LASTEXITCODE -ne 0
    } finally {
        Pop-Location
    }

    if ($failed) {
        Get-Content $log -Tail 40 | Write-Host
        Write-Error "msbuild failed for $configuration; the whole log is at $log"
        exit 1
    }
}

Write-Log "installing into $Prefix"
if (Test-Path $Prefix) { Remove-Item $Prefix -Recurse -Force }

$IncludeDir = Join-Path $Prefix 'include'
New-Item -ItemType Directory -Force $IncludeDir | Out-Null

foreach ($module in @('pjlib', 'pjlib-util', 'pjnath', 'pjmedia', 'pjsip')) {
    $from = Join-Path $BuildDir "$module\include"
    Copy-Item "$from\*" $IncludeDir -Recurse -Force
}

$configurations = @{ 'Debug' = 'Debug-Dynamic'; 'Release' = 'Release-Dynamic' }
foreach ($name in $configurations.Keys) {
    $libDir = Join-Path $Prefix "lib\$name"
    New-Item -ItemType Directory -Force $libDir | Out-Null

    $built = Join-Path $BuildDir "lib\libpjproject-x86_64-x64-vc14-$($configurations[$name]).lib"
    if (-not (Test-Path $built)) {
        Write-Error "expected $built to exist after the build"
        exit 1
    }

    Copy-Item $built (Join-Path $libDir 'libpjproject.lib') -Force
}

Set-Content -Path $FingerprintFile -Value $Fingerprint -Encoding ascii -NoNewline

Write-Log "installed pjproject $PjprojectVersion"
Write-Output $Prefix
