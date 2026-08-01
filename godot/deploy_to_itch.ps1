# One-command release: export the Windows release build, then push it to
# itch.io via butler (only uploads what changed, generates patches for the
# itch.io app automatically). Mirrors aevoria-simulator's deploy_to_itch.ps1.
#
# Requires:
#   - A release GDExtension build already exists at
#     bin/libhelgaflightsim.windows.template_release.x86_64.dll (rebuild with
#     `python -m SCons platform=windows target=template_release -j4` from
#     the repo root first if the C++ side changed).
#   - butler installed and logged in (`butler login`) -- already done on
#     this machine, credentials live at ~/.config/itch/butler_creds.
#
# Usage: powershell -File deploy_to_itch.ps1

$ErrorActionPreference = "Stop"

# Adjust if Godot is installed somewhere else on this machine.
$GodotExe = "C:\Program Files\Godot_v4.6.3-stable_win64.exe\Godot_v4.6.3-stable_win64_console.exe"
$ButlerExe = "c:\Users\keefe\AppData\Local\butler\butler.exe"
$ItchTarget = "aevoria-simulator/project-helga-ssto-44-starlifter-ii-flight-simulator:windows"
$BuildDir = Join-Path $PSScriptRoot "builds\windows-release"
$BuildExe = Join-Path $BuildDir "HelgaFlightSim.exe"

Write-Host "Exporting release build..."
# --path is required explicitly -- Godot has no project-discovery fallback,
# so this only "worked" before by accident of whatever directory the caller
# happened to already be in. $PSScriptRoot is this script's own directory
# (godot/), which is always the project root regardless of where the script
# is invoked from.
& $GodotExe --headless --path $PSScriptRoot --export-release "Windows Desktop" $BuildExe
if ($LASTEXITCODE -ne 0) { throw "Godot export failed with exit code $LASTEXITCODE" }

Copy-Item (Join-Path $PSScriptRoot "packaging\Create Desktop Shortcut.bat") $BuildDir -Force

function Get-LiveBuildId {
    $statusJson = & $ButlerExe status $ItchTarget.Split(":")[0] --json 2>$null | Select-String '"type":"result"'
    if (-not $statusJson) { return $null }
    $status = ($statusJson | Select-Object -Last 1).ToString() | ConvertFrom-Json
    return ($status.value.channels | Where-Object { $_.name -eq "windows" }).head
}

# Recorded BEFORE the push, so the post-push poll below has something to
# compare against -- `butler status` right after a push can still return the
# PREVIOUS build for a few seconds while itch.io finishes processing the new
# one ("Build is now processing, should be up in a bit" is not a formality).
$beforeBuild = Get-LiveBuildId

Write-Host "Pushing to itch.io ($ItchTarget)..."
& $ButlerExe push $BuildDir $ItchTarget
if ($LASTEXITCODE -ne 0) { throw "butler push failed with exit code $LASTEXITCODE" }

# Pull the build number butler just assigned (the same number shown on the
# public itch.io page) and stamp it as "1.0.<N>" everywhere else in the repo
# that quotes a version -- see sync_version.ps1 at the repo root for the
# exact file list. This intentionally reads it back from `butler status`
# rather than trusting a locally-incremented counter, since butler is the
# actual source of truth for "what build is live." Polls until the head
# build id actually changes (or times out), rather than trusting the first
# response.
Write-Host "Waiting for itch.io to finish processing the new build..."
$build = $null
for ($i = 0; $i -lt 20; $i++) {
    $build = Get-LiveBuildId
    if ($build -and (-not $beforeBuild -or $build.id -ne $beforeBuild.id)) { break }
    Start-Sleep -Seconds 3
}
if (-not $build -or ($beforeBuild -and $build.id -eq $beforeBuild.id)) {
    throw "Timed out waiting for the new build to appear in butler status -- version numbers NOT synced. Re-run sync_version.ps1 by hand once 'butler status $($ItchTarget.Split(':')[0])' shows the new build."
}
if ($build.state -ne "completed") {
    Write-Host "Warning: new build $($build.id) is still in state '$($build.state)', not 'completed' -- proceeding anyway since the version number itself won't change further."
}
$buildNumber = $build.version
Write-Host "Syncing repo version numbers to build #$buildNumber..."
powershell -File (Join-Path $PSScriptRoot "..\sync_version.ps1") -BuildNumber $buildNumber
if ($LASTEXITCODE -ne 0) { throw "sync_version.ps1 failed with exit code $LASTEXITCODE" }

Write-Host "Done. Live version: 1.0.$buildNumber -- check status with: $ButlerExe status aevoria-simulator/project-helga-ssto-44-starlifter-ii-flight-simulator"
