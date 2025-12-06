import re
import json

INPUT_FILE = "pocketedit.html"
OUTPUT_FILE = "sys_ex_data.h"

def extract_delay_data():
    try:
        with open(INPUT_FILE, 'r', encoding='utf-8') as f:
            content = f.read()

        # 1. Locate the "parameters" section for Module 7 (Delay) -> Effect 0 (Pure) -> Param 1 (Time)
        # Structure: "7": { "0": { "1": { "20": "HEX", ... } } }
        
        # Find start of Module 7 parameters
        mod7_index = content.find('"7":')
        if mod7_index == -1: raise ValueError("Module 7 not found")
        
        # Find start of Effect 0 (Pure) within Module 7
        eff0_index = content.find('"0":', mod7_index)
        if eff0_index == -1: raise ValueError("Effect 0 not found in Module 7")
        
        # Find start of Parameter 1 (Time) within Effect 0
        param1_index = content.find('"1":', eff0_index)
        if param1_index == -1: raise ValueError("Parameter 1 (Time) not found")
        
        # Find the closing brace of this parameter block
        # We'll search for the next "2": (Feedback) or the closing of the effect object
        block_end_index = content.find('}', param1_index)
        
        # Extract just this chunk of text
        target_block = content[param1_index:block_end_index+1]
        
        # 2. Extract keys and hex strings from this block only
        matches = re.findall(r'"(\d+)":\s*"([0-9a-fA-F]+)"', target_block)
        
        delay_map = {}
        for time_str, hex_str in matches:
            delay_map[int(time_str)] = hex_str

        print(f"Found {len(delay_map)} delay time entries.")

        # 3. Write the C++ Header File
        with open(OUTPUT_FILE, 'w') as out:
            out.write("#ifndef SYS_EX_DATA_H\n")
            out.write("#define SYS_EX_DATA_H\n\n")
            out.write("#include <Arduino.h>\n\n")
            out.write("// Auto-generated from pocketedit.html\n")
            out.write("// Contains raw SysEx (F0...F7) stripped of the 80 80 BLE header\n")
            out.write(f"const uint8_t PROGMEM delayTimeSysEx[1001][36] = {{\n")
            
            for i in range(1001):
                if i in delay_map:
                    full_hex = delay_map[i]
                    # Pocket Edit format: 80 80 F0 ... F7
                    # We strip the first 4 chars (80 80) to get raw SysEx
                    sysex_hex = full_hex[4:] 
                    
                    # Format as C array: 0xF0, 0x05, ...
                    bytes_list = [f"0x{sysex_hex[j:j+2]}" for j in range(0, len(sysex_hex), 2)]
                    c_array = ", ".join(bytes_list)
                    out.write(f"    {{ {c_array} }}, // {i} ms\n")
                else:
                    # Fill empty slots (0-19ms) with zeros
                    out.write(f"    {{ 0 }}, // {i} (No Data)\n")

            out.write("};\n\n")
            out.write("#endif\n")
            
        print(f"✅ Successfully generated {OUTPUT_FILE}")

    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    extract_delay_data()