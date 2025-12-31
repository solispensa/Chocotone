#ifndef STORAGE_H
#define STORAGE_H

#include "Globals.h"

void saveSystemSettings();
void loadSystemSettings();
void savePresets();
void loadPresets();
void saveCurrentPresetIndex();
void loadCurrentPresetIndex();
void saveAnalogInputs();
void loadAnalogInputs();
// initializeGlobalOverrides() removed - no longer needed

#endif
