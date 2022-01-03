# SPDX-License-Identifier: GPL-2.0-only

cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(tgvisd VERSION 1.0 LANGUAGES C CXX)

list(APPEND CMAKE_PREFIX_PATH "tdlib/lib/cmake/Td")

find_package(Td 1.8.0 REQUIRED)


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
	-fsanitize=address
	-Wno-gnu-statement-expression
)
##################################################################






##################################################################
#
# Main executable of tgvisd
#


set(
	TGVISD_SOURCE

	Logger/ChatFoundation.cpp
	Logger/ChatFoundation.hpp
	Logger/Chat/Group.cpp
	Logger/Chat/Group.hpp
	Logger/Chat/User.cpp
	Logger/Chat/User.hpp
	Logger/Sender/Chat.hpp
	Logger/Sender/Chat.cpp
	Logger/Sender/User.hpp
	Logger/Sender/User.cpp
	Logger/Message.cpp
	Logger/Message.hpp
	Logger/SenderFoundation.cpp
	Logger/SenderFoundation.hpp

	../mysql/MySQL.cpp
	../mysql/MySQL.hpp
	common.hpp
	entry.cpp
	Main.cpp
	Main.hpp
	mysql_helpers.hpp
	mysql_helpers.cpp
	print.c
	print.h
	Scraper.cpp
	Scraper.hpp
	KWorker.cpp
	KWorker.hpp
)

add_executable(tgvisd ${TGVISD_SOURCE})
set_property(TARGET tgvisd PROPERTY CXX_STANDARD 20)
target_link_libraries(tgvisd PRIVATE asan tgvisdtd mysqlclient pthread)
target_compile_options(tgvisd PRIVATE
	-ggdb3
	-Wall
	-Wextra
	-Wpedantic
	-Wno-unused-parameter
	-fno-omit-frame-pointer
	-fstack-protector-strong
	-fsanitize=address
	-Wno-gnu-statement-expression
)
##################################################################
