<#
.SYNOPSIS
    install.ps1 — one-command installer for Transcript Viewer on Windows.

.DESCRIPTION
    Remote (nothing to clone, nothing to click):

        irm https://raw.githubusercontent.com/SengathitBannavong/Transcript-Viewer/main/install.ps1 | iex

    Local (from a checkout):

        powershell -ExecutionPolicy Bypass -File .\install.ps1

    It downloads the prebuilt transcript-viewer-windows.zip from the Releases
    page, unpacks it into %LOCALAPPDATA%\Programs\Transcript-Viewer, and
    registers a `ctt` command plus a Start Menu shortcut.

    Why an installer script instead of "download the .zip and double-click":
    a file downloaded by a *browser* is tagged with the Mark-of-the-Web, and
    Windows then greets the user with SmartScreen's "Windows protected your PC"
    on first launch. Invoke-WebRequest does not apply that tag, so the same
    binary starts without the scare screen. The script also unblocks every
    extracted file defensively, in case the archive itself arrived marked.

    This does NOT compile anything — the Windows binary is built by CI in an
    MSYS2/MinGW64 environment. To build from source instead, see README.md.

.PARAMETER InstallDir
    Where to install. Default: %LOCALAPPDATA%\Programs\Transcript-Viewer

.PARAMETER Version
    Release tag to install, e.g. v1.2.3. Default: the latest release.

.PARAMETER NoPathUpdate
    Skip adding the install directory to your user PATH (no `ctt` command).

.PARAMETER NoShortcut
    Skip creating the Start Menu shortcut.
#>
[CmdletBinding()]
param(
    [string] $InstallDir,
    [string] $Version,
    [string] $Repo = 'SengathitBannavong/Transcript-Viewer',
    [switch] $NoPathUpdate,
    [switch] $NoShortcut
)

$ErrorActionPreference = 'Stop'

# ── Config ──────────────────────────────────────────────────────────────────
$AssetName   = 'transcript-viewer-windows.zip'
$ProjectName = 'Transcript Viewer'
$CmdName     = 'ctt'          # the command you'll type
$ExeName     = 'program.exe'

# Piping through `iex` gives no way to pass -Parameters, so the two knobs worth
# having remotely are also readable from the environment.
if (-not $InstallDir) {
    $InstallDir = if ($env:INSTALL_DIR) { $env:INSTALL_DIR }
                  else { Join-Path $env:LOCALAPPDATA 'Programs\Transcript-Viewer' }
}
if (-not $Version -and $env:TV_VERSION) { $Version = $env:TV_VERSION }

# ── Pretty output ───────────────────────────────────────────────────────────
function Step($msg) { Write-Host "==> $msg" -ForegroundColor Green }
function Info($msg) { Write-Host "    $msg" }
function Warn($msg) { Write-Host "!!  $msg" -ForegroundColor Yellow }
# `throw`, not `exit`: when this script is run via `irm … | iex` in an
# interactive session, `exit` would close the user's whole console window.
function Die($msg)  { Write-Host "xx  $msg" -ForegroundColor Red; throw 'Installation aborted.' }

# ── 0. Environment guards ───────────────────────────────────────────────────
function Test-Platform {
    # $IsWindows only exists on PowerShell Core; on 5.1 its absence means Windows.
    if ($null -ne $PSVersionTable.Platform -and $PSVersionTable.Platform -ne 'Win32NT') {
        Die "This installer is for Windows. On Linux use install.sh (see README.md)."
    }
    if ($PSVersionTable.PSVersion.Major -lt 5) {
        Die "PowerShell 5.0 or newer is required (found $($PSVersionTable.PSVersion))."
    }
    # Windows 8.1 / 2012 R2 era hosts still default to TLS 1.0, which GitHub
    # rejects outright. Opt into TLS 1.2 before the first request.
    try {
        [Net.ServicePointManager]::SecurityProtocol =
            [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    } catch { }
}

# ── 1. Download ─────────────────────────────────────────────────────────────
# Uses the /releases/latest/download/ redirect rather than the REST API so the
# script keeps working for unauthenticated users who have hit the API rate limit.
function Get-Package($destZip) {
    $url = if ($Version) {
        "https://github.com/$Repo/releases/download/$Version/$AssetName"
    } else {
        "https://github.com/$Repo/releases/latest/download/$AssetName"
    }

    Step "Downloading $AssetName"
    Info $url

    # A progress bar makes Invoke-WebRequest dramatically slower on PS 5.1.
    $oldProgress = $ProgressPreference
    $ProgressPreference = 'SilentlyContinue'
    try {
        Invoke-WebRequest -Uri $url -OutFile $destZip -UseBasicParsing
    } catch {
        if ($Version) {
            Die "Could not download release '$Version'. Check the tag exists at https://github.com/$Repo/releases"
        }
        Die "Download failed: $($_.Exception.Message)`n    Check your connection, or grab $AssetName manually from https://github.com/$Repo/releases/latest"
    } finally {
        $ProgressPreference = $oldProgress
    }

    $size = (Get-Item $destZip).Length
    if ($size -lt 100KB) { Die "Downloaded file is only $size bytes — that is not the package." }
    Info ("Downloaded {0:N1} MB" -f ($size / 1MB))
}

# ── 2. Extract ──────────────────────────────────────────────────────────────
# The archive has a single top-level transcript-viewer-windows/ folder; its
# *contents* are what belong in the install dir.
function Expand-Package($zip, $stage) {
    Step "Extracting"
    # Already loaded on PowerShell 7; only Windows PowerShell 5.1 needs the Add-Type.
    try { Add-Type -AssemblyName System.IO.Compression.FileSystem -ErrorAction Stop } catch { }
    [System.IO.Compression.ZipFile]::ExtractToDirectory($zip, $stage)

    $root = Get-ChildItem -LiteralPath $stage -Directory |
            Where-Object { Test-Path (Join-Path $_.FullName $ExeName) } |
            Select-Object -First 1
    if (-not $root) {
        # Older/flat archives may put program.exe straight at the top level.
        if (Test-Path (Join-Path $stage $ExeName)) { return $stage }
        Die "$ExeName not found in the archive — the package layout changed."
    }
    return $root.FullName
}

# ── 3. Install ──────────────────────────────────────────────────────────────
# Refreshes program.exe, assets\ and Font\. Any db_<user>.db already sitting in
# the install dir is user data and is deliberately left alone.
function Install-Files($src) {
    Step "Installing into $InstallDir"

    if (Test-Path $InstallDir) {
        # Copying over a running .exe fails with a locked-file error that reads
        # like a permissions problem, so check first and say what it really is.
        # Reading .Path throws on processes we cannot open, hence the try.
        $running = Get-Process -Name ([IO.Path]::GetFileNameWithoutExtension($ExeName)) -ErrorAction SilentlyContinue |
                   Where-Object {
                       try { $_.Path -and $_.Path.StartsWith($InstallDir, 'OrdinalIgnoreCase') }
                       catch { $false }
                   }
        if ($running) { Die "$ProjectName is currently running. Close it and re-run the installer." }

        foreach ($stale in 'assets', 'Font') {
            $p = Join-Path $InstallDir $stale
            if (Test-Path $p) { Remove-Item $p -Recurse -Force }
        }
    } else {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    }

    Copy-Item -Path (Join-Path $src '*') -Destination $InstallDir -Recurse -Force

    $exe = Join-Path $InstallDir $ExeName
    if (-not (Test-Path $exe)) {
        # Almost always Defender quarantining an unsigned, low-prevalence binary.
        Warn "$ExeName vanished after being copied."
        Info ""
        Info "This is a Microsoft Defender false positive: the build is unsigned and"
        Info "statically linked, which its heuristics score as suspicious."
        Info ""
        Info "  1. Check quarantine:  Windows Security -> Virus & threat protection"
        Info "                        -> Protection history -> Restore"
        Info "  2. Report it so the detection gets fixed for everyone:"
        Info "     https://www.microsoft.com/en-us/wdsi/filesubmission"
        Info "  3. Or exclude the folder (Settings -> Exclusions -> Add folder):"
        Info "     $InstallDir"
        Die "Install incomplete."
    }

    # Belt and braces: strip any Mark-of-the-Web that rode along inside the zip,
    # so Explorer/SmartScreen treats the files as locally produced.
    Get-ChildItem -LiteralPath $InstallDir -Recurse -File |
        Unblock-File -ErrorAction SilentlyContinue

    Info "Installed: $ExeName, assets\, Font\"
}

# ── 4. `ctt` launcher ───────────────────────────────────────────────────────
# The app resolves assets\, Font\ and db_<user>.db relative to the *working
# directory*, so the shim pins the cwd before launching. `cd` inside a .cmd
# child process cannot affect the caller's shell, so your own cwd is unchanged.
function Write-Launcher {
    $shim = Join-Path $InstallDir "$CmdName.cmd"
    Step "Writing launcher: $shim"
    $lines = @(
        '@echo off'
        "rem Auto-generated by $ProjectName install.ps1 - defines the '$CmdName' command."
        'cd /d "%~dp0"'
        "start `"`" `"%~dp0$ExeName`" %*"
    )
    # ASCII with CRLF: cmd.exe misreads a UTF-8 BOM as a stray command.
    [IO.File]::WriteAllText($shim, ($lines -join "`r`n") + "`r`n", [Text.Encoding]::ASCII)
    return $shim
}

# ── 5. PATH wiring ──────────────────────────────────────────────────────────
# Written through the registry rather than [Environment]::SetEnvironmentVariable
# because that API rewrites a REG_EXPAND_SZ Path as REG_SZ, permanently breaking
# any existing %USERPROFILE%-style entries for the user.
function Register-Path {
    Step "Registering ``$CmdName`` on your PATH"

    $target = $InstallDir.TrimEnd('\')
    $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Environment', $true)
    try {
        $current = $key.GetValue('Path', '', 'DoNotExpandEnvironmentNames')
        $kind    = if ($null -ne $key.GetValue('Path')) { $key.GetValueKind('Path') } else { 'ExpandString' }

        $entries = @($current -split ';' | Where-Object { $_ -ne '' })
        if ($entries | Where-Object { $_.TrimEnd('\') -ieq $target }) {
            Info "Already on your user PATH — nothing to change."
        } else {
            $key.SetValue('Path', (@($entries) + $InstallDir) -join ';', $kind)
            Info "Added $InstallDir to your user PATH."
        }
    } finally {
        # .Close() rather than .Dispose(): present on every .NET version this
        # script can land on, whereas public Dispose() only arrived in 4.6.
        if ($key) { $key.Close() }
    }

    # Independently of the registry, make `ctt` work in *this* session too — a
    # re-run on an already-registered install would otherwise leave the current
    # console without it.
    if (-not (@($env:Path -split ';') | Where-Object { $_.TrimEnd('\') -ieq $target })) {
        $env:Path = "$env:Path;$InstallDir"
    }
}

# ── 6. Start Menu shortcut ──────────────────────────────────────────────────
function New-Shortcut {
    Step "Creating Start Menu shortcut"
    $dir = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs'
    $lnk = Join-Path $dir "$ProjectName.lnk"
    try {
        $shell = New-Object -ComObject WScript.Shell
        $sc = $shell.CreateShortcut($lnk)
        $sc.TargetPath       = Join-Path $InstallDir $ExeName
        $sc.WorkingDirectory = $InstallDir     # required: assets\ is resolved from cwd
        $sc.Description      = "$ProjectName — student transcript viewer"
        $sc.Save()
        Info $lnk
    } catch {
        Warn "Could not create the shortcut: $($_.Exception.Message)"
    }
}

# ── Run ─────────────────────────────────────────────────────────────────────
Step "$ProjectName installer (Windows)"
Info "Target : $InstallDir"
Info "Command: $CmdName"
Write-Host ""

Test-Platform

$tmp = Join-Path ([IO.Path]::GetTempPath()) ("tv-install-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tmp -Force | Out-Null
try {
    $zip   = Join-Path $tmp $AssetName
    $stage = Join-Path $tmp 'stage'

    Get-Package $zip
    $src = Expand-Package $zip $stage
    Install-Files $src
    Write-Launcher | Out-Null
    if (-not $NoPathUpdate) { Register-Path } else { Info "Skipped PATH update (-NoPathUpdate)." }
    if (-not $NoShortcut)   { New-Shortcut }  else { Info "Skipped shortcut (-NoShortcut)." }
} finally {
    Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host ""
Step "Done"
Info "Open a new terminal, then launch with:  $CmdName"
Info "Or use the $ProjectName entry in your Start Menu."
Info "Your database lives in $InstallDir as db_<username>.db"
