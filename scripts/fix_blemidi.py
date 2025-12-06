import re

# Read file
with open(r'.\Chocotone\BleMidi.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix 1: Add global pClient after myDevice
content = re.sub(
    r'(BLEAdvertisedDevice\* myDevice;)',
    r'\1\r\nBLEClient* pClient = nullptr;',
    content,
    count=1
)

# Fix 2: Change local pClient to use global
content = re.sub(
    r'    BLEClient\* pClient = BLEDevice::createClient\(\);',
    r'    pClient = BLEDevice::createClient();',
    content,
    count=1
)

# Write back
with open(r'.\Chocotone\BleMidi.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("✓ Fixed BleMidi.cpp - added global pClient pointer")
