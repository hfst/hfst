#!/bin/sh
#
# cibuildwheel "before-all" step for Linux (runs inside the manylinux container).
#
# OpenFst and foma are not packaged in the manylinux (AlmaLinux/CentOS) images,
# so build them from the same sources HFST uses for its WASM/Debian builds:
#   * OpenFst : TinoDidriksen's fork (the apertium/projectjj variant)
#   * foma    : mainline (mhulden), built with CMake
# ICU and readline ARE available from yum, so install those as packages.
# Everything installs to /usr/local; auditwheel later vendors the shared libs
# into the wheel.
#
# Pin these to keep wheel builds reproducible; bump deliberately.
OPENFST_REPO="https://github.com/TinoDidriksen/openfst.git"
OPENFST_REF="main"
FOMA_REPO="https://github.com/mhulden/foma.git"
FOMA_REF="v0.10.0"

set -ex

PROJECT_DIR="${1:-$(pwd)}"
BUILD_DIR="$(mktemp -d)"

# Clone with retries (container networking can be flaky).
git_clone() {
    n=0
    until [ "$n" -ge 5 ]; do
        git clone --depth 1 --branch "$1" "$2" "$3" && return 0
        n=$((n + 1))
        echo "git clone failed (attempt $n), retrying..." >&2
        rm -rf "$3"
        sleep 5
    done
    return 1
}

# ----- system build dependencies -----
if command -v yum >/dev/null 2>&1; then
    yum install -y bison flex cmake libtool libicu-devel readline-devel zlib-devel git
elif command -v apk >/dev/null 2>&1; then   # musllinux
    apk add --no-cache bison flex cmake libtool icu-dev readline-dev zlib-dev git build-base
fi

# ----- OpenFst -----
git_clone "$OPENFST_REF" "$OPENFST_REPO" "$BUILD_DIR/openfst"
cd "$BUILD_DIR/openfst"
[ -x ./configure ] || ./autogen.sh || autoreconf -fvi
./configure --prefix=/usr/local --enable-static --disable-shared --with-pic \
    CXXFLAGS="-fPIC -O2 -std=c++17"
make -j"$(nproc)"
make install

# ----- foma -----
git_clone "$FOMA_REF" "$FOMA_REPO" "$BUILD_DIR/foma"
cd "$BUILD_DIR/foma"
# The CMake project lives in the foma/ subdirectory.
FOMA_SRC="$BUILD_DIR/foma"
[ -f "$FOMA_SRC/CMakeLists.txt" ] || FOMA_SRC="$BUILD_DIR/foma/foma"
cmake -S "$FOMA_SRC" -B "$BUILD_DIR/foma-build" \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR/foma-build" -j"$(nproc)"
cmake --install "$BUILD_DIR/foma-build"

# ----- generate flex/bison parser sources for libhfst -----
cd "$PROJECT_DIR"
sh python/generate-parsers.sh "$PROJECT_DIR/libhfst/src/parsers"

ldconfig || true
