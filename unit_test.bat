@echo off
set PROJECT=UnitTest
set BUILD_PATH=build/unit_test
set CONFIG=Debug
set ARGS=

if /I "%~1"=="-release" (
    set CONFIG=Release
    shift
) else if /I "%~1"=="-r" (
    set CONFIG=Release
    shift
)

:collect_args
if "%~1"=="" goto run
set ARGS=%ARGS% %1
shift
goto collect_args

:run
"%~dp0%BUILD_PATH%\%CONFIG%\%PROJECT%.exe" %ARGS%