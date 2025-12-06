# Fix BleMidi.cpp - add global pClient
$file = ".\ESP32_MIDI\BleMidi.cpp"
$lines = Get-Content $file

# Find line with myDevice and add pClient after it
for ($i = 0; $i < $lines.Count; $i++) {
    if ($lines[$i] -match 'BLEAdvertisedDevice\* myDevice;') {
        # Insert new line after this one
        $lines = $lines[0..$i] + "BLEClient* pClient = nullptr;" + $lines[($i + 1)..($lines.Count - 1)]
        break
    }
}

# Replace local pClient declaration with assignment
for ($i = 0; $i < $lines.Count; $i++) {
    if ($lines[$i] -match '^\s*BLEClient\* pClient = BLEDevice::createClient') {
        $lines[$i] = $lines[$i] -replace 'BLEClient\* pClient =', 'pClient ='
        break
    }
}

Set-Content $file -Value $lines
Write-Host "Fixed BleMidi.cpp"
