# SPDX-License-Identfier: GPL-2.0-only
#
# Copyright (C)  2021 Ammar Faizi <ammarfaizi2@gmail.com>
#

CFLAGS := -Wall -Wextra -O3 -ggdb3 -I.. -fsanitize=address -fno-omit-frame-pointer
CXXFLAGS := -Wall -Wextra -O3 -ggdb3 -I.. -fsanitize=address -fno-omit-frame-pointer
LIB_LDFLAGS := -lasan -lmysqlclient

ifeq ($(CC),clang)
CFLAGS += -shared-libsan
endif

ifeq ($(CXX),clang++)
CXXFLAGS += -shared-libsan
endif


CXX_TESTS := \
	prepared_statement \
	query_fetch

TEST_LD_ENV = \
	LD_PRELOAD="$(shell $(CC) -print-file-name=libasan.so)" \
	LD_LIBRARY_PATH="$(shell pwd)"

all: $(CXX_TESTS:=.test)


runtests: $(CXX_TESTS:=__do_test)


libclang_rt.asan-x86_64.so:
	ln -sv $(shell $(CC) -print-file-name=libasan.so) $(@)


define GEN_TEST

$(1:=.o): $(1:=.cpp)
	$(CXX) $(CXXFLAGS) -c -o $(1:=.o) $(1:=.cpp)

$(1:=.test): $(1:=.o) ../MySQL.o
	$(CXX) $(CXXFLAGS) -o $(1:=.test) $(1:=.o) ../MySQL.o $(LIB_LDFLAGS)

$(1:=__do_test): $(1:=.test) libclang_rt.asan-x86_64.so
	@echo "Running $(1:=.test)...";
	@$(TEST_LD_ENV) ./$(1:=.test) && echo "Test $(1:=.test) passed!";

endef

$(foreach i, $(CXX_TESTS), $(eval $(call GEN_TEST,$(i))))

clean:
	@rm -vf $(CXX_TESTS:=.test) $(CXX_TESTS:=.o) libclang_rt.asan-x86_64.so

.PHONY: all clean runtests $(CXX_TESTS:=__do_test)
