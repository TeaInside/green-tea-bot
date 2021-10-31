# SPDX-License-Identifier: GPL-2.0-only

cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(tgvisd VERSION 1.0 LANGUAGES C CXX)

list(APPEND CMAKE_PREFIX_PATH "tdlib/lib/cmake/Td")

find_package(Td 1.7.8 REQUIRED)


include_directories(..)
include_directories(tdlib/include)

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
target_link_libraries(tgvisdtd PRIVATE Td::TdStatic pthread)
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
	common.hpp
	entry.cpp
	Main.cpp
	Main.hpp
	print.c
	print.h
	Scraper.cpp
	Scraper.hpp
)

add_executable(tgvisd ${TGVISD_SOURCE})
set_property(TARGET tgvisd PROPERTY CXX_STANDARD 20)
target_link_libraries(tgvisd PRIVATE tgvisdtd pthread)
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
