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

$OpenSslVersion = '3.5.7'
$OpenSslSha256 = 'a8c0d28a529ca480f9f36cf5792e2cd21984552a3c8e4aa11a24aa31aeac98e8'
$OpenSslUrl = "https://github.com/openssl/openssl/releases/download/openssl-$OpenSslVersion/openssl-$OpenSslVersion.tar.gz"

$PerlVersion = '5.40.4.1'
$PerlSha256 = 'aa9052ac082c8a8a0f952823b3f4f0bf9c1d0f84bbe0b3dfe0aa94291d6b1045'
$PerlUrl = 'https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_54041_64bit/strawberry-perl-5.40.4.1-64bit-portable.zip'

if ($env:SOFTPHONE_CACHE_DIR) {
    $CacheRoot = $env:SOFTPHONE_CACHE_DIR
} else {
    $CacheRoot = Join-Path $env:LOCALAPPDATA 'softphone-ai-deps'
}

if ($env:SOFTPHONE_OPENSSL_PREFIX) {
    $Prefix = $env:SOFTPHONE_OPENSSL_PREFIX
} else {
    $Prefix = Join-Path $CacheRoot "openssl-$OpenSslVersion"
}

$DownloadDir = Join-Path $CacheRoot 'downloads'
$BuildDir = Join-Path $CacheRoot "build\openssl-$OpenSslVersion"
$PerlDir = Join-Path $CacheRoot "strawberry-perl-$PerlVersion"
$Tarball = Join-Path $DownloadDir "openssl-$OpenSslVersion.tar.gz"

$Stamp = Join-Path $Prefix 'lib\libcrypto.lib'
$FingerprintFile = Join-Path $Prefix '.build-script-sha256'

function Write-Log([string] $Message) {
    Write-Host "[openssl] $Message" -ForegroundColor White
}

$sha = [System.Security.Cryptography.SHA256]::Create()
try {
    $Fingerprint = ($sha.ComputeHash([System.IO.File]::ReadAllBytes($PSCommandPath)) |
        ForEach-Object { $_.ToString('x2') }) -join ''
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

function Resolve-Perl {
    $candidates = @()
    $onPath = Get-Command perl -ErrorAction SilentlyContinue
    if ($onPath) { $candidates += $onPath.Source }
    $candidates += 'C:\Strawberry\perl\bin\perl.exe'
    $candidates += (Join-Path $PerlDir 'perl\bin\perl.exe')

    foreach ($candidate in $candidates) {
        if (-not (Test-Path $candidate)) { continue }
        & $candidate -MLocale::Maketext::Simple -e 1 2>$null
        if ($LASTEXITCODE -eq 0) { return $candidate }
        Write-Log "$candidate cannot run Configure (missing core modules); looking further"
    }
    return $null
}

$Perl = Resolve-Perl

if (-not $Perl) {
    Write-Log 'no usable Perl on this machine; fetching a portable Strawberry'
    Write-Log 'this is a 300 MB download and happens once'

    New-Item -ItemType Directory -Force $DownloadDir | Out-Null
    $perlZip = Join-Path $DownloadDir "strawberry-perl-$PerlVersion-64bit-portable.zip"

    if (-not (Test-Path $perlZip)) {
        Invoke-WebRequest $PerlUrl -OutFile $perlZip -UseBasicParsing
    }

    $actual = (Get-FileHash $perlZip -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $PerlSha256) {
        Write-Error @"
Checksum mismatch for $perlZip.
  expected $PerlSha256
  got      $actual
Delete it and re-run, or update `$PerlSha256 if the pin changed.
"@
        exit 1
    }

    Write-Log "extracting into $PerlDir"
    if (Test-Path $PerlDir) { Remove-Item $PerlDir -Recurse -Force }
    Expand-Archive -Path $perlZip -DestinationPath $PerlDir -Force

    $Perl = Resolve-Perl
    if (-not $Perl) {
        Write-Error "Extracted Strawberry Perl to $PerlDir and still cannot run Configure with it."
        exit 1
    }
}

Write-Log "perl: $Perl"

$perlRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $Perl))
$PerlPathEntries = @(
    (Join-Path $perlRoot 'perl\site\bin'),
    (Join-Path $perlRoot 'perl\bin'),
    (Join-Path $perlRoot 'c\bin')
) | Where-Object { Test-Path $_ }

New-Item -ItemType Directory -Force $DownloadDir | Out-Null

if (-not (Test-Path $Tarball)) {
    Write-Log "downloading $OpenSslVersion"
    Invoke-WebRequest $OpenSslUrl -OutFile $Tarball -UseBasicParsing
}

Write-Log 'verifying checksum'
$actual = (Get-FileHash $Tarball -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $OpenSslSha256) {
    Write-Error @"
Checksum mismatch for $Tarball.
  expected $OpenSslSha256
  got      $actual
Delete it and re-run, or update `$OpenSslSha256 if the pin changed.
"@
    exit 1
}

Write-Log "extracting into $BuildDir"
if (Test-Path $BuildDir) { Remove-Item $BuildDir -Recurse -Force }
New-Item -ItemType Directory -Force $BuildDir | Out-Null

$SystemTar = Join-Path $env:SystemRoot 'System32\tar.exe'
& $SystemTar -xzf $Tarball -C $BuildDir --strip-components=1
if ($LASTEXITCODE -ne 0) { Write-Error "tar failed with $LASTEXITCODE"; exit 1 }

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
$installerDir = Split-Path -Parent $VsWhere

if (Test-Path $Prefix) { Remove-Item $Prefix -Recurse -Force }

$log = Join-Path $BuildDir 'build.log'
$extraPath = ($PerlPathEntries + $installerDir) -join ';'

$configure = "`"$Perl`" Configure VC-WIN64A no-asm no-uplink no-tests no-docs " +
             "--prefix=`"$Prefix`" --openssldir=`"$Prefix\ssl`""

$command = "( set `"PATH=$extraPath;%PATH%`" && " +
           "call `"$VcVars`" >nul && " +
           "$configure && nmake && nmake install_sw ) > `"$log`" 2>&1"

Write-Log "building with $(Split-Path -Leaf $VsPath); this takes a few minutes"

Push-Location $BuildDir
try {
    & cmd.exe /c $command
    $failed = $LASTEXITCODE -ne 0
} finally {
    Pop-Location
}

if ($failed) {
    Get-Content $log -Tail 40 | Write-Host
    Write-Error "the OpenSSL build failed; the whole log is at $log"
    exit 1
}

foreach ($wanted in @('include\openssl\ssl.h', 'lib\libcrypto.lib', 'lib\libssl.lib',
                      'bin\libcrypto-3-x64.dll', 'bin\libssl-3-x64.dll')) {
    $path = Join-Path $Prefix $wanted
    if (-not (Test-Path $path)) {
        Write-Error "the build finished but $path is missing; the log is at $log"
        exit 1
    }
}

Set-Content -Path $FingerprintFile -Value $Fingerprint -Encoding ascii

Write-Log "installed at $Prefix"
Write-Output $Prefix
