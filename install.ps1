# Wyn Language Installer for Windows
# Usage: irm wynlang.com/install.ps1 | iex

$ErrorActionPreference = 'Stop'

Write-Host "Wyn Language Installer" -ForegroundColor Green
Write-Host ""

# Detect architecture
$arch = if ([Environment]::Is64BitOperatingSystem) { "x86_64" } else { "x86" }
if ($arch -eq "x86") {
    Write-Host "Error: 32-bit Windows is not supported" -ForegroundColor Red
    exit 1
}

$platform = "windows-$arch"
Write-Host "Detected platform: $platform" -ForegroundColor Yellow

# Get latest release version
Write-Host "Fetching latest version..."
try {
    $release = Invoke-RestMethod -Uri "https://api.github.com/repos/wyn-lang/wyn/releases/latest"
    $version = $release.tag_name
} catch {
    Write-Host "Error: Could not fetch latest version" -ForegroundColor Red
    exit 1
}

Write-Host "Latest version: $version" -ForegroundColor Green

# Download URL
$url = "https://github.com/wyn-lang/wyn/releases/download/$version/wyn-$version-$platform.tar.gz"
$tempFile = "$env:TEMP\wyn.tar.gz"
$installDir = "$env:LOCALAPPDATA\Wyn"

Write-Host "Downloading from: $url"
try {
    Invoke-WebRequest -Uri $url -OutFile $tempFile -UseBasicParsing
} catch {
    Write-Host "Error: Download failed" -ForegroundColor Red
    Write-Host "URL: $url" -ForegroundColor Red
    exit 1
}

# Extract
Write-Host "Extracting..."
New-Item -ItemType Directory -Force -Path $installDir | Out-Null

# Use tar (available in Windows 10+)
tar -xzf $tempFile -C $installDir

# Move binary to root of install dir
if (Test-Path "$installDir\release\wyn.exe") {
    Move-Item -Path "$installDir\release\wyn.exe" -Destination "$installDir\wyn.exe" -Force
}

# Move VERSION file
if (Test-Path "$installDir\release\VERSION") {
    Move-Item -Path "$installDir\release\VERSION" -Destination "$installDir\VERSION" -Force
}

# Move std library
if (Test-Path "$installDir\release\std") {
    Move-Item -Path "$installDir\release\std" -Destination "$installDir\std" -Force
}

# Cleanup release directory
if (Test-Path "$installDir\release") {
    Remove-Item -Path "$installDir\release" -Recurse -Force
}

# Add to PATH
$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($userPath -notlike "*$installDir*") {
    Write-Host "Adding to PATH..."
    [Environment]::SetEnvironmentVariable("Path", "$userPath;$installDir", "User")
    $env:Path = "$env:Path;$installDir"
}

# Cleanup temp file
Remove-Item -Path $tempFile -Force

Write-Host ""
Write-Host "✅ Wyn $version installed successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "Verify installation:" -ForegroundColor Yellow
Write-Host "  wyn --version"
Write-Host ""
Write-Host "Get started:" -ForegroundColor Yellow
Write-Host "  wyn --help"
Write-Host ""
Write-Host "Documentation: https://wynlang.com/docs" -ForegroundColor Yellow
Write-Host ""
Write-Host "Note: Restart your terminal to use 'wyn' command" -ForegroundColor Cyan
