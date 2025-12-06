# 🔧 FIX: OLED Stops When Encoder Button Pressed

## Your Problem

When you press the rotary encoder button, the OLED display stops working or glitches.

## Root Cause

Your 5-pin encoder needs power (5V), but when connected incorrectly it can cause:
- Power surge when button pressed
- Ground loop issues
- Current draw exceeding ESP32 3.3V regulator capacity

## Solution Options (Try in Order)

### Option 1: Add Decoupling Capacitor ⚡ **EASIEST FIX**

1. Get a **0.1µF (100nF) ceramic capacitor**
2. Solder it **very close** to the encoder between:
   - VCC pin → GND pin
3. Test encoder button

**Why this works:** Filters power spikes when button pressed

---

### Option 2: Power Encoder from VIN (5V) 🔌

**Current wiring (causing problem):**
```
Encoder VCC → ESP32 3.3V ❌
```

**Correct wiring:**
```
Encoder VCC → ESP32 VIN (5V input) ✅
```

**Steps:**
1. Disconnect encoder VCC from ESP32 3.3V pin
2. Connect encoder VCC to ESP32 **VIN pin** (this is the 5V input)
3. Keep all other connections the same

**Why this works:** VIN provides stable 5V, not limited by 3.3V regulator

---

### Option 3: Try 3.3V Power (If Encoder Tolerates It) 🔄

Some 5V encoders actually work fine at 3.3V:

1. **Current setup stays same** (VCC → 3.3V)
2. But **improve ground connection**:
   - Use thicker wire for GND
   - Ensure common ground point
   - Twist GND and VCC wires together

**Test:** If encoder still works but OLED is stable, this is fine!

---

### Option 4: External 5V Power 🔋 **Most Reliable**

If ESP32 can't supply enough current:

1. Get external 5V power supply (USB adapter)
2. Connect:
   ```
   5V USB → Encoder VCC
   5V USB GND → ESP32 GND (IMPORTANT: common ground!)
   ```
3. Keep signal wires (CLK, DT, SW) to ESP32

---

## Quick Diagnostic

**Test if it's a power issue:**

```cpp
// Add to setup():
pinMode(23, INPUT_PULLUP);  // Encoder button

// In loop(), monitor button without OLED update:
if (digitalRead(23) == LOW) {
    Serial.println("Button pressed");
    delay(500);
}
```

If Serial Monitor shows "Button pressed" reliably but OLED crashes → **Power issue confirmed**

---

## What I Recommend for You

Based on your symptoms, try this order:

1. **First**: Add 0.1µF cap between encoder VCC and GND (5 minute fix)
2. **If that doesn't work**: Move encoder VCC from 3.3V to VIN
3. **If still problems**: Check all ground connections are solid
4. **Last resort**: External 5V power for encoder + NeoPixels

---

## Prevention

When you rebuild/finalize:
- Always use decoupling caps on powered components
- Keep power and ground wires short and thick
- Use common ground star point
- Consider adding 10µF cap on ESP32 VIN

---

## Still Having Issues?

If none of these work, check:
- [ ] Encoder button is NO (Normally Open), not NC
- [ ] No shorts between encoder pins
- [ ] ESP32 board has good USB power supply (2A minimum)
- [ ] OLED and encoder have separate ground paths to ESP32

Let me know which solution works for you!
