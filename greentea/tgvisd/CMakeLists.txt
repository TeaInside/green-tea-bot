# SPDX-License-Identifier: GPL-2.0-only

cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(tgvisd VERSION 1.0 LANGUAGES C CXX)

list(APPEND CMAKE_PREFIX_PATH "tdlib/lib/cmake/Td")

find_package(Td 1.7.8 REQUIRED)


##################################################################
#
# TD core wrapper
#


set(
	TGVISDTD_SOURCE
	Td/Td.cpp
	Td/Td.hpp
)

# Build Tdlib wrapper as a shared library.
add_library(tgvisdtd SHARED ${TGVISDTD_SOURCE})
set_property(TARGET tgvisdtd PROPERTY CXX_STANDARD 20)
target_link_libraries(tgvisdtd PRIVATE Td::TdStatic)
target_include_directories(tgvisdtd PRIVATE tdlib/include)
target_compile_options(tgvisdtd PRIVATE
	-ggdb3
	-Wall
	-Wextra
	-Wpedantic
	-Wno-unused-parameter
	-fno-omit-frame-pointer
	-fstack-protector-strong
)
##################################################################






##################################################################
#
# Main executable of tgvisd
#


set(
	TGVISD_SOURCE
	main.cpp
)

add_executable(tgvisd ${TGVISD_SOURCE})
set_property(TARGET tgvisd PROPERTY CXX_STANDARD 20)
target_link_libraries(tgvisd tgvisdtd)
target_compile_options(tgvisd PRIVATE
	-ggdb3
	-Wall
	-Wextra
	-Wpedantic
	-Wno-unused-parameter
	-fno-omit-frame-pointer
	-fstack-protector-strong
)
##################################################################
