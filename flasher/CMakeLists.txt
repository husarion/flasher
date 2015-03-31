cmake_minimum_required (VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(flasher_hard C CXX ASM)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(CROSS)
	set(CMAKE_C_COMPILER "i686-w64-mingw32-gcc")
	set(CMAKE_CXX_COMPILER "i686-w64-mingw32-g++")
endif()

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

include_directories(${CURRENT_DIR})

if(WIN32)
  set(PROJECT_PORT_DIR ${CURRENT_DIR}/port/windows)
  set(CMAKE_C_FLAGS "-DPLATFORM_WIN32 -D_CRT_SECURE_NO_WARNINGS -std=c99")
  set(CMAKE_CXX_FLAGS "-DPLATFORM_WIN32 -D_CRT_SECURE_NO_WARNINGS")

	include_directories(${CURRENT_DIR}/port/windows/)
	link_directories(${CURRENT_DIR}/port/windows/)
elseif(UNIX)
  set(PROJECT_PORT_DIR ${CURRENT_DIR}/port/linux)
  set(CMAKE_C_FLAGS "-DPLATFORM_LINUX -std=c99")
  set(CMAKE_CXX_FLAGS "-DPLATFORM_LINUX")

	include_directories(${CURRENT_DIR}/port/linux/ /usr/include/libusb-1.0/)
endif()

set(COMMON_SOURCES main.cpp myFTDI.cpp devices.cpp ihex.cpp xcpmaster.cpp HardFlasher.cpp Flasher.cpp
	${PROJECT_PORT_DIR}/xcptransport.cpp ${PROJECT_PORT_DIR}/timeutil.cpp)

if(WIN32)
	add_executable(flasher.exe
		${COMMON_SOURCES})

	target_link_libraries(flasher.exe
		ftdi1 usb-1.0)

	add_custom_target(copy ALL COMMENT "Copying files...")
	add_custom_command(TARGET copy
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/port/windows/libftdi1.dll ${CMAKE_SOURCE_DIR}/bin/)
	add_custom_command(TARGET copy
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/port/windows/libusb-1.0.dll ${CMAKE_SOURCE_DIR}/bin/)

	if(CROSS)
		add_custom_command(TARGET copy
			COMMAND ${CMAKE_COMMAND} -E copy /usr/lib/gcc/i686-w64-mingw32/4.8/libstdc++-6.dll ${CMAKE_SOURCE_DIR}/bin/)
		add_custom_command(TARGET copy
			COMMAND ${CMAKE_COMMAND} -E copy /usr/lib/gcc/i686-w64-mingw32/4.8/libgcc_s_sjlj-1.dll ${CMAKE_SOURCE_DIR}/bin/)
		add_custom_command(TARGET copy
			COMMAND ${CMAKE_COMMAND} -E copy /usr/i686-w64-mingw32/lib/libwinpthread-1.dll ${CMAKE_SOURCE_DIR}/bin/)
	endif()
elseif(UNIX)
	add_executable(flasher
		${COMMON_SOURCES}
		port/linux/ftdi.c port/linux/ftdi_stream.c)

	target_link_libraries(flasher
		usb-1.0)
endif()

option(X86 "32 bit executable" OFF)

if(X86)
	set_target_properties(flasher_hard PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif()
