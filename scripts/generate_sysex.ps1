# Generate complete SysEx lookup table from source data
$data = Get-Content "Sysex List.txt" | Where-Object { $_ -match "^delay_time`t" }

Write-Host "Generating complete SysEx lookup table from $($data.Count) entries..."

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("// Complete Delay Time SysEx Lookup Table (20ms - 1000ms)")
[void]$sb.AppendLine("// All $($data.Count) entries from SPM MIDI specification")
[void]$sb.AppendLine("// Stored in PROGMEM (~37KB in flash)")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("#ifndef DELAY_TIME_SYSEX_H")
[void]$sb.AppendLine("#define DELAY_TIME_SYSEX_H")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("#include <Arduino.h>")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("const struct {")
[void]$sb.AppendLine("    uint16_t delayMs;")
[void]$sb.AppendLine("    uint8_t data[38];  // Max size is 38 bytes")
[void]$sb.AppendLine("} DELAY_TIME_LOOKUP[] PROGMEM = {")

foreach ($line in $data) {
    $parts = $line -split "`t"
    $ms = $parts[1]
    $hex = $parts[2]
    
    $hexBytes = @()
    for ($i = 0; $i -lt $hex.Length; $i += 2) {
        $hexBytes += "0x" + $hex.Substring($i,2).ToUpper()
    }
    
    $bytes = $hexBytes -join ","
    [void]$sb.AppendLine("    {$ms, {$bytes}},")
}

# Remove last comma
$content = $sb.ToString()
$lastCommaIndex = $content.LastIndexOf('},')
if ($lastCommaIndex -gt 0) {
    $content = $content.Substring(0, $lastCommaIndex + 1) + $content.Substring($lastCommaIndex + 2)
}

$content += "`n};"
$content += "`n`nconst int DELAY_TIME_LOOKUP_SIZE = sizeof(DELAY_TIME_LOOKUP) / sizeof(DELAY_TIME_LOOKUP[0]);"
$content += "`n`n#endif`n"

# Write with ASCII encoding (no BOM)
[System.IO.File]::WriteAllText("ESP32_MIDI\delay_time_sysex.h", $content, [System.Text.Encoding]::ASCII)

Write-Host "Generated delay_time_sysex.h with $($data.Count) entries (ASCII, no BOM)"
