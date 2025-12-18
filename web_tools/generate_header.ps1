$ErrorActionPreference = "Stop"
$baseDir = "$env:USERPROFILE\Documents\AG 2025test"
$htmlPath = "$baseDir\web_tools\offline_editor_v2.html"
$dbPath = "$baseDir\web_tools\devices_database.js"
$targetPath = "$baseDir\Chocotone\WebEditorHTML.h"

Write-Host "Reading files..."
$html = Get-Content $htmlPath -Raw -Encoding UTF8
$db = Get-Content $dbPath -Raw -Encoding UTF8

Write-Host "Embedding database..."
# Escape $ in db content or use string concatenation to prevent interpolation
# The safest way is to avoid double quotes around $db
$scriptTag = "<script>`n" + $db + "`n</script>"
$html = $html -replace '<script src="devices_database.js"></script>', $scriptTag

Write-Host "Patching saveChanges..."
$newSave = "function saveChanges() { try { var json = JSON.stringify(presetData); document.getElementById('jsonArea').value = JSON.stringify(presetData, null, 2); console.log('Saving JSON as file, length: ' + json.length); var blob = new Blob([json], { type: 'application/json' }); var formData = new FormData(); formData.append('config', blob, 'config.json'); var xhr = new XMLHttpRequest(); xhr.open('POST', '/import', true); xhr.onreadystatechange = function() { if (xhr.readyState === 4) { alert('Rebooting device, restart the WiFi and connect again to continue editing'); } }; xhr.onerror = function() { alert('Rebooting device, restart the WiFi and connect again to continue editing'); }; xhr.send(formData); alert('Saving... Device will reboot after save.'); } catch(e) { alert('JS Error: ' + e.message); } }"
$html = $html -replace 'function saveChanges\(\)\s*\{[\s\S]*?alert\([^\)]+\);\s*\}', $newSave


Write-Host "Injecting Init Logic..."
# Replace the trailing render(); call. Fix regex to match any whitespace including newlines.
# Original: render();\n    </script>
# Regex 'render\(\);\s*</script>' allows \s* to consume indentation.
$initScript = "function testPost() { var xhr = new XMLHttpRequest(); xhr.open('POST', '/test', true); xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded'); xhr.onreadystatechange = function() { if (xhr.readyState === 4) { alert('Test: ' + xhr.status + ' ' + xhr.responseText); } }; xhr.send('json_data=test123'); } window.addEventListener('DOMContentLoaded', () => { fetch('/export').then(r => r.json()).then(d => { if(d && d.presets) { presetData = d; if(!d.system.ledMap) d.system.ledMap = [0,1,2,3,7,6,5,4,8,9]; render(); } else { render(); } }).catch(e => { console.error('Fetch error', e); render(); }); });"
$replacement = $initScript + "`n    </script>"
$html = $html -replace 'render\(\);\s*</script>', $replacement

Write-Host "Writing C++ Header..."
$cppContent = "const char EDITOR_HTML[] PROGMEM = R`"raw(" + $html + ")raw`";"
$utf8NoBom = New-Object System.Text.UTF8Encoding $False
[System.IO.File]::WriteAllText($targetPath, $cppContent, $utf8NoBom)

Write-Host "Done."
