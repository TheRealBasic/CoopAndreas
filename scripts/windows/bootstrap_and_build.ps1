[CmdletBinding()]
param(
    [string]$GtaSaDir,
    [string]$PluginSdkDir,
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RequiredPluginSdkCommit = '050d18b6e1770477deab81a40028a40277583d97'

function Write-Info([string]$Message) {
    Write-Host "[INFO] $Message" -ForegroundColor Cyan
}

function Write-Success([string]$Message) {
    Write-Host "[OK]   $Message" -ForegroundColor Green
}

function Write-Warn([string]$Message) {
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

function Stop-WithError([string]$Message, [string]$HowToFix) {
    Write-Host "`n[ERROR] $Message" -ForegroundColor Red
    if ($HowToFix) {
        Write-Host "How to fix: $HowToFix" -ForegroundColor Red
    }
    exit 1
}

function Test-CommandAvailable([string]$CommandName) {
    return $null -ne (Get-Command $CommandName -ErrorAction SilentlyContinue)
}

function Install-WithWingetIfMissing([string]$CommandName, [string]$PackageId, [string]$DisplayName, [string]$ExtraArgs = '') {
    if (Test-CommandAvailable $CommandName) {
        Write-Success "$DisplayName is already installed."
        return
    }

    Write-Info "Installing $DisplayName via winget ($PackageId)..."
    $args = @(
        'install',
        '--id', $PackageId,
        '--accept-package-agreements',
        '--accept-source-agreements',
        '--silent'
    )

    if (-not [string]::IsNullOrWhiteSpace($ExtraArgs)) {
        $args += $ExtraArgs
    }

    & winget @args
    if ($LASTEXITCODE -ne 0) {
        Stop-WithError "winget failed to install $DisplayName." "Install it manually and rerun this script."
    }

    Write-Success "$DisplayName installation completed."
}

function Ensure-BuildToolsInstalled {
    $vsWherePath = "$env:ProgramFiles(x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vsWherePath)) {
        Write-Info 'Installing Visual Studio 2022 Build Tools with C++ workload...'
        $override = '--override "--wait --quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"'
        & winget install --id Microsoft.VisualStudio.2022.BuildTools --accept-package-agreements --accept-source-agreements --silent $override
        if ($LASTEXITCODE -ne 0) {
            Stop-WithError 'Failed to install Visual Studio Build Tools.' 'Install VS 2022 Build Tools with the Desktop C++ workload manually.'
        }
    }

    if (-not (Test-Path $vsWherePath)) {
        Stop-WithError 'Visual Studio installer tooling was not found after installation.' 'Restart Windows and retry, or install VS Build Tools manually.'
    }

    $installPath = & $vsWherePath -products * -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Workload.VCTools -property installationPath 2>$null | Select-Object -First 1
    if (-not $installPath) {
        Stop-WithError 'Visual Studio 2022 with C++ workload was not detected.' 'Open Visual Studio Installer and add "Desktop development with C++".'
    }

    Write-Success "Visual Studio Build Tools detected at '$installPath'."
}

function Resolve-PathOrNull([string]$PathValue) {
    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        return $null
    }

    try {
        return (Resolve-Path $PathValue).Path
    }
    catch {
        return $null
    }
}

try {
    if (-not (Test-CommandAvailable 'winget')) {
        Stop-WithError 'winget was not found.' 'Install App Installer from Microsoft Store (provides winget), then rerun.'
    }

    Install-WithWingetIfMissing -CommandName 'git' -PackageId 'Git.Git' -DisplayName 'Git'
    Install-WithWingetIfMissing -CommandName 'cmake' -PackageId 'Kitware.CMake' -DisplayName 'CMake'
    Install-WithWingetIfMissing -CommandName 'xmake' -PackageId 'xmake-io.xmake' -DisplayName 'xmake'
    Ensure-BuildToolsInstalled

    $repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')

    $resolvedPluginSdkDir = Resolve-PathOrNull $PluginSdkDir
    if (-not $resolvedPluginSdkDir) {
        $defaultPluginSdk = Join-Path $repoRoot 'third_party\plugin-sdk'
        if (-not (Test-Path $defaultPluginSdk)) {
            Write-Info "Cloning plugin-sdk into '$defaultPluginSdk'..."
            & git clone https://github.com/DK22Pac/plugin-sdk.git $defaultPluginSdk
            if ($LASTEXITCODE -ne 0) {
                Stop-WithError 'Failed to clone plugin-sdk.' 'Check your network connection and rerun the script.'
            }
        }

        Write-Info "Checking out required plugin-sdk commit $RequiredPluginSdkCommit..."
        & git -C $defaultPluginSdk fetch --all --tags
        & git -C $defaultPluginSdk checkout $RequiredPluginSdkCommit
        if ($LASTEXITCODE -ne 0) {
            Stop-WithError 'Failed to checkout required plugin-sdk commit.' 'Open plugin-sdk manually and checkout the required commit.'
        }

        $resolvedPluginSdkDir = (Resolve-Path $defaultPluginSdk).Path
    }

    $setupScript = Join-Path $repoRoot 'scripts\windows\setup_and_build.ps1'
    if (-not (Test-Path $setupScript)) {
        Stop-WithError 'setup_and_build.ps1 is missing.' 'Restore scripts/windows/setup_and_build.ps1 and rerun.'
    }

    if ($SkipBuild) {
        Write-Success 'Dependency bootstrap completed. Build step skipped by -SkipBuild.'
        exit 0
    }

    Write-Info 'Running guided setup_and_build script...'
    & powershell -ExecutionPolicy Bypass -File $setupScript -GtaSaDir $GtaSaDir -PluginSdkDir $resolvedPluginSdkDir
    if ($LASTEXITCODE -ne 0) {
        Stop-WithError 'setup_and_build.ps1 failed.' 'Review the error above and rerun once corrected.'
    }

    Write-Host "`nBootstrap + build completed successfully." -ForegroundColor Green
}
catch {
    Stop-WithError 'bootstrap_and_build.ps1 terminated unexpectedly.' $_.Exception.Message
}
