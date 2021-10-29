
CFLAGS := -Wall -Wextra -O2 -ggdb3 -fsanitize=address -I..
CXXFLAGS := -Wall -Wextra -O2 -ggdb3 -fsanitize=address -I..
LIB_LDFLAGS := -lasan -lmysqlclient

CXX_TESTS := \
	prepared_statement \
# 	query_fetch

all: $(CXX_TESTS:=.test)

runtests: $(CXX_TESTS:=__do_test)


define GEN_TEST

$(1:=.o): $(1:=.cpp)
	$(CXX) $(CXXFLAGS) -c -o $(1:=.o) $(1:=.cpp)

$(1:=.test): $(1:=.o) ../MySQL.o
	$(CXX) $(CXXFLAGS) -o $(1:=.test) $(1:=.o) ../MySQL.o $(LIB_LDFLAGS)

endef

$(foreach i, $(CXX_TESTS), $(eval $(call GEN_TEST,$(i))))

$(CXX_TESTS:=__do_test): $(CXX_TESTS:=.test)
	@echo "Running $(@:%__do_test=%.test)...";
	@./$(@:%__do_test=%.test);

clean:
	@rm -vf $(CXX_TESTS:=.test) $(CXX_TESTS:=.o)

.PHONY: all clean runtests $(CXX_TESTS:=__do_test)
