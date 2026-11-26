#!/usr/bin/env bash
set -euo pipefail

echo "Building VoxRide VST3 / AU / Standalone for macOS..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel

echo ""
echo "Build complete! Plugin artifacts installed automatically to:"
echo "  VST3: ~/Library/Audio/Plug-Ins/VST3/VoxRide.vst3"
echo "  AU:   ~/Library/Audio/Plug-Ins/Components/VoxRide.component"
