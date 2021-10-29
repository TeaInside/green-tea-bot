
CFLAGS := -Wall -Wextra -O2 -ggdb3 -fsanitize=address -I..
CXXFLAGS := -Wall -Wextra -O2 -ggdb3 -fsanitize=address -I..
LIB_LDFLAGS := -lasan -lmysqlclient

CXX_TESTS := \
	query_fetch

all: $(CXX_TESTS:=.test)

runtests: $(CXX_TESTS:=__do_test)

$(CXX_TESTS:=.o): $(CXX_TESTS:=.cpp)
	$(CXX) $(CXXFLAGS) $(^) -c -o $(@)

$(CXX_TESTS:=.test): $(CXX_TESTS:=.o) ../MySQL.o
	$(CXX) $(CXXFLAGS) $(^) -o $(@) $(LIB_LDFLAGS)

$(CXX_TESTS:=__do_test): $(CXX_TESTS:=.test)
	./$(@:%__do_test=%.test)

clean:
	@rm -vf $(CXX_TESTS:=.test) $(CXX_TESTS:=.o)

.PHONY: all clean runtests $(CXX_TESTS:=__do_test)
