set PROJECT=NauiApp

set CONFIG=Debug
if /I "%~1"=="-release" (
    set CONFIG=Release
    shift
) else if /I "%~1"=="-r" (
    set CONFIG=Release
    shift
)

premake5 vs2022
msbuild "%PROJECT%.sln" /p:Configuration=%CONFIG%