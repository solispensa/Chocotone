const fs = require('fs');
const path = require('path');

const inputFile = path.join(__dirname, 'Chocotone', 'delay_time_sysex.h');
const outputFile = path.join(__dirname, 'delay_sysex.js');

console.log(`Reading ${inputFile}...`);

try {
    const content = fs.readFileSync(inputFile, 'utf8');

    // Regex to match: {20, {0xF0, 0x08...}}
    // Matches { digits, { hex, hex... } }
    const entryPattern = /\{\s*(\d+),\s*\{(.*?)\}\s*\}/gs;

    let match;
    let count = 0;

    let jsContent = "// Auto-generated from delay_time_sysex.h\n";
    jsContent += "// Contains lookup table for SPM Delay Time SysEx commands (20ms - 1000ms)\n\n";
    jsContent += "const DELAY_TIME_LOOKUP = [\n";

    while ((match = entryPattern.exec(content)) !== null) {
        const delayMs = match[1];
        const dataStr = match[2];

        // Clean up data string (remove newlines, extra spaces)
        // items are like "0xF0,0x08,..."
        const bytes = dataStr.split(',')
            .map(s => s.trim())
            .filter(s => s.length > 0);

        jsContent += `    { ms: ${delayMs}, data: [${bytes.join(', ')}] },\n`;
        count++;
    }

    jsContent += "];\n\n";
    jsContent += "console.log(`Loaded DELAY_TIME_LOOKUP with ${DELAY_TIME_LOOKUP.length} entries`);\n";

    // Add helper function to find closest
    jsContent += `
function getSpmDelaySysex(targetMs) {
    // Constrain to range
    if (targetMs < 20) targetMs = 20;
    if (targetMs > 1000) targetMs = 1000;
    
    let closest = DELAY_TIME_LOOKUP[0];
    let minDiff = Math.abs(closest.ms - targetMs);
    
    // Binary search would be faster but linear is fine for <1000 items on modern JS engine
    // Optimization: The array is sorted by ms, so we can stop when diff increases
    for (let i = 1; i < DELAY_TIME_LOOKUP.length; i++) {
        const entry = DELAY_TIME_LOOKUP[i];
        const diff = Math.abs(entry.ms - targetMs);
        
        if (diff < minDiff) {
            minDiff = diff;
            closest = entry;
        } else if (diff > minDiff) {
            // If difference started increasing, we passed the sweet spot (assuming sorted)
            break;
        }
    }
    
    return closest;
}
`;

    console.log(`Found ${count} entries.`);
    console.log(`Writing to ${outputFile}...`);

    fs.writeFileSync(outputFile, jsContent);
    console.log("Done!");

} catch (err) {
    console.error("Error:", err);
    process.exit(1);
}
