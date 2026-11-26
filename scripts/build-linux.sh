#!/usr/bin/env bash
set -euo pipefail

echo "Building VoxRide VST3 & Standalone for Linux..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel

echo ""
echo "Build complete! VST3 installed to:"
echo "  ~/.vst3/VoxRide.vst3"
