#!/bin/sh
#
# cibuildwheel "before-build" step for Pyodide.
#
# cibuildwheel has already selected the Pyodide xbuildenv and put Emscripten
# tools on PATH when this hook runs. This script only builds the native HFST
# dependencies for WebAssembly. It follows the libhfst/wasm-demo/build-wasm.sh
# dependency strategy: foma, OpenFst and ICU are built with Emscripten; SFST
# and readline stay disabled for Pyodide.

set -eux

PROJECT_DIR="${1:-$(pwd)}"
DEPS_DIR="${HFST_PYODIDE_DEPS_DIR:-${PROJECT_DIR}/wasm-deps/pyodide}"
BUILD_DIR="${DEPS_DIR}/build"
INCLUDE_DIR="${DEPS_DIR}/include"
LIB_DIR="${DEPS_DIR}/lib"
TOOLCHAIN_MARKER="${DEPS_DIR}/.pyodide-toolchain"

OPENFST_REPO="${OPENFST_REPO:-https://github.com/TinoDidriksen/openfst.git}"
OPENFST_REF="${OPENFST_REF:-main}"
FOMA_REPO="${FOMA_REPO:-https://github.com/mhulden/foma.git}"
FOMA_REF="${FOMA_REF:-v0.10.0}"
ICU_RELEASE="${ICU_RELEASE:-release-77-1}"
ICU_BASENAME="${ICU_BASENAME:-icu4c-77_1}"
ICU_SRC_URL="${ICU_SRC_URL:-https://github.com/unicode-org/icu/releases/download/${ICU_RELEASE}/${ICU_BASENAME}-src.tgz}"
ICU_DATA_URL="${ICU_DATA_URL:-https://github.com/unicode-org/icu/releases/download/${ICU_RELEASE}/${ICU_BASENAME}-data.zip}"

OPTIM_FLAGS="${HFST_PYODIDE_OPTIM_FLAGS:--Oz}"
OPTIM_LD_FLAGS="${HFST_PYODIDE_OPTIM_LD_FLAGS:--Oz}"

command -v emcc >/dev/null 2>&1
command -v emconfigure >/dev/null 2>&1
command -v emcmake >/dev/null 2>&1
command -v emmake >/dev/null 2>&1

TOOLCHAIN_ID="PYODIDE_ROOT=${PYODIDE_ROOT:-}
EMCC=$(emcc --version | sed -n '1p')"

if [ -f "$TOOLCHAIN_MARKER" ] && [ "$(cat "$TOOLCHAIN_MARKER")" != "$TOOLCHAIN_ID" ]; then
    rm -rf "$BUILD_DIR" "$INCLUDE_DIR" "$LIB_DIR"
fi

mkdir -p "$BUILD_DIR" "$INCLUDE_DIR" "$LIB_DIR"
printf '%s\n' "$TOOLCHAIN_ID" > "$TOOLCHAIN_MARKER"

if command -v nproc >/dev/null 2>&1; then
    NPROC="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
    NPROC="$(sysctl -n hw.ncpu)"
else
    NPROC=2
fi
if [ "$NPROC" -gt 1 ]; then
    NPROC=$((NPROC - 1))
fi

maybe_sudo() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        "$@"
    fi
}

install_host_packages() {
    if command -v apt-get >/dev/null 2>&1; then
        maybe_sudo apt-get -qy update
        maybe_sudo apt-get -qy install --no-install-recommends \
            autoconf \
            automake \
            bison \
            build-essential \
            cmake \
            curl \
            default-jre \
            flex \
            git \
            libtool \
            pkg-config \
            unzip \
            zlib1g-dev
    elif command -v brew >/dev/null 2>&1; then
        brew install autoconf automake bison cmake curl flex git libtool pkg-config unzip
        BREW_PREFIX="$(brew --prefix)"
        export PATH="$BREW_PREFIX/opt/bison/bin:$BREW_PREFIX/opt/flex/bin:$PATH"
    fi
}

git_clone() {
    ref="$1"
    repo="$2"
    dest="$3"
    n=0
    until [ "$n" -ge 5 ]; do
        git clone --depth 1 --branch "$ref" "$repo" "$dest" && return 0
        n=$((n + 1))
        echo "git clone failed (attempt $n), retrying..." >&2
        rm -rf "$dest"
        sleep 5
    done
    return 1
}

build_foma() {
    if [ -f "$LIB_DIR/libfoma.a" ]; then
        echo "foma already built for this Pyodide toolchain; skipping."
        return
    fi

    rm -rf "$BUILD_DIR/foma" "$BUILD_DIR/foma-build"
    git_clone "$FOMA_REF" "$FOMA_REPO" "$BUILD_DIR/foma"

    FOMA_SRC="$BUILD_DIR/foma"
    [ -f "$FOMA_SRC/CMakeLists.txt" ] || FOMA_SRC="$BUILD_DIR/foma/foma"

    CFLAGS="$OPTIM_FLAGS" \
    CXXFLAGS="$OPTIM_FLAGS" \
    LDFLAGS="$OPTIM_LD_FLAGS" \
    emcmake cmake -S "$FOMA_SRC" -B "$BUILD_DIR/foma-build" \
        -DCMAKE_INSTALL_PREFIX="$DEPS_DIR" \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR/foma-build" -j"$NPROC"
    cmake --install "$BUILD_DIR/foma-build" || true

    if [ ! -f "$LIB_DIR/libfoma.a" ]; then
        found_lib="$(find "$BUILD_DIR/foma-build" "$BUILD_DIR/foma" -name libfoma.a -print -quit)"
        [ -n "$found_lib" ]
        cp "$found_lib" "$LIB_DIR/libfoma.a"
    fi
}

build_openfst() {
    if [ -f "$LIB_DIR/libfst.a" ] && [ -d "$INCLUDE_DIR/fst" ]; then
        echo "OpenFst already built for this Pyodide toolchain; skipping."
        return
    fi

    rm -rf "$BUILD_DIR/openfst"
    git_clone "$OPENFST_REF" "$OPENFST_REPO" "$BUILD_DIR/openfst"
    cd "$BUILD_DIR/openfst"
    [ -x ./configure ] || ./autogen.sh || autoreconf -fvi

    CFLAGS="$OPTIM_FLAGS" \
    CXXFLAGS="$OPTIM_FLAGS -std=c++17" \
    LDFLAGS="$OPTIM_LD_FLAGS" \
    emconfigure ./configure \
        --prefix="$DEPS_DIR" \
        --enable-static \
        --disable-shared \
        --with-pic
    emmake make
    emmake make install

    if [ ! -f "$LIB_DIR/libfst.a" ]; then
        cp src/lib/.libs/libfst.a "$LIB_DIR/libfst.a"
    fi
    if [ ! -d "$INCLUDE_DIR/fst" ]; then
        cp -R src/include/fst "$INCLUDE_DIR/fst"
    fi
}

build_icu() {
    if [ -f "$LIB_DIR/libicudata.a" ]; then
        echo "ICU already built for this Pyodide toolchain; skipping."
        return
    fi

    cd "$BUILD_DIR"
    if [ ! -d icu ]; then
        curl -L --retry 5 -o "${ICU_BASENAME}-src.tgz" "$ICU_SRC_URL"
        tar -xzf "${ICU_BASENAME}-src.tgz"
        for dat in icu/source/data/in/icudt??l.dat; do
            [ -f "$dat" ] && mv "$dat" "$dat.hidden"
        done
        cp icu/source/config/mh-linux icu/source/config/mh-unknown
    fi

    if [ ! -f icu/source/data/locales/root.txt ]; then
        curl -L --retry 5 -o "${ICU_BASENAME}-data.zip" "$ICU_DATA_URL"
        mv icu/source/data icu/source/data_from_src_tgz
        unzip -q "${ICU_BASENAME}-data.zip" -d icu/source/
    fi

    ICU_DATA_FILTER_FILE="$BUILD_DIR/icu-data-filter.json"
    export ICU_DATA_FILTER_FILE
    cat > "$ICU_DATA_FILTER_FILE" <<'EOF'
{
  "strategy": "additive",
  "localeFilter": {
    "filterType": "language",
    "includelist": ["en"]
  },
  "featureFilters": {
    "brkitr_rules": "include",
    "brkitr_tree": "include",
    "cnvalias": "include",
    "ulayout": "include",
    "uemoji": "include",
    "brkitr_dictionaries": "include",
    "brkitr_lstm": "include",
    "brkitr_adaboost": "include"
  }
}
EOF

    cd "$BUILD_DIR/icu/source"
    mkdir -p native-build
    cd native-build
    NATIVE_ICU_DIR="$(pwd)"
    if [ ! -f bin/pkgdata ]; then
        ../configure --disable-shared --enable-static
        make -j"$NPROC"
    fi

    cd "$BUILD_DIR/icu/source"
    mkdir -p wasm-build
    cd wasm-build
    export PKGDATA_OPTS="--without-assembly -O ../data/icupkg.inc"
    PKG_CONFIG_LIBDIR="$LIB_DIR/pkgconfig" \
    CFLAGS="$OPTIM_FLAGS" \
    CXXFLAGS="$OPTIM_FLAGS" \
    LDFLAGS="$OPTIM_LD_FLAGS" \
    emconfigure ../configure \
        --host=wasm32-unknown-emscripten \
        --prefix="$DEPS_DIR" \
        --with-cross-build="$NATIVE_ICU_DIR" \
        --enable-static=yes \
        --disable-shared \
        --with-data-packaging=static \
        --enable-icu-config \
        --disable-extras \
        --disable-samples \
        --disable-tests

    CFLAGS="$OPTIM_FLAGS" \
    CXXFLAGS="$OPTIM_FLAGS" \
    LDFLAGS="$OPTIM_LD_FLAGS" \
    emmake make -j"$NPROC" install
}

install_host_packages
build_foma
build_openfst
build_icu

cd "$PROJECT_DIR"
sh python/generate-parsers.sh "$PROJECT_DIR/libhfst/src/parsers"
