@echo off
setlocal
echo Building VoxRide VST3 & Standalone for Windows (x64)...
cmake -B build -A x64
cmake --build build --config Release --parallel

echo.
echo Build complete! Copy the VST3 bundle to your DAW plugin folder if not auto-copied:
echo   build\VoxRide_artefacts\Release\VST3\VoxRide.vst3
echo   Target folder: C:\Program Files\Common Files\VST3\
pause
