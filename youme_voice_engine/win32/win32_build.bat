@echo off

set VS2013DEVENVPATH="C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE"  

if not exist %VS2013DEVENVPATH% (
    echo "没有找到vs2013安装目录,请编辑脚本的VS2013DEVENVPATH路径"
) else (
    echo build start...
    set BUILDPATH=%cd%
    cd /d %VS2013DEVENVPATH% 
    devenv.exe %BUILDPATH%\vs2013\YouMe.sln /Rebuild Release  /Project %BUILDPATH%\youme_voice_engine\youme_voice_engine.vcxproj /ProjectConfig "Release|Win32"
    cd /d %BUILDPATH%
    echo build ok
)