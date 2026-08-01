# Keeps the "1.0.N" version number consistent across the repo, where N is
# the itch.io build number (the same number shown on the public itch.io
# page and returned by `butler status`). This is the single source of
# truth for what "the version" means for this project -- there's no
# separately-maintained VERSION file to fall out of sync with itch.
# Mirrors aevoria-simulator's sync_version.ps1, scaled down to the files
# that actually exist in this (much smaller) repo.
#
# Called automatically by godot/deploy_to_itch.ps1 after a successful push,
# with the build number butler just reported. Can also be run by hand:
#   powershell -File sync_version.ps1 -BuildNumber 12
#
# Deliberately narrow regexes (anchored to the surrounding text specific to
# each file) rather than a blind "replace 1.0.1 with 1.0.12", so an
# unrelated string that happens to also read "1.0.1" is never touched.

param(
    [int]$BuildNumber = 0
)

$ErrorActionPreference = "Stop"
$RepoRoot = $PSScriptRoot
$Version = "1.0.$BuildNumber"

function Update-InFile {
    param([string]$Path, [string]$Pattern, [string]$Replacement, [string]$Label)
    $full = Join-Path $RepoRoot $Path
    $content = Get-Content -Raw -Path $full
    $updated = $content -replace $Pattern, $Replacement
    if ($updated -eq $content) {
        Write-Host "  (no change) $Label"
        return
    }
    Set-Content -Path $full -Value $updated -NoNewline
    Write-Host "  updated: $Label"
}

if ($BuildNumber -le 0) {
    Write-Host "No -BuildNumber given: nothing to sync."
    exit 0
}

Write-Host "Syncing repo version to $Version (itch build #$BuildNumber)..."

# README.md badge: Version-1.0.1--Alpha-blue (shields.io escapes a literal
# "-" as "--", so the suffix after the version number reads "--Alpha").
Update-InFile "README.md" `
    'Version-1\.0\.\d+--Alpha' `
    "Version-$Version--Alpha" `
    "README.md badge"

# godot/export_presets.cfg: Windows exe file/product version metadata
# (shows up in the .exe's Properties > Details tab in Explorer).
# file_version wants the traditional 4-part Windows form.
Update-InFile "godot/export_presets.cfg" `
    'application/file_version="[^"]*"' `
    "application/file_version=`"$Version.0`"" `
    "export_presets.cfg (file_version)"
Update-InFile "godot/export_presets.cfg" `
    'application/product_version="[^"]*"' `
    "application/product_version=`"$Version`"" `
    "export_presets.cfg (product_version)"

Write-Host "Version sync done."
