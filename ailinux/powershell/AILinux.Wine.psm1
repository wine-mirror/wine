# SPDX-License-Identifier: LGPL-2.1-or-later
# Native Linux PowerShell control plane. NEVER runs privileged commands.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-AILinuxWineContext {
    [CmdletBinding()]
    param(
        [string]$Prefix = "$HOME/.local/share/ailinux-wine/prefixes/ailinux-win11",
        [string]$Wine = "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine"
    )
    $homeRoot = [IO.Path]::GetFullPath($HOME).TrimEnd('/')
    $allowed = "$homeRoot/.local/share/ailinux-wine/prefixes/"
    $full = [IO.Path]::GetFullPath($Prefix)
    if (-not $full.StartsWith($allowed, [StringComparison]::Ordinal) -or
        $full -eq "$homeRoot/.wine" -or
        -not (Test-Path -LiteralPath $full -PathType Container)) {
        throw "Refusing missing or unsafe Wine prefix: $Prefix"
    }
    $prefixDir = Get-Item -LiteralPath $full -Force
    if ($prefixDir.LinkType) { throw 'Symlinked Wine prefixes are forbidden' }
    if (-not (Test-Path -LiteralPath (Join-Path $full 'system.reg') -PathType Leaf)) {
        throw "Uninitialized Wine prefix: $full"
    }
    $engine = [IO.Path]::GetFullPath($Wine)
    if (-not $engine.StartsWith("$homeRoot/.local/opt/ailinux-wine/", [StringComparison]::Ordinal) -or
        -not (Test-Path -LiteralPath $engine -PathType Leaf)) {
        throw "Refusing unmanaged Wine executable: $Wine"
    }
    [pscustomobject]@{ Prefix = $full; Wine = $engine; Home = $homeRoot }
}

function Invoke-AILinuxWineTool {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Executable, [string[]]$Arguments = @(), [int]$TimeoutSeconds = 60)
    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $Executable
    foreach ($argument in $Arguments) { [void]$psi.ArgumentList.Add($argument) }
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $process = [Diagnostics.Process]::Start($psi)
    try {
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $process.Kill($true)
            throw "Command timed out: $Executable"
        }
        [pscustomobject]@{
            ExitCode = $process.ExitCode
            Stdout = $stdout.GetAwaiter().GetResult()
            Stderr = $stderr.GetAwaiter().GetResult()
        }
    } finally { $process.Dispose() }
}

function Get-AILinuxWineStatus {
    [CmdletBinding()]
    param([string]$Prefix = "$HOME/.local/share/ailinux-wine/prefixes/ailinux-win11",
          [string]$Wine = "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine")
    $ctx = Get-AILinuxWineContext -Prefix $Prefix -Wine $Wine
    $info = Invoke-AILinuxWineTool -Executable $ctx.Wine -Arguments @('--version')
    if ($info.ExitCode -ne 0) { throw "Wine version check failed: $($info.Stderr)" }
    $files = @('system.reg', 'user.reg', 'drive_c/windows/system32/kernel32.dll')
    $checks = foreach ($file in $files) {
        [pscustomobject]@{ File = $file; Present = Test-Path -LiteralPath (Join-Path $ctx.Prefix $file) }
    }
    [pscustomobject]@{ Prefix = $ctx.Prefix; Engine = $ctx.Wine
        Version = $info.Stdout.Trim(); Checks = @($checks)
        PowerShell = $PSVersionTable.PSVersion.ToString()
        HostOS = [Runtime.InteropServices.RuntimeInformation]::OSDescription }
}

function Test-AILinuxWineIntegrity {
    [CmdletBinding()]
    param([string]$Prefix = "$HOME/.local/share/ailinux-wine/prefixes/ailinux-win11",
          [string]$Wine = "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine")
    $ctx = Get-AILinuxWineContext -Prefix $Prefix -Wine $Wine
    $files = @('system.reg','user.reg','userdef.reg')
    $details = foreach ($file in $files) {
        $target = Join-Path $ctx.Prefix $file
        [pscustomobject]@{ File = $file
          Exists = Test-Path -LiteralPath $target -PathType Leaf
          SHA256 = if (Test-Path -LiteralPath $target -PathType Leaf) { (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash } else { $null } }
    }
    $runtime = Join-Path $PSScriptRoot '../runtime-manager.py'
    $result = Invoke-AILinuxWineTool -Executable '/usr/bin/python3' -Arguments @('-B',$runtime,'--prefix',$ctx.Prefix,'status')
    [pscustomobject]@{ Healthy = (@($details | Where-Object { -not $_.Exists }).Count -eq 0 -and $result.ExitCode -eq 0)
      Prefix = $ctx.Prefix; RegistryFiles = @($details); Runtimes = $result.Stdout.Trim(); RuntimeError = $result.Stderr.Trim() }
}

function Backup-AILinuxWinePrefix {
    [CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
    param([string]$Prefix = "$HOME/.local/share/ailinux-wine/prefixes/ailinux-win11",
          [string]$Wine = "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine")
    $ctx = Get-AILinuxWineContext -Prefix $Prefix -Wine $Wine
    $backups = "$($ctx.Home)/.local/share/ailinux-wine/backups"
    $destination = Join-Path $backups ("{0}-pwsh-{1}" -f (Split-Path $ctx.Prefix -Leaf),[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
    if ($PSCmdlet.ShouldProcess($ctx.Prefix, "Create isolated copy at $destination")) {
        [void](New-Item -ItemType Directory -Path $backups -Force)
        $res = Invoke-AILinuxWineTool -Executable '/usr/bin/cp' -Arguments @('-a','--reflink=auto','--',$ctx.Prefix,$destination) -TimeoutSeconds 120
        if ($res.ExitCode -ne 0) { throw "Backup failed: $($res.Stderr)" }
        [pscustomobject]@{ Source = $ctx.Prefix; Backup = $destination; Created = (Test-Path -LiteralPath "$destination/system.reg") }
    }
}

function Get-AILinuxWineRuntimeAudit {
    [CmdletBinding()]
    param([string]$Prefix = "$HOME/.local/share/ailinux-wine/prefixes/ailinux-win11",
          [string]$Wine = "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine")
    $ctx = Get-AILinuxWineContext -Prefix $Prefix -Wine $Wine
    $tool = Join-Path $PSScriptRoot '../runtime-audit.py'
    $res = Invoke-AILinuxWineTool -Executable '/usr/bin/python3' -Arguments @('-B',$tool,'--wine',$ctx.Wine,'--prefix',$ctx.Prefix) -TimeoutSeconds 90
    if ($res.ExitCode -ne 0) { throw "Audit failed: $($res.Stderr)" }
    $res.Stdout | ConvertFrom-Json
}

function Invoke-AILinuxWineGraphicsRuntime {
    [CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
    param([ValidateSet('status','install','remove')][string]$Action = 'status',
          [string]$Prefix = "$HOME/.local/share/ailinux-wine/prefixes/ailinux-win11",
          [string]$Wine = "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine")
    $ctx = Get-AILinuxWineContext -Prefix $Prefix -Wine $Wine
    if ($Action -ne 'status' -and -not $PSCmdlet.ShouldProcess($ctx.Prefix, "Runtime manager action: $Action")) { return }
    $tool = Join-Path $PSScriptRoot '../runtime-manager.py'
    $res = Invoke-AILinuxWineTool -Executable '/usr/bin/python3' -Arguments @('-B',$tool,'--prefix',$ctx.Prefix,$Action) -TimeoutSeconds 180
    if ($res.ExitCode -ne 0) { throw "Runtime manager failed: $($res.Stderr)" }
    $res.Stdout.Trim()
}

function Get-AILinuxWineHostPackages {
    [CmdletBinding()]
    param()
    $res = Invoke-AILinuxWineTool -Executable '/usr/bin/dpkg-query' -Arguments @('-W','-f=${binary:Package} ${Version} ${Status}\n')
    [pscustomobject]@{ ExitCode = $res.ExitCode; Packages = (($res.Stdout -split '\r?\n' | Where-Object { $_ -match '^(wine|powershell|libvulkan1|mesa-vulkan-drivers)' -and $_ -match ' install ok installed$' }) -join [Environment]::NewLine); Warnings = $res.Stderr.Trim() }
}

function Test-AILinuxWineHostIntegrity {
    [CmdletBinding()]
    param()
    $broken = Invoke-AILinuxWineTool -Executable '/usr/bin/dpkg' -Arguments @('--audit') -TimeoutSeconds 45
    $dpkg = Invoke-AILinuxWineTool -Executable '/usr/bin/dpkg-query' -Arguments @('-W','-f=${binary:Package} ${Status}\n') -TimeoutSeconds 90
    $notInstalled = @($dpkg.Stdout -split "\r?\n" | Where-Object { $_ -match '^[^ ]+ install (?!ok installed)' })
    [pscustomobject]@{
        Healthy = ($broken.ExitCode -eq 0 -and -not $broken.Stdout.Trim() -and $notInstalled.Count -eq 0)
        DpkgAudit = $broken.Stdout.Trim()
        DpkgExitCode = $broken.ExitCode
        UnexpectedPackageStates = $notInstalled
        Note = 'Read-only dpkg audit. Full filesystem checksum validation is not performed.'
    }
}

Export-ModuleMember -Function Get-AILinuxWineStatus,Test-AILinuxWineIntegrity,Backup-AILinuxWinePrefix,Get-AILinuxWineRuntimeAudit,Invoke-AILinuxWineGraphicsRuntime,Get-AILinuxWineHostPackages,Test-AILinuxWineHostIntegrity
