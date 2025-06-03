#!/bin/bash


# This script builds the HFST library for WebAssembly using Emscripten.
# It is intended to be run from the root dir of the repository in a fresh
# Debian/Ubuntu container.

set -e

if ! [ -d libhfst ]; then
    echo "Please run this script from the HFST root directory."
    exit 1
fi

# Check if we are running in a Docker container
if [ -f /.dockerenv ]; then
    echo "Running in a Docker container. Proceeding with build."
else
    echo "This script is intended to be run in a Debian/Ubuntu container."
    read -p "Do you want to continue? (y/n) " answer
    if [[ ! $answer =~ ^[Yy]$ ]]; then
        echo "Exiting."
        exit 1
    fi
fi

HFST_DIR="$(pwd)"
DEPS_DIR="${HFST_DIR}/wasm-deps"
INCLUDE_DIR="${DEPS_DIR}/include"
LIB_DIR="${DEPS_DIR}/lib"
mkdir -p ${INCLUDE_DIR}
mkdir -p ${LIB_DIR}

OPTIM_FLAGS="-Oz"
OPTIM_LD_FLAGS="-Oz --closure 1"

NPROC=$(nproc)
if [ ${NPROC} -gt 1 ]; then
    NPROC=$((${NPROC} - 1))
fi

APT_UPDATE_STAMP="/tmp/apt-update-stamp"
if [ ! -f ${APT_UPDATE_STAMP} ]; then
    sudo apt-get update
    touch ${APT_UPDATE_STAMP}
else
    echo "apt-get update already done. Skipping..."
fi
sudo apt-get install -y \
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
    python3 \
    python3-jsonschema \
    unzip \
    zlib1g-dev

cd ${DEPS_DIR}
if ! [ -d emsdk ]; then
    git clone https://github.com/emscripten-core/emsdk.git
fi
if [ -z $(which emcc) ]; then
    pushd emsdk
    ./emsdk install latest
    ./emsdk activate latest
    . emsdk_env.sh
    popd
fi

FOMA_HEADING='
=======================================================================
====================== Building Foma ==================================
======================================================================='
cd ${DEPS_DIR}
if [ -f "${LIB_DIR}/libfoma.a" ]; then
  echo "Foma already built (wasm-deps/libfoma.a exists), skipping..."
else
  echo "${FOMA_HEADING}"
  [ -d foma ] || git clone https://github.com/mhulden/foma.git
  cd foma/foma
  git pull
  CFLAGS="${OPTIM_FLAGS}" CXXFLAGS="${OPTIM_FLAGS}" LDFLAGS="${OPTIM_LD_FLAGS}" emcmake cmake .
  CFLAGS="${OPTIM_FLAGS}" CXXFLAGS="${OPTIM_FLAGS}" LDFLAGS="${OPTIM_LD_FLAGS}" emmake make -j"${NPROC}"
  cp libfoma.a ${LIB_DIR}/
fi


OPENFST_HEADING='
=======================================================================
====================== Building OpenFST ===============================
======================================================================='
cd ${DEPS_DIR}
if [ -f "${LIB_DIR}/libfst.a" ]; then
  echo "OpenFst already built (wasm-deps/libfst.a exists), skipping..."
else
  echo "${OPENFST_HEADING}"
  [ -d openfst ] || git clone https://github.com/TinoDidriksen/openfst.git
  cd openfst
  autoreconf -fiv
  CFLAGS="${OPTIM_FLAGS}" CXXFLAGS="${OPTIM_FLAGS}" LDFLAGS="${OPTIM_LD_FLAGS}" emconfigure ./configure --enable-static --disable-shared  # TODO more flags needed for libhfst?
  CFLAGS="${OPTIM_FLAGS}" CXXFLAGS="${OPTIM_FLAGS}" LDFLAGS="${OPTIM_LD_FLAGS}" emmake make # -j"${NPROC}" multithreaded build fails
  cp src/lib/.libs/libfst.a ${LIB_DIR}/
  [ -d ${DEPS_DIR}/include/fst ] || ln -s ${DEPS_DIR}/openfst/src/include/fst ${DEPS_DIR}/include/
fi

ICU_HEADING='
=======================================================================
====================== Building ICU ______ ============================
======================================================================='
cd ${DEPS_DIR}
# ICU_SRC_URL="https://github.com/unicode-org/icu/releases/download/release-77-1/icu4c-77_1-src.tgz"
ICU_SRC_URL=$(curl -s https://api.github.com/repos/unicode-org/icu/releases/latest | grep '/icu4c-.*-src.tgz"' | cut -d '"' -f 4)
ICU_SRC_TGZ=$(basename ${ICU_SRC_URL})
ICU_DATA_URL=${ICU_SRC_URL/-src.tgz/-data.zip}
ICU_DATA_ZIP=$(basename ${ICU_DATA_URL})

if ! [ -d icu ]; then
    curl -L -o "${ICU_SRC_TGZ}" "${ICU_SRC_URL}"
    tar -xvf "${ICU_SRC_TGZ}"
    # Hide dat file to force make to rebuild data using filter
    ORIG_DAT="$(pwd)/icu/source/data/in/icudt??l.dat"
    if [ -f ${ORIG_DAT} ]; then
        mv ${ORIG_DAT} ${ORIG_DAT}.hidden
    fi
    # Let config assume that emscripten build is Linux
    cp icu/source/config/mh-linux icu/source/config/mh-unknown
fi

if ! [ -f icu/source/data/locales/root.txt ]; then
    curl -L -o "${ICU_DATA_ZIP}" "${ICU_DATA_URL}"
    mv icu/source/data icu/source/data_from_src_tgz
    unzip "${ICU_DATA_ZIP}" -d icu/source/
fi

export ICU_DATA_FILTER_FILE="$(pwd)/data-filter.json"
echo '{
  "strategy": "additive",

  "localeFilter": {
    "filterType": "language",
    "includelist": ["en"]
  },

  "featureFilters": {
    "brkitr_rules":        "include",
    "brkitr_tree":         "include",

    "cnvalias":            "include",
    "ulayout":             "include",
    "uemoji":              "include",

    "brkitr_dictionaries": "include",
    "brkitr_lstm":         "include",
    "brkitr_adaboost":     "include"
  }
}' > ${ICU_DATA_FILTER_FILE}

pushd icu/source

# Build ICU natively (host build) to generate required tools and ICU data
echo '=================== Building ICU (native tools) ======================='
mkdir -p native-build
pushd native-build
NATIVE_ICU_DIR=$(pwd)
if [ -f "bin/pkgdata" ]; then
  echo "ICU native tools already built (native-build/bin/pkgdata exists), skipping..."
else
  echo "${ICU_HEADING/______/native}"
  ../configure --disable-shared --enable-static VERBOSE=1
  make -j"${NPROC}" VERBOSE=1
fi
popd

echo '====================== Building ICU (wasm) ==========================='
mkdir -p wasm-build
pushd wasm-build
if [ -f "lib/libicudata.a" ]; then
  echo "ICU wasm libs already built (wasm-build/lib/libicudata.a exists), skipping..."
else
  echo "${ICU_HEADING/______/ wasm }"
  export PKGDATA_OPTS="--without-assembly -O ../data/icupkg.inc"
  PKG_CONFIG_LIBDIR="${LIB_DIR}/pkgconfig" \
  CFLAGS="${OPTIM_FLAGS}" CXXFLAGS="${OPTIM_FLAGS}" LDFLAGS="${OPTIM_LD_FLAGS}" \
  emconfigure ../configure \
      --host=wasm32-unknown-emscripten \
      --prefix="${DEPS_DIR}" \
      --with-cross-build="${NATIVE_ICU_DIR}" \
      --enable-static=yes \
      --disable-shared \
      --with-data-packaging=static \
      --enable-icu-config \
      --disable-extras \
      --disable-samples \
      --disable-tests \
      VERBOSE=1

  CFLAGS="${OPTIM_FLAGS}" CXXFLAGS="${OPTIM_FLAGS}" LDFLAGS="${OPTIM_LD_FLAGS}" emmake make -j"${NPROC}" install
  popd
  popd
echo "WebAssembly ICU build complete. Static libraries are in ${LIB_DIR}"
fi


echo '
=======================================================================
====================== Building LibHFST ===============================
======================================================================='
cd ${HFST_DIR}

if [ ! -f configure ]; then
    ./autogen.sh
fi

autoreconf -fiv

emconfigure ./configure \
  --host=wasm32-unknown-emscripten \
  --with-sfst=no \
  --with-foma=yes \
  --with-openfst=yes \
  --enable-no-tools \
  --disable-load-so-entries \
  --with-readline=no \
  CFLAGS="${OPTIM_FLAGS}" \
  CXXFLAGS="${OPTIM_FLAGS}" \
  CPPFLAGS="-I${DEPS_DIR}/include -I${DEPS_DIR}/foma/foma" \
  FOMA_CFLAGS="-I${DEPS_DIR}/foma/foma" \
  FOMA_LIBS="-L${LIB_DIR} -lfoma" \
  LDFLAGS="${OPTIM_LD_FLAGS} \
           -L${LIB_DIR} \
           -lembind \
           -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS'] \
           -s EXPORTED_FUNCTIONS=['_malloc','_free'] \
           -s ALLOW_MEMORY_GROWTH=1 \
           -s MODULARIZE=1 \
           -s EXPORT_NAME='createHfstModule' \
           -s INITIAL_MEMORY=64MB \
           -s MAXIMUM_MEMORY=4GB" \
  VERBOSE=1

echo "Build libhfst"
CFLAGS="${OPTIM_FLAGS}" CXXFLAGS="${OPTIM_FLAGS}" LDFLAGS="${OPTIM_LD_FLAGS}" emmake make -C libhfst VERBOSE=1 -j"${NPROC}"

echo '
=======================================================================
==================== LibHFST WASM build complete ======================
======================================================================='
