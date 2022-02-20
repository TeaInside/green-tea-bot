
export CC=clang
export CXX=clang++

all: build/tgvisd/tgvisd

build:
	mkdir -pv $(@);

build/tdlib: | build
	+cd td && \
	(mkdir -pv build || true) && \
	cd build && \
	VERBOSE=1 cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=../../build/tdlib .. && \
	VERBOSE=1 cmake --build . -j$(shell nproc) && \
	VERBOSE=1 cmake --build . --target install -j$(shell nproc);

build/tgvisd: tgvisd | build/tdlib
	+cd build && \
	(mkdir -pv tgvisd || true) && \
	cd tgvisd && \
	cmake -DCMAKE_BUILD_TYPE=Release ../../tgvisd

build/tgvisd/tgvisd:
	+cd build/tgvisd && $(MAKE);

clean:
	+cd build/tgvisd && $(MAKE) clean;

.PHONY: all build/tgvisd/tgvisd
