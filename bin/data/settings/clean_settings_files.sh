#!/bin/bash
# Run from the script's own directory - the app execl()s this with an
# absolute path, so the working directory is not the project root.
cd "$(dirname "$0")" || exit 1
python3 clean_settings_files.py
