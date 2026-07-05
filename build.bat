@echo off

IF NOT EXIST .\bin mkdir .\bin

set build_output=-Fe:"bin\limax.exe" -Fo:"bin\limax.obj" -Fd:"bin\vc140.pdb"
set build_settings=-nologo -GR- -EHa- -D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -Wall -WX -wd4710

IF "%1"=="release" (
    set build_settings=%build_settings% -O2i
) ELSE (
    :: DEBUG BUILD
    set build_settings=%build_settings% -Odi -Zi -DNOM_ENABLE_ASSERT -wd4711 -wd4100 -wd4820 -wd4189 -wd5045 -wd4005 -wd4668 -wd4702 -wd4201 -wd5246 -wd4191 -wd4127 -wd4514 -wd4309 -wd4056 -wd4310 -wd4239
)

cl "src\limax.cpp" /I "external\nom" /I "external\stb" %build_settings% %build_output%
