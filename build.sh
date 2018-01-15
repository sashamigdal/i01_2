#!/bin/bash

set -e -u

usage() {
    cat <<EOF
Usage: ${0##*/} [OPTIONS] <cmake args...>

The dash options (until --) are read by this script, and may be an alias as below:

OPTIONS
  -h            Show this message
  -d            Pass -DCMAKE_BUILD_TYPE=Debug
  -r            Pass -DCMAKE_BUILD_TYPE=Release
  -g            Use the GNU Compiler Collection (gcc)
  -i            Use the Intel Compiler Collection (icc)
  -c            Use the Clang/LLVM Compiler (clang)
  -C            Use the Clang/LLVM Compiler with clang-analyzer (clang-analyzer)
  -f            Remove existing build directories, building from scratch.
  -a            Use all compilers and build types.
  --            Pass through the remaining dash arguments to cmake.
EOF
}



BUILD_TYPES=()
COMPILERS=()
while getopts ":hfdricCga" OPTION
do
    case "$OPTION" in
        h)
            usage
            exit
            ;;
        f)
            CLEAN=1
            ;;
        d)
            BUILD_TYPES+=(Debug)
            ;;
        r)
            BUILD_TYPES+=(Release)
            ;;
        i)
            COMPILERS+=(intel)
            ;;
        c)
            COMPILERS+=(clang)
            ;;
        C)
            COMPILERS+=(clang-analyzer)
            ;;
        g)
            COMPILERS+=(gcc)
            ;;
        a)
            COMPILERS=(gcc clang) #intel)
            BUILD_TYPES=(Debug Release)
            ;;
        (*)
            [[ "$OPTION" == "--" ]] && break
            usage
            exit
    esac
done

shift $(( $OPTIND - 1 ))

[[ ${#COMPILERS[@]} -eq 0 ]] && COMPILERS=(gcc)
[[ ${#BUILD_TYPES[@]} -eq 0 ]] && BUILD_TYPES=(Release)

cd $(dirname $0)

for BUILD_TYPE in ${BUILD_TYPES[@]}; do
for COMPILER in ${COMPILERS[@]}; do
    if [[ "$COMPILER" == intel ]]; then
        # Intel compiler build
        export CC=icc CXX=icpc
        PFX_CMD=""
    elif [[ "$COMPILER" == clang ]]; then
        # Clang/LLVM build
        export CC=clang CXX=clang++
        PFX_CMD=""
    elif [[ "$COMPILER" == gcc ]]; then
        # GCC build
        export CC=gcc CXX=g++
        PFX_CMD=""
    elif [[ "$COMPILER" == "clang-analyzer" ]]; then
        export CC=clang CXX=clang++
        PFX_CMD="scan-build -o analyse"
    else
        echo "Unrecognized compiler: $COMPILER"
        exit 1
    fi

    BDIR=".build.${COMPILER}.${BUILD_TYPE}.${HOSTNAME}"
    if [ -n "${CLEAN:-}" ]; then
        rm -r "$BDIR"
    fi
    if [ ! -d "$BDIR" ]; then
        mkdir -p "$BDIR"/analyse
    fi
    pushd "$BDIR"
        ${PFX_CMD} cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" $* .. || exit $?
        ${PFX_CMD} cmake --build . || exit $?
        # Tests that stack-allocate OrderManager (oe, oe_simsession) will fail
        # with the default stack size of 8192, so we increase it here:
        ulimit -s 16384
        ${PFX_CMD} cmake --build . --target test || exit $?
    popd
done
done
