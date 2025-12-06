# Find max SysEx length
$data = Get-Content "Sysex List.txt" | Where-Object { $_ -match "^delay_time`t" }
$maxLen = 0
"Checking $($data.Count) entries..."

foreach ($line in $data) {
    $parts = $line -split "`t"
    $hexLen = $parts[2].Length / 2
    if ($hexLen -gt $maxLen) {
        $maxLen = $hexLen
        $maxEntry = $parts[1]
        "Found new max: $maxLen bytes at ${maxEntry}ms"
    }
}

"Maximum SysEx length: $maxLen bytes"
