param(
    [string]$BuildDir = "SpikyMC-build",
    [string]$ReleaseDir = "SpikyMC-release",
    [switch]$ConfigureOnly,
    [switch]$OnlyCleanUserData,
    [switch]$SkipCleanUserData,
    [switch]$CleanAll,
    [switch]$SkipDist
)

$ErrorActionPreference = "Stop"

function Get-ScriptDir {
    if ($PSScriptRoot) { return $PSScriptRoot }
    return (Get-Location).Path
}

function Resolve-Path2 {
    param([string]$Base, [string]$P)
    if ([System.IO.Path]::IsPathRooted($P)) { return $P }
    return (Join-Path $Base $P)
}

$root = Get-ScriptDir
$parent = Split-Path -Parent $root
$buildDir = Resolve-Path2 $parent $BuildDir
$releaseDir = Resolve-Path2 $parent $ReleaseDir
$distRoot = Join-Path $parent "dist"
$exe = Join-Path $buildDir "spikymc.exe"

# user/runtime data that must never ship in a release build
# NOTE: "jars" (JavaCheck.jar, NewLaunch.jar, NewLaunchLegacy.jar) is NOT runtime data —
# it is mandatory app content shipped by default, so it must NOT be excluded.
$UserDataDirNames = @(
    "accounts.json",
    "playtime.cfg",
    "spikymc_credentials.cfg",
    "spikymc.cfg",
    "spikymc_update.cfg",
    "instgroups.json",
    "metacache",
    "instances",
    "libraries",
    "java",
    "meta",
    "assets",
    "translations",
    "cache",
    "networkinformation",
    "logs"
)

if (-not (Test-Path $buildDir)) {
    Write-Error "Build directory not found: $buildDir. Run cmake configure first."
    exit 1
}
if (-not (Test-Path $releaseDir)) {
    Write-Error "Release directory not found: $releaseDir"
    exit 1
}

function Stop-Launcher {
    Get-Process spikymc -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1
}

function Clear-UserData {
    param([string]$Dir)

    $targets = @(
        "accounts.json",
        "playtime.cfg",
        "spikymc_credentials.cfg"
    )

    if ($CleanAll) {
        $targets += @(
            "spikymc.cfg",
            "spikymc_update.cfg",
"instgroups.json",
    "instances",
    "libraries",
    "java",
    "meta",
            "assets",
            "translations",
            "cache",
            "metacache",
            "networkinformation",
            "logs"
        )
    } else {
        $targets += "metacache"
    }

    foreach ($name in $targets) {
        $path = Join-Path $Dir $name
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force -ErrorAction SilentlyContinue
            Write-Host "cleaned: $name"
        }
    }
}

function Get-BuildVersion {
    $cmRaw = Get-Content -LiteralPath (Join-Path $root "CMakeLists.txt") -Raw
    $major = if ($cmRaw -match "Launcher_VERSION_MAJOR (\d+)") { $matches[1] } else { "0" }
    $minor = if ($cmRaw -match "Launcher_VERSION_MINOR (\d+)") { $matches[1] } else { "0" }
    $patch = if ($cmRaw -match "Launcher_VERSION_PATCH (\d+)") { $matches[1] } else { "0" }
    $ver = "$major.$minor.$patch"

    $cache = ""
    if (Test-Path -LiteralPath (Join-Path $buildDir "CMakeCache.txt")) {
        $cache = Get-Content -LiteralPath (Join-Path $buildDir "CMakeCache.txt") -Raw
    }
    $channel = ""
    if ($cache -match "Launcher_BUILD_CHANNEL:STRING=([^\r]*)") { $channel = $matches[1] }
    $buildNum = ""
    if ($cache -match "Launcher_BUILD_NUMBER:STRING=([^\r]*)") { $buildNum = $matches[1] }

    if ($channel -and $channel -ne "stable") {
        $ver += "-" + $channel
        if ($buildNum -and $buildNum -ne "0") { $ver += "." + $buildNum }
    }
    return $ver
}

function Make-Dist {
    $version = Get-BuildVersion
    $distDir = Join-Path $distRoot ("SpikyMC-" + $version)

    if (Test-Path -LiteralPath $distDir) {
        Remove-Item -LiteralPath $distDir -Recurse -Force
        Write-Host "removed previous dist: $distDir"
    }
    New-Item -ItemType Directory -Path $distDir -Force | Out-Null

    # copy everything from the release tree except user/runtime data
    foreach ($entry in Get-ChildItem -LiteralPath $releaseDir -Force) {
        if ($UserDataDirNames -contains $entry.Name) { continue }
        Copy-Item -LiteralPath $entry.FullName -Destination $distDir -Recurse -Force
    }

    # ensure portable marker in the dist folder
    $marker = Join-Path $distDir "portable.txt"
    if (-not (Test-Path -LiteralPath $marker)) {
        Set-Content -LiteralPath $marker -Value @"
This file enables the portable mode for the launcher.

If this file is present in the root directory of the launcher, it will store all data here. Otherwise it will store your data in your appdata directory.
You can safely delete this file, if you don't want the launcher to store your data here.
"@ -Encoding UTF8
    }

    # qt.conf: make Qt self-contained inside the bundle (same as Prism packages).
    # qtlogging.ini: default log filtering shipped with every Prism build.
    if (-not (Test-Path -LiteralPath (Join-Path $distDir "qt.conf"))) {
        Set-Content -LiteralPath (Join-Path $distDir "qt.conf") -Value " " -Encoding ASCII
    }
    $qtlogging = Join-Path $root "launcher\qtlogging.ini"
    if (Test-Path -LiteralPath $qtlogging) {
        Copy-Item -LiteralPath $qtlogging -Destination $distDir -Force
    }

    # manifest.txt: list of shipped files (used by the standalone updater
    # to diff old vs new releases). Same format as Prism's packaging action.
    $manifestLines = Get-ChildItem -LiteralPath $distDir -Recurse -File | ForEach-Object {
        $_.FullName.Substring($distDir.Length).TrimStart('\').Replace('/', '\')
    }
    Set-Content -LiteralPath (Join-Path $distDir "manifest.txt") -Value $manifestLines -Encoding ASCII

    Write-Host "dist built: $distDir"
    return $distDir
}

if ($OnlyCleanUserData) {
    if (-not $SkipCleanUserData) {
        Stop-Launcher
        Clear-UserData $releaseDir
    }
    exit 0
}

if (-not $ConfigureOnly) {
    Stop-Launcher
}

if (-not $SkipCleanUserData) {
    Clear-UserData $releaseDir
}

$env:PATH = "C:\msys64\clang64\bin;C:\msys64\usr\bin;" + $env:PATH

Push-Location $root
try {
    cmake --build $buildDir --config Release --target SpikyMC -j 12
    if ($LASTEXITCODE -ne 0) { throw "cmake build failed with exit code $LASTEXITCODE" }
} finally {
    Pop-Location
}

if (-not $ConfigureOnly) {
    if (Test-Path -LiteralPath $exe) {
        Copy-Item -LiteralPath $exe -Destination $releaseDir -Force
        Write-Host "deployed: $(Get-Item (Join-Path $releaseDir 'spikymc.exe'))"
    } else {
        Write-Error "Built spikymc.exe not found at $exe"
        exit 1
    }

    # Java checker + launcher jars must ship next to the executable.
    $jarsSrc = Join-Path $buildDir "jars"
    if (Test-Path -LiteralPath $jarsSrc) {
        $jarsDst = Join-Path $releaseDir "jars"
        New-Item -ItemType Directory -Path $jarsDst -Force | Out-Null
        foreach ($jar in Get-ChildItem -LiteralPath $jarsSrc -Filter "*.jar") {
            Copy-Item -LiteralPath $jar.FullName -Destination $jarsDst -Force
            Write-Host "deployed: jars/$($jar.Name)"
        }
    } else {
        Write-Error "Java jars not found at $jarsSrc"
        exit 1
    }

    if (-not $SkipDist) {
        Make-Dist
    }
}

Write-Host "Done."