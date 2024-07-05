@echo off
setlocal

set rootDir=.\

echo %VULKAN_SDK%
echo %rootDir%

if defined VULKAN_SDK (
    echo VULKAN_SDK is set to: %VULKAN_SDK%
) else (
    echo Error: VULKAN_SDK environment variable is not set.
)

if exist "%rootDir%" (
    echo Root directory is set to: %rootDir%
) else (
    echo Error: The specified root directory does not exist.
)

for /r "%rootDir%" %%i in (*.vert *.frag) do (
    "%VULKAN_SDK%\Bin\glslc.exe" "%%i" -o "%%i.spv"
    echo Compiled: %%i
)

endlocal