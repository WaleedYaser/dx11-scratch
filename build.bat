@echo off

if "%~1"=="/d" (
    set DEBUG=true
) else (
    set DEBUG=false
)

call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd" -arch=amd64

SETLOCAL

SET ROOT_DIR=%~dp0
SET BUILD_DIR=%ROOT_DIR%build\

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%

if %DEBUG$==true (
    SET COMPILER_FLAGS=/MTd /nologo /GR- /EHa /Od /Oi /WX /W4 /wd4201 /wd4100 /wd4109 /FC /Z7
    SET LINKER_FLAGS=/incremental:no /opt:ref
) else (
    SET COMPILER_FLAGS=/MTd /nologo /GR- /EHa /Od /Oi /WX /W4 /wd4201 /wd4100 /wd4109 /FC /Z7
    SET LINKER_FLAGS=/incremental:no /opt:ref
)

SET MAIN_COMPILER_FLAGS=%COMPILER_FLAGS% /Fe: main.exe /Fm: main.map
SET MAIN_LINKER_FLAGS=%LINKER_FLAGS%
SET MAIN_SHARED_LIBS=
cl %MAIN_COMPILER_FLAGS% %ROOT_DIR%main.cpp /link %MAIN_LINKER_FLAGS% %MAIN_SHARED_LIBS%

SET ERR=%errorlevel%
if %ERR%==0 (
    echo Success!
)

exit /b %ERR%

popd

ENDLOCAL