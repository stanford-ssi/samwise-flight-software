@echo off
rem Build, flash, and debug SAMWISE firmware via SWD + GDB
rem
rem Usage: scripts\swd_debug.bat [--config=picubed-debug|picubed-flight|picubed-bringup]
rem Default config: picubed-debug
rem
rem This starts OpenOCD as a GDB server on port 3333, then connects
rem arm-none-eabi-gdb. The firmware is flashed before debugging begins.

rem Note this never really worked, and was not tested extensively. If this is useful in
rem the future, try it out!

IF "%1"=="" (SET CONFIG=--config=picubed-debug) ELSE (SET CONFIG=%1)

FOR /F "tokens=*" %%i IN ('git rev-parse --show-toplevel') DO SET REPO_ROOT=%%i
cd /d "%REPO_ROOT%"

SET OPENOCD=C:\Users\ayush\.pico-sdk\openocd\0.12.0+dev\openocd.exe
SET OPENOCD_SCRIPTS=C:\Users\ayush\.pico-sdk\openocd\0.12.0+dev\scripts
SET ELF=bazel-bin\samwise.elf

echo Building :samwise_elf (%CONFIG%)...
bazel build :samwise_elf %CONFIG%
IF ERRORLEVEL 1 goto :error

echo Flashing firmware...
"%OPENOCD%" ^
    -s "%OPENOCD_SCRIPTS%" ^
    -f interface/cmsis-dap.cfg ^
    -f target/rp2350.cfg ^
    -c "adapter speed 5000" ^
    -c "program %ELF% verify reset exit"
IF ERRORLEVEL 1 goto :error

echo Starting OpenOCD GDB server on :3333...
start "OpenOCD" "%OPENOCD%" ^
    -s "%OPENOCD_SCRIPTS%" ^
    -f interface/cmsis-dap.cfg ^
    -f target/rp2350.cfg ^
    -c "adapter speed 5000"

timeout /t 2 /nobreak >nul

echo Connecting GDB...
arm-none-eabi-gdb "%ELF%" ^
    -ex "target extended-remote :3333" ^
    -ex "monitor reset halt" ^
    -ex "break main" ^
    -ex "continue"

goto :cleanup

:error
echo ERROR: Command failed.

:cleanup
echo Stopping OpenOCD...
taskkill /f /fi "WINDOWTITLE eq OpenOCD" >nul 2>&1
