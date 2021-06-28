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

if %DEBUG%==true (
    SET COMPILER_FLAGS=/MTd /nologo /GR- /EHa /Od /Oi /WX /W4 /wd4201 /wd4100 /wd4109 /FC /Z7
    SET LINKER_FLAGS=/incremental:no /opt:ref
) else (
    SET COMPILER_FLAGS=/MTd /nologo /GR- /EHa /Od /Oi /WX /W4 /wd4201 /wd4100 /wd4109 /FC /Z7
    SET LINKER_FLAGS=/incremental:no /opt:ref
)

for /r %%i in (..\*.cpp) do (
    cl %COMPILER_FLAGS% "%%i" /link %LINKER_FLAGS%
)

XCOPY /I/Y %ROOT_DIR%data %BUILD_DIR%data

SET ERR=%errorlevel%
if %ERR%==0 (
    echo success!
)
exit /b %ERR%

popd

ENDLOCAL