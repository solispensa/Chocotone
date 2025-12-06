# Utility Scripts

This directory contains helper scripts for development and maintenance of the ESP32 MIDI Controller.

## Scripts

### `generate_sysex.ps1`
Generates SysEx data files from source data. Used to create the delay time lookup tables.

### `generate_header.ps1` / `generate_header (2).ps1`
Creates C header files from data sources.

### `fix_blemidi.ps1` / `fix_blemidi.py`
Utility to fix BLE MIDI-related issues during development. Available in both PowerShell and Python versions.

### `find_max_length.ps1`
Analyzes data files to find maximum length values for array sizing.

## Usage

Run these scripts from the repository root directory. Most scripts are written in PowerShell for Windows development environment.
