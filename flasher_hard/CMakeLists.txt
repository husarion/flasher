cmake_minimum_required (VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(flasher_hard C CXX ASM)

set(debug_flags "-DHFRAMEWORK_COMPILE -DSYSTEM_DEBUG -DUSB_DEBUG -DUSB_ADK_DEBUG -DUSB_CORE_DEBUG")

set(common_flags "")

set(include_flags "-I ${CMAKE_SOURCE_DIR}/include/")
set(lib_flags "-L libftdi/")

set(CMAKE_CXX_FLAGS "${include_flags} ${common_flags}")
set(CMAKE_C_FLAGS "${include_flags} ${common_flags} -std=c99")
SET(CMAKE_ASM_FLAGS "${common_flags}")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

add_executable(flasher_hard
	main.cpp uart.cpp devices.cpp proto.cpp cmds.cpp)

target_link_libraries(flasher_hard
	ftdi1)
