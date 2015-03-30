cmake_minimum_required (VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(flasher_hard C CXX ASM)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(WIN32)
	set(include_flags "-I ${CMAKE_SOURCE_DIR}/port/windows/ -I ../common")
	set(lib_flags "-L  ${CMAKE_SOURCE_DIR}/port/windows/")
else()
	set(include_flags "-I ${CMAKE_SOURCE_DIR}/port/linux/ -I ../common -I /usr/include/libusb-1.0/")
endif()

set(CMAKE_EXE_LINKER_FLAGS "${lib_flags}")
set(CMAKE_CXX_FLAGS "${include_flags} ${common_flags}")
set(CMAKE_C_FLAGS "${include_flags} ${common_flags} -std=c99")
SET(CMAKE_ASM_FLAGS "${common_flags}")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")


if(WIN32)
	add_executable(flasher_hard
		main.cpp uart.cpp proto.cpp cmds.cpp devices.cpp ../common/ihex.cpp)

	target_link_libraries(flasher_hard
		ftdi1 usb-1.0)

	add_custom_target(copy ALL COMMENT "Copying files...")
	add_custom_command(TARGET copy
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/port/windows/libftdi1.dll ${CMAKE_SOURCE_DIR}/bin/)
	add_custom_command(TARGET copy
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/port/windows/libusb-1.0.dll ${CMAKE_SOURCE_DIR}/bin/)
else()
	add_executable(flasher_hard
		main.cpp uart.cpp proto.cpp cmds.cpp devices.cpp ../common/ihex.cpp
		port/linux/ftdi.c port/linux/ftdi_stream.c)

	target_link_libraries(flasher_hard
		usb-1.0)
endif()

option(X86 "32 bit executable" OFF)

if(X86)
	set_target_properties(flasher_hard PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif()
