-- premake5.lua
workspace "JScr"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "TestApp"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "JScrCore"
	include "JScrCore/Build.lua"
group ""

include "TestApp/Build.lua"