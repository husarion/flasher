cmake_minimum_required(VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(flasher_hard C CXX ASM)

if(CROSS)
	set(CMAKE_C_COMPILER "i686-w64-mingw32-gcc")
	set(CMAKE_CXX_COMPILER "i686-w64-mingw32-g++")
endif()

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

include_directories(${CURRENT_DIR})

if(WIN32)
	set(PROJECT_PORT_DIR ${CURRENT_DIR}/port/windows)
	set(CMAKE_C_FLAGS "-DPLATFORM_WIN32 -D_CRT_SECURE_NO_WARNINGS -std=c99 -Wall")
	set(CMAKE_CXX_FLAGS "-DPLATFORM_WIN32 -D_CRT_SECURE_NO_WARNINGS -std=c++11 -Wall")

	include_directories(${CURRENT_DIR}/port/windows/)
	link_directories(${CURRENT_DIR}/port/windows/)
elseif(UNIX)
	set(PROJECT_PORT_DIR ${CURRENT_DIR}/port/linux)
	set(CMAKE_C_FLAGS "-DPLATFORM_LINUX -std=c99 -Wall")
	set(CMAKE_CXX_FLAGS "-DPLATFORM_LINUX -std=c++11 -Wall")

	include_directories(${CURRENT_DIR}/port/linux/ /usr/include/libusb-1.0/)
endif()
include_directories(${CURRENT_DIR}/include)

set(COMMON_SOURCES src/main.cpp src/myFTDI.cpp src/devices.cpp src/ihex.cpp src/xcpmaster.cpp
	src/HardFlasher.cpp src/utils.cpp src/TRoboCOREHeader.cpp
	src/console.cpp
	${PROJECT_PORT_DIR}/xcptransport.cpp ${PROJECT_PORT_DIR}/timeutil.cpp)

if(EMBED_BOOTLOADERS)
	set(COMMON_SOURCES ${COMMON_SOURCES} gen/bootloaders.cpp)
	include_directories(${CURRENT_DIR}/gen)
	add_definitions(-DEMBED_BOOTLOADERS)
endif()

if(WIN32)
	if(CROSS)
		set(TARGET_NAME core2-flasher.exe)
	else()
		set(TARGET_NAME core2-flasher)
	endif()

	add_executable(${TARGET_NAME} ${COMMON_SOURCES})

	target_link_libraries(${TARGET_NAME}
		ftdi1 usb-1.0 pthread ws2_32)

	add_definitions(-DWIN32)

	add_custom_target(copy ALL COMMENT "Copying files...")
	add_custom_command(TARGET copy
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/port/windows/libftdi1.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	add_custom_command(TARGET copy
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/port/windows/libusb-1.0.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

	if(CROSS)
		add_custom_command(TARGET copy
			COMMAND ${CMAKE_COMMAND} -E copy /usr/lib/gcc/i686-w64-mingw32/5.3-posix/libstdc++-6.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
		add_custom_command(TARGET copy
			COMMAND ${CMAKE_COMMAND} -E copy /usr/lib/gcc/i686-w64-mingw32/5.3-posix/libgcc_s_sjlj-1.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
		add_custom_command(TARGET copy
			COMMAND ${CMAKE_COMMAND} -E copy /usr/i686-w64-mingw32/lib/libwinpthread-1.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	endif()
elseif(UNIX)
	set(TARGET_NAME core2-flasher)
	add_definitions(-DUNIX)

	add_executable(${TARGET_NAME}
		${COMMON_SOURCES}
		port/linux/ftdi.c port/linux/ftdi_stream.c)

	target_link_libraries(${TARGET_NAME}
		usb-1.0 pthread)
endif()

option(X86 "32 bit executable" OFF)

if(X86)
  if(MULTILIB_HACK)
	# gcc-multilib conficts with cross-compilers on old Debians
	set_target_properties(${TARGET_NAME} PROPERTIES
	  COMPILE_FLAGS "-m32" LINK_FLAGS "-m32"
	  "-B" "/usr/lib/gcc/${MULTILIB_HACK}" "-L" "/usr/lib/gcc/${MULTILIB_HACK}")
  else()
	set_target_properties(${TARGET_NAME} PROPERTIES
	  COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
  endif()
endif()

if(DEBUG)
  set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "-g" LINK_FLAGS "-g")
endif()
