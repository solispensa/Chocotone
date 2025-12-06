$inputFile = ".\Sysex List.txt"
$outputFile = ".\sys_ex_data.h"

$headerContent = @"
#ifndef SYS_EX_DATA_H
#define SYS_EX_DATA_H

#include <Arduino.h>

// SysEx data for Delay Time (20ms - 1000ms)
// Index is (ms) directly. 0-19 are empty.
const uint8_t PROGMEM delayTimeSysEx[1001][38] = {
"@

Set-Content -Path $outputFile -Value $headerContent

# Initialize array with zeros for 0-19
for ($i = 0; $i -lt 20; $i++) {
    Add-Content -Path $outputFile -Value "    {0},"
}

# Read file and parse
Get-Content $inputFile | ForEach-Object {
    if ($_ -match "delay_time\s+(\d+)\s+([0-9a-fA-F]+)") {
        $ms = [int]$matches[1]
        $hex = $matches[2]
        
        # Convert hex string to byte array format: 0xF0, 0x08, ...
        $bytes = for ($i = 0; $i -lt $hex.Length; $i += 2) {
            "0x" + $hex.Substring($i, 2)
        }
        $byteString = $bytes -join ", "
        
        # We need to ensure the array index matches the ms.
        # The file seems to be sorted by ms.
        # We assume the file has entries for 20, 21, ... 1000 in order.
        # If there are gaps, this simple append might fail to align indices if we strictly rely on line order.
        # However, the file looked sequential.
        # To be safe, we could use a hashtable, but for now let's assume sequentiality as observed.
        
        Add-Content -Path $outputFile -Value "    {$byteString}, // $ms ms"
    }
}

Add-Content -Path $outputFile -Value "};"
Add-Content -Path $outputFile -Value "#endif // SYS_EX_DATA_H"

Write-Host "Header file generated at $outputFile"
