import re
import os

input_file = r'c:\Users\André Solis\Documents\AG 2025test\Chocotone\delay_time_sysex.h'
output_file = r'c:\Users\André Solis\Documents\AG 2025test\delay_sysex.js'

print(f"Reading {input_file}...")

with open(input_file, 'r') as f:
    content = f.read()

# Regular expression to match each entry
# {20, {0xF0,0x08...}}
# Capture delay (ms) and data bytes
entry_pattern = re.compile(r'\{\s*(\d+),\s*\{(.*?)\}\s*\}', re.DOTALL)

matches = entry_pattern.findall(content)

print(f"Found {len(matches)} entries.")

js_content = "// Auto-generated from delay_time_sysex.h\n"
js_content += "// Contains lookup table for SPM Delay Time SysEx commands (20ms - 1000ms)\n\n"
js_content += "const DELAY_TIME_LOOKUP = [\n"

count = 0
for delay_ms, data_str in matches:
    # Clean up data string (remove 0x, make decimal or keep hex string?)
    # Keeping as array of hex bytes is fine for JS
    bytes_list = [b.strip() for b in data_str.split(',')]
    
    # Store as compact structure
    js_content += f"    {{ ms: {delay_ms}, data: [{', '.join(bytes_list)}] }},\n"
    count += 1

js_content += "];\n\n"
js_content += "console.log(`Loaded DELAY_TIME_LOOKUP with ${DELAY_TIME_LOOKUP.length} entries`);\n"

print(f"Writing {count} entries to {output_file}...")

with open(output_file, 'w') as f:
    f.write(js_content)

print("Done!")
