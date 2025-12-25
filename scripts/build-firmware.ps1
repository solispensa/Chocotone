# Chocotone Firmware Build Script
# Run this before pushing to GitHub to compile the .bin file

# Configuration
$VERSION = (Get-Content -Path "manifest.json" | ConvertFrom-Json).version
$OUTPUT_NAME = "chocotone_v$VERSION.bin"

Write-Host "=== Chocotone Firmware Builder ===" -ForegroundColor Cyan
Write-Host "Version: $VERSION" -ForegroundColor Yellow
Write-Host ""

# Check if Arduino IDE build output exists
$buildPath = "$env:TEMP\arduino\sketches"
$sketchName = "Chocotone.ino"

Write-Host "STEP 1: Compile in Arduino IDE" -ForegroundColor Green
Write-Host "   1. Open Chocotone/Chocotone.ino in Arduino IDE"
Write-Host "   2. Go to Sketch > Export Compiled Binary"
Write-Host "   3. Wait for compilation to complete"
Write-Host ""
Write-Host "Press Enter when compilation is done..." -ForegroundColor Yellow
Read-Host

# Look for the compiled files
Write-Host ""
Write-Host "STEP 2: Looking for compiled files..." -ForegroundColor Green

# Common locations for Arduino IDE exports
$possiblePaths = @(
    "Chocotone\build\esp32.esp32.esp32",
    "Chocotone\build",
    "$env:TEMP\arduino_build_*"
)

$binFile = $null
foreach ($path in $possiblePaths) {
    $search = Get-ChildItem -Path $path -Filter "*.bin" -Recurse -ErrorAction SilentlyContinue | 
              Where-Object { $_.Name -like "*Chocotone*" -and $_.Name -notlike "*bootloader*" -and $_.Name -notlike "*partitions*" } |
              Select-Object -First 1
    if ($search) {
        $binFile = $search.FullName
        break
    }
}

if (-not $binFile) {
    Write-Host "Could not find compiled .bin file automatically." -ForegroundColor Red
    Write-Host "Please enter the full path to the Chocotone.ino.bin file:"
    $binFile = Read-Host
}

if (-not (Test-Path $binFile)) {
    Write-Host "Error: File not found at $binFile" -ForegroundColor Red
    exit 1
}

Write-Host "Found: $binFile" -ForegroundColor Green

# Copy to firmware folder
Write-Host ""
Write-Host "STEP 3: Copying to firmware folder..." -ForegroundColor Green

$destPath = "firmware\$OUTPUT_NAME"
Copy-Item -Path $binFile -Destination $destPath -Force
Write-Host "Created: $destPath" -ForegroundColor Green

# Update index.json
Write-Host ""
Write-Host "STEP 4: Updating firmware index..." -ForegroundColor Green

$index = @{
    latest = $VERSION
    versions = @(
        @{
            version = $VERSION
            file = $OUTPUT_NAME
        }
    )
}

# Check for existing versions and merge
if (Test-Path "firmware\index.json") {
    $existing = Get-Content -Path "firmware\index.json" | ConvertFrom-Json
    $existingVersions = $existing.versions | Where-Object { $_.version -ne $VERSION }
    $index.versions = @($index.versions) + @($existingVersions)
}

$index | ConvertTo-Json -Depth 3 | Set-Content -Path "firmware\index.json"
Write-Host "Updated: firmware\index.json" -ForegroundColor Green

Write-Host ""
Write-Host "=== BUILD COMPLETE ===" -ForegroundColor Cyan
Write-Host "Firmware: firmware\$OUTPUT_NAME" -ForegroundColor Yellow
Write-Host ""
Write-Host "Now you can push to GitHub:" -ForegroundColor Green
Write-Host "   git add -A"
Write-Host "   git commit -m 'v$VERSION`: [description]'"
Write-Host "   git push"
