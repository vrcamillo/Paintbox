@echo off
setlocal enableDelayedExpansion

rem 
rem /////////////////////////////
rem

set build_folder=.build

set glfw_id=glfw-3.3.8
set imgui_id=imgui-1.88
set glad_id=glad
set paintbox_id=paintbox

rem
rem /////////////////////////////
rem

set glfw_folder=third_party\!glfw_id!
set glfw_build_folder=!build_folder!\.!glfw_id!
set glfw_library=!glfw_build_folder!\glfw.lib

set imgui_folder=third_party\!imgui_id!
set imgui_build_folder=!build_folder!\.!imgui_id!
set imgui_library=!imgui_build_folder!\imgui.lib

set glad_folder=..\third_party\!glad_id!
set glad_build_folder=!build_folder!\.!glad_id!
set glad_library=!glad_build_folder!\glad.lib

set paintbox_folder=..
set paintbox_build_folder=!build_folder!\.!paintbox_id!
set paintbox_library=!paintbox_build_folder!\paintbox.lib

rem These variables are going to be filled below.
set source_files=
set includes=
set defines=
set libraries= kernel32.lib shell32.lib user32.lib gdi32.lib Winmm.lib

rem Check if CL exists
where /q cl
if errorlevel 1 (
    echo Microsoft compiler tools are missing. 
    echo Please run vcvarsall.bat or rerun the script from a developer console.
    echo:
    echo If you need more informative help, check the following website for reference:
    echo 	https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line
    exit /b
)

rem Create the build folder if it does not already exist.
if not exist !build_folder! (mkdir !build_folder!)

rem 
rem GLFW
rem 

echo Searching for !glfw_id!...
if not exist !glfw_library! (
	echo Compiling GLFW...
	
	set glfw_source_files=^
		"!glfw_folder!/src/init.c"^
		"!glfw_folder!/src/monitor.c" ^
		"!glfw_folder!/src/window.c" ^
		"!glfw_folder!/src/input.c" ^
		"!glfw_folder!/src/context.c" ^
		"!glfw_folder!/src/vulkan.c" ^
		^
		"!glfw_folder!/src/win32_init.c" ^
		"!glfw_folder!/src/win32_window.c" ^
		"!glfw_folder!/src/win32_monitor.c" ^
		"!glfw_folder!/src/win32_thread.c" ^
		"!glfw_folder!/src/win32_time.c" ^
		"!glfw_folder!/src/win32_joystick.c" ^
		^
		"!glfw_folder!/src/wgl_context.c" ^
		"!glfw_folder!/src/egl_context.c" ^
		"!glfw_folder!/src/osmesa_context.c"

	if exist !glfw_build_folder! (
		del /S /Q !glfw_build_folder!\* 1>nul
	) else (
		mkdir !glfw_build_folder!
	)
	
	for %%f in (!glfw_source_files!) do ( 
 	  cl /O2 /nologo /c /D"_GLFW_WIN32" %%f /Fo"!glfw_build_folder!\%%~nf.obj"
 	  if errorlevel 1 (
			echo:
			echo Compilation error! Stopping...
			exit /b
		)
	)

	lib /nologo /out:!glfw_library! !glfw_build_folder!\*.obj
)

if exist !glfw_library! (
	echo Success^^! !glfw_id! found at !glfw_library!.
	echo:
	
	set defines=!defines! /D"_GLFW_WIN32"
	set includes=!includes! /I"!glfw_folder!\include"
	set libraries=!libraries! !glfw_library!
) else (
	echo Error! Some error occured while compiling GLFW. We are unable to proceed compilation.
	exit /b
)

rem
rem ImGui
rem

echo Searching for !imgui_id!...
if not exist !imgui_library! (
	echo Compiling ImGui...

	set imgui_source_files=^
		"!imgui_folder!/imgui.cpp"^
		"!imgui_folder!/imgui_draw.cpp" ^
		"!imgui_folder!/imgui_tables.cpp" ^
		"!imgui_folder!/imgui_widgets.cpp" ^
		^
		"!imgui_folder!/backends/imgui_impl_glfw.cpp" ^
		"!imgui_folder!/backends/imgui_impl_opengl3.cpp"

	if exist !imgui_build_folder! (
		del /S /Q !imgui_build_folder!\* 1>nul
	) else (
		mkdir !imgui_build_folder!
	)
	
	for %%f in (!imgui_source_files!) do ( 
 	  cl /nologo /c /O2 /I. /I!imgui_folder! /I!glfw_folder!/include /I"!paintbox_folder!\include" /D IMGUI_USER_CONFIG=\"my_imgui_config.h\" %%f /Fo"!imgui_build_folder!\%%~nf.obj"
		if errorlevel 1 (
			echo:
			echo Compilation error! Stopping...
			exit /b
		)
	)

	lib /nologo /out:!imgui_library! !imgui_build_folder!\*.obj	
)

if exist !imgui_library! (
	echo Success^^! !imgui_id! found at !imgui_library!.
	echo:
	set includes=!includes! /I"!imgui_folder!"
	set libraries=!libraries! !imgui_library!
) else (
	echo Error! Some error occured while compiling ImGui. We are unable to proceed compilation.
	exit /b
)

rem
rem Glad
rem

echo Searching for !glad_id!...
if not exist !glad_library! (
	echo Compiling Glad...

	set glad_source_files=!glad_folder!\src\gl.c

	if exist !glad_build_folder! (
		del /S /Q !glad_build_folder!\* 1>nul
	) else (
		mkdir !glad_build_folder!
	)
	
	for %%f in (!glad_source_files!) do ( 
 	  cl /nologo /c /O2 /I!glad_folder!/include %%f /Fo"!glad_build_folder!\%%~nf.obj"
		if errorlevel 1 (
			echo:
			echo Compilation error! Stopping...
			exit /b
		)
	)

	lib /nologo /out:!glad_library! !glad_build_folder!\*.obj	
)

if exist !glad_library! (
	echo Success^^! !glad_id! found at !glad_library!.
	echo:
	set includes=!includes! /I"!glad_folder!\include"
	set libraries=!libraries! !glad_library!
) else (
	echo Error! Some error occured while compiling ImGui. We are unable to proceed compilation.
	exit /b
)

rem
rem Paintbox
rem

echo Checking for changes in !paintbox_id!...

if exist !paintbox_build_folder! (
	del /S /Q !paintbox_build_folder!\* 1>nul
) else (
	mkdir !paintbox_build_folder!
)

for %%i in (!paintbox_folder!\implementation\*.cpp) do (
   cl /nologo /c /O2 /I!paintbox_folder!/include !includes! %%~fi /Fo"!paintbox_build_folder!\%%~ni.obj"
	if errorlevel 1 (
		echo:
		echo Compilation error! Stopping...
		exit /b
	)
)

lib /nologo /out:!paintbox_library! !paintbox_build_folder!\*.obj	

if exist !paintbox_library! (
	echo Success^^! !paintbox_id! compiled at !paintbox_library!.
	echo:
	set includes=!includes! /I"!paintbox_folder!\include"
	set libraries=!libraries! !paintbox_library!
) else (
	echo Error! Some error occured while compiling ImGui. We are unable to proceed compilation.
	exit /b
)


rem 
rem Examples
rem 

echo Building examples...

set includes=!includes! /Ithird_party

for /r %%i in (example_*.cpp) do (
	rem set source_files=!source_files! %%~fi

	set object_file="!build_folder!\%%~ni.obj"
	set executable="!build_folder!\%%~ni.exe"

	cl /c /nologo /Zi !includes! !defines! %%~fi /Fo!object_file!
	if errorlevel 1 (
		exit /b
	)
	
	link /nologo /DEBUG /INCREMENTAL:NO !object_file! !libraries! /out:!executable!
	
	rem Will we ever need the object file?
	del !object_file! 

	echo:
	if errorlevel 0 (
		echo Example "%%~ni" written to !executable!.
	) else (
		echo Error^^!
	)
)
	

