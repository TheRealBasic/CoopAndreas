[CmdletBinding()]
param(
    [string]$GtaSaDir,
    [string]$PluginSdkDir
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

function Resolve-ExistingDirectory(
    [string]$Provided,
    [string]$EnvValue,
    [string]$Prompt
) {
    $candidate = $Provided
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        $candidate = $EnvValue
    }

    if ([string]::IsNullOrWhiteSpace($candidate)) {
        $candidate = Read-Host $Prompt
    }

    if ([string]::IsNullOrWhiteSpace($candidate)) {
        return $null
    }

    try {
        $resolved = Resolve-Path -Path $candidate
        return $resolved.Path
    }
    catch {
        return $null
    }
}

function Set-UserEnvironmentVariable([string]$Name, [string]$Value) {
    [Environment]::SetEnvironmentVariable($Name, $Value, 'User')
    Set-Item -Path "Env:$Name" -Value $Value
    Write-Success "Saved $Name for current user and current session."
}

function Find-VsWherePath {
    $locations = @(
        "$env:ProgramFiles(x86)\Microsoft Visual Studio\Installer\vswhere.exe",
        "$env:ProgramFiles\Microsoft Visual Studio\Installer\vswhere.exe"
    )

    foreach ($path in $locations) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

function Test-VS2022CppWorkload {
    $vsWhere = Find-VsWherePath
    if (-not $vsWhere) {
        return @{ Success = $false; Message = 'vswhere.exe was not found.' }
    }

    $installPath = & $vsWhere -products * -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Workload.VCTools -property installationPath 2>$null | Select-Object -First 1
    if (-not $installPath) {
        return @{ Success = $false; Message = 'No VS 2022 installation with Desktop C++ workload (Microsoft.VisualStudio.Workload.VCTools) was detected.' }
    }

    $msbuildPath = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
    if (-not (Test-Path $msbuildPath)) {
        return @{ Success = $false; Message = "Visual Studio was found at '$installPath', but MSBuild.exe is missing." }
    }

    return @{ Success = $true; Message = "Detected VS 2022 C++ workload at '$installPath'." }
}

function Assert-GitCommitMatch([string]$RepoDir, [string]$ExpectedCommit) {
    if (-not (Test-Path (Join-Path $RepoDir '.git'))) {
        Stop-WithError "PLUGIN_SDK_DIR ('$RepoDir') is not a git repository." "Use a clone of plugin-sdk at commit $ExpectedCommit."
    }

    $actualCommit = (& git -C $RepoDir rev-parse HEAD 2>$null).Trim()
    if ([string]::IsNullOrWhiteSpace($actualCommit)) {
        Stop-WithError "Could not read plugin-sdk commit hash from '$RepoDir'." 'Ensure git is installed and this folder is a valid plugin-sdk clone.'
    }

    Write-Info "Required plugin-sdk commit: $ExpectedCommit"
    Write-Info "Detected plugin-sdk commit: $actualCommit"

    if ($actualCommit -ne $ExpectedCommit) {
        Stop-WithError "plugin-sdk commit mismatch." "Checkout the required commit with: git -C `"$RepoDir`" checkout $ExpectedCommit"
    }

    Write-Success 'plugin-sdk commit matches the required version.'
}

function Invoke-Step([string]$Description, [scriptblock]$Action) {
    Write-Info $Description
    try {
        & $Action
        Write-Success $Description
    }
    catch {
        Stop-WithError "$Description failed." $_.Exception.Message
    }
}

function Find-ProxyDll([string]$RepoRoot) {
    $candidates = @(
        (Join-Path $RepoRoot 'build\windows\x86\release\proxy.dll'),
        (Join-Path $RepoRoot 'build\windows\x86\release\proxy\proxy.dll')
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    $found = Get-ChildItem -Path (Join-Path $RepoRoot 'build') -Filter '*.dll' -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq 'proxy.dll' } |
        Select-Object -First 1

    return $found?.FullName
}

try {
    $repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
    Write-Info "Repository root: $repoRoot"

    if (-not (Test-CommandAvailable 'xmake')) {
        Stop-WithError 'xmake is not installed or not on PATH.' 'Install xmake from https://xmake.io/ and reopen PowerShell.'
    }
    Write-Success 'xmake detected.'

    if (-not (Test-CommandAvailable 'cmake')) {
        Stop-WithError 'cmake is not installed or not on PATH.' 'Install CMake from https://cmake.org/download/ and reopen PowerShell.'
    }
    Write-Success 'cmake detected.'

    $vsCheck = Test-VS2022CppWorkload
    if (-not $vsCheck.Success) {
        Stop-WithError $vsCheck.Message 'Install Visual Studio 2022 with the "Desktop development with C++" workload.'
    }
    Write-Success $vsCheck.Message

    $resolvedGtaSaDir = Resolve-ExistingDirectory -Provided $GtaSaDir -EnvValue $env:GTA_SA_DIR -Prompt 'Enter full path to GTA San Andreas directory (GTA_SA_DIR)'
    if (-not $resolvedGtaSaDir) {
        Stop-WithError 'GTA_SA_DIR is empty or does not exist.' 'Provide a valid GTA San Andreas installation folder.'
    }
    Set-UserEnvironmentVariable -Name 'GTA_SA_DIR' -Value $resolvedGtaSaDir

    $resolvedPluginSdkDir = Resolve-ExistingDirectory -Provided $PluginSdkDir -EnvValue $env:PLUGIN_SDK_DIR -Prompt 'Enter full path to plugin-sdk directory (PLUGIN_SDK_DIR)'
    if (-not $resolvedPluginSdkDir) {
        Stop-WithError 'PLUGIN_SDK_DIR is empty or does not exist.' 'Provide a valid plugin-sdk directory path.'
    }
    Set-UserEnvironmentVariable -Name 'PLUGIN_SDK_DIR' -Value $resolvedPluginSdkDir

    Assert-GitCommitMatch -RepoDir $resolvedPluginSdkDir -ExpectedCommit $RequiredPluginSdkCommit

    Push-Location $repoRoot

    Invoke-Step 'Configuring xmake release mode (xmake f -m release)' {
        & xmake f -m release
        if ($LASTEXITCODE -ne 0) {
            throw "xmake f -m release exited with code $LASTEXITCODE"
        }
    }

    Invoke-Step 'Building client target (xmake --build client)' {
        & xmake --build client
        if ($LASTEXITCODE -ne 0) {
            throw "xmake --build client exited with code $LASTEXITCODE"
        }
    }

    Invoke-Step 'Building server target (xmake --build server)' {
        & xmake --build server
        if ($LASTEXITCODE -ne 0) {
            throw "xmake --build server exited with code $LASTEXITCODE"
        }
    }

    Invoke-Step 'Building proxy target (xmake --build proxy)' {
        & xmake --build proxy
        if ($LASTEXITCODE -ne 0) {
            throw "xmake --build proxy exited with code $LASTEXITCODE"
        }
    }

    $clientOutput = Join-Path $resolvedGtaSaDir 'CoopAndreasSA.dll'
    if (-not (Test-Path $clientOutput)) {
        Stop-WithError "Client output not found at '$clientOutput'." 'Verify GTA_SA_DIR points to your real game root and rebuild.'
    }
    Write-Success "Client DLL is present: $clientOutput"

    $proxyDllSource = Find-ProxyDll -RepoRoot $repoRoot
    if (-not $proxyDllSource) {
        Stop-WithError 'Could not find proxy.dll build output.' 'Check build logs for the proxy target and make sure the build completed.'
    }

    $eaxDestination = Join-Path $resolvedGtaSaDir 'eax.dll'
    $eaxBackup = Join-Path $resolvedGtaSaDir 'eax_orig.dll'

    if (Test-Path $eaxDestination) {
        if (-not (Test-Path $eaxBackup)) {
            Copy-Item -Path $eaxDestination -Destination $eaxBackup -Force
            Write-Success "Backed up existing eax.dll to eax_orig.dll"
        }
        else {
            Write-Warn "eax_orig.dll already exists. Existing eax.dll will be replaced without changing backup."
        }
    }

    Copy-Item -Path $proxyDllSource -Destination $eaxDestination -Force
    Write-Success "Installed proxy as '$eaxDestination'"

    $mainScmPath = Join-Path $resolvedGtaSaDir 'CoopAndreas\main.scm'
    if (-not (Test-Path $mainScmPath)) {
        Write-Warn "main.scm was not found at '$mainScmPath'."
        Write-Host 'Next steps to generate it:' -ForegroundColor Yellow
        Write-Host '  1) Install Sanny Builder 4.' -ForegroundColor Yellow
        Write-Host '  2) Copy files from sdk/Sanny Builder 4/ into your Sanny Builder installation directory.' -ForegroundColor Yellow
        Write-Host '  3) Open scm/main.txt in Sanny Builder, compile, then copy generated files into %GTA_SA_DIR%\CoopAndreas\' -ForegroundColor Yellow
    }
    else {
        Write-Success "Found compiled main.scm at '$mainScmPath'"
    }

    Write-Host "`nSetup and build completed successfully." -ForegroundColor Green
}
catch {
    Stop-WithError 'Setup/build script terminated unexpectedly.' $_.Exception.Message
}
finally {
    Pop-Location -ErrorAction SilentlyContinue
}
