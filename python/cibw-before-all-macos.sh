#!/bin/sh
#
# cibuildwheel "before-all" step for macOS.
#
# Same strategy as the Linux script: build OpenFst + foma from source, get ICU
# and readline from Homebrew. Installs to /usr/local so setup.py's default
# search paths and delocate pick everything up.
#
OPENFST_REPO="https://github.com/TinoDidriksen/openfst.git"
OPENFST_REF="main"
FOMA_REPO="https://github.com/mhulden/foma.git"
FOMA_REF="v0.10.0"

set -ex

PROJECT_DIR="${1:-$(pwd)}"
BUILD_DIR="$(mktemp -d)"

# Clone with retries (networking can be flaky).
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

# ----- Homebrew build dependencies -----
brew install bison flex cmake libtool icu4c readline autoconf automake pkg-config

# Homebrew keeps bison/flex/icu4c keg-only; expose them.
BREW_PREFIX="$(brew --prefix)"
export PATH="$BREW_PREFIX/opt/bison/bin:$BREW_PREFIX/opt/flex/bin:$PATH"
ICU_PREFIX="$(brew --prefix icu4c)"
READLINE_PREFIX="$(brew --prefix readline)"
export PKG_CONFIG_PATH="$ICU_PREFIX/lib/pkgconfig:$READLINE_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
export CPPFLAGS="-I$ICU_PREFIX/include -I$READLINE_PREFIX/include $CPPFLAGS"
export LDFLAGS="-L$ICU_PREFIX/lib -L$READLINE_PREFIX/lib $LDFLAGS"
case "${MACOSX_DEPLOYMENT_TARGET:-}" in
    ""|10.*|11.*|12.*|13.*|14.*) export MACOSX_DEPLOYMENT_TARGET=15.0 ;;
esac

# ----- OpenFst -----
git_clone "$OPENFST_REF" "$OPENFST_REPO" "$BUILD_DIR/openfst"
cd "$BUILD_DIR/openfst"
[ -x ./configure ] || ./autogen.sh || autoreconf -fvi
./configure --prefix=/usr/local --enable-static --disable-shared --with-pic \
    CXXFLAGS="-fPIC -O2 -std=c++17 -mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET"
make -j"$(sysctl -n hw.ncpu)"
sudo make install

# ----- foma -----
git_clone "$FOMA_REF" "$FOMA_REPO" "$BUILD_DIR/foma"
FOMA_SRC="$BUILD_DIR/foma"
[ -f "$FOMA_SRC/CMakeLists.txt" ] || FOMA_SRC="$BUILD_DIR/foma/foma"
cmake -S "$FOMA_SRC" -B "$BUILD_DIR/foma-build" \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR/foma-build" -j"$(sysctl -n hw.ncpu)"
sudo cmake --install "$BUILD_DIR/foma-build"

# ----- generate flex/bison parser sources for libhfst -----
cd "$PROJECT_DIR"
sh python/generate-parsers.sh "$PROJECT_DIR/libhfst/src/parsers"
