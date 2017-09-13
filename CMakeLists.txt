cmake_minimum_required(VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(flasher_hard C CXX ASM)

if(CROSS)
    set(CMAKE_C_COMPILER "i686-w64-mingw32.static-gcc")
    set(CMAKE_CXX_COMPILER "i686-w64-mingw32.static-g++")
endif()

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

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
    set(TARGET_NAME core2-flasher)

	add_executable(${TARGET_NAME} ${COMMON_SOURCES})

	target_link_libraries(${TARGET_NAME}
		ftdi1 usb-1.0 pthread ws2_32)

    add_definitions(-DWIN32)
elseif(UNIX)
	set(TARGET_NAME core2-flasher)
	add_definitions(-DUNIX)

	include_directories(third-party/libusb/libusb)

	add_executable(${TARGET_NAME}
		${COMMON_SOURCES}
		port/linux/ftdi.c port/linux/ftdi_stream.c

        third-party/libusb/libusb/os/linux_netlink.c
        third-party/libusb/libusb/os/linux_usbfs.c
        third-party/libusb/libusb/os/poll_posix.c
        third-party/libusb/libusb/os/threads_posix.c
        third-party/libusb/libusb/core.c
        third-party/libusb/libusb/descriptor.c
        third-party/libusb/libusb/hotplug.c
        third-party/libusb/libusb/io.c
        third-party/libusb/libusb/strerror.c
        third-party/libusb/libusb/sync.c)

	target_link_libraries(${TARGET_NAME}
	  pthread rt)
endif()

option(X86 "32 bit executable" OFF)

if(X86)
  if(MULTILIB_HACK STREQUAL "")
	set_target_properties(${TARGET_NAME} PROPERTIES
	  COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
  else()
	# gcc-multilib conficts with cross-compilers on old Debians
	set_target_properties(${TARGET_NAME} PROPERTIES
	  COMPILE_FLAGS "-m32"
	  LINK_FLAGS "-m32 -B /usr/lib/gcc/${MULTILIB_HACK} -L /usr/lib/gcc/${MULTILIB_HACK}")
  endif()
endif()

if(DEBUG)
  set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "-g" LINK_FLAGS "-g")
endif()
