#!/bin/bash

# This script cleans the build artifacts for HFST WebAssembly dependencies and libhfst.
# It is intended to be run from the root dir of the repository.

set -e

if ! [ -d libhfst ]; then
    echo "Please run this script from the HFST root directory."
    exit 1
fi

HFST_DIR="$(pwd)"
DEPS_DIR="${HFST_DIR}/wasm-deps"

# Activate emsdk environment if emmake is not found, as some clean steps might need it.
if [ -z $(which emmake) ] && [ -d "${DEPS_DIR}/emsdk" ]; then
    echo "Activating emsdk environment..."
    pushd "${DEPS_DIR}/emsdk"
    . emsdk_env.sh
    popd
fi

echo "Cleaning Foma..."
if [ -d "${DEPS_DIR}/foma/foma" ]; then
    pushd "${DEPS_DIR}/foma/foma"
    rm -rf CMakeCache.txt
    if [ -f Makefile ]; then
        make clean || echo "Foma clean failed, continuing..."
    else
        echo "Foma Makefile not found, skipping clean."
    fi
    popd
else
    echo "Foma directory not found, skipping clean."
fi

echo "Cleaning OpenFST..."
if [ -d "${DEPS_DIR}/openfst" ]; then
    pushd "${DEPS_DIR}/openfst"
    if [ -f Makefile ]; then
        make clean || echo "OpenFST clean failed, continuing..."
    else
        echo "OpenFST Makefile not found, skipping clean."
    fi
    popd
else
    echo "OpenFST directory not found, skipping clean."
fi

echo "Cleaning ICU native build..."
if [ -d "${DEPS_DIR}/icu/source/native-build" ]; then
    pushd "${DEPS_DIR}/icu/source/native-build"
    if [ -f Makefile ]; then
        make clean || echo "ICU native clean failed, continuing..."
    else
        echo "ICU native Makefile not found, skipping clean."
    fi
    popd
else
    echo "ICU native-build directory not found, skipping clean."
fi

echo "Cleaning ICU wasm build..."
if [ -d "${DEPS_DIR}/icu/source/wasm-build" ]; then
    pushd "${DEPS_DIR}/icu/source/wasm-build"
    if [ -f Makefile ]; then
        emmake make clean || echo "ICU wasm clean failed, continuing..."
    else
        echo "ICU wasm Makefile not found, skipping clean."
    fi
    popd
else
    echo "ICU wasm-build directory not found, skipping clean."
fi

echo "Cleaning LibHFST..."
if [ -d "${HFST_DIR}/libhfst" ]; then
    pushd "${HFST_DIR}/libhfst"
    if [ -f Makefile ]; then
        emmake make clean || echo "LibHFST clean failed, continuing..."
    else
        echo "LibHFST Makefile not found, skipping clean."
    fi
    popd
else
    echo "LibHFST directory not found, skipping clean."
fi

echo "Cleaning compiled libraries in ${DEPS_DIR}/lib..."
rm -f ${DEPS_DIR}/lib/libfoma.a
rm -f ${DEPS_DIR}/lib/libfst.a
rm -f ${DEPS_DIR}/lib/libicu*.a

echo "Cleaning include links/copies in ${DEPS_DIR}/include..."
rm -rf ${DEPS_DIR}/include/fst
rm -rf ${DEPS_DIR}/include/unicode
rm -rf ${DEPS_DIR}/include/layout # Part of ICU
rm -rf ${DEPS_DIR}/include/URES*.h # Part of ICU

echo "Clean script finished."
