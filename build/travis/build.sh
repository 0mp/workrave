#!/bin/bash -x

if [[ $DOCKER_IMAGE ]]; then 
    WORKSPACE=/workspace
    SOURCE_DIR=${WORKSPACE}/source
    OUTPUT_DIR=${WORKSPACE}/output
    BUILD_DIR=${WORKSPACE}/build
else
    WORKSPACE=$TRAVIS_BUILD_DIR
    SOURCE_DIR=${WORKSPACE}
    OUTPUT_DIR=${WORKSPACE}/output
    BUILD_DIR=${WORKSPACE}/build
fi

CMAKE_FLAGS=()
CMAKE_FLAGS64=()
MAKE_FLAGS=()


build()
{
  config=$1
  cmake_args=("${!2}")

  mkdir -p ${BUILD_DIR}/${config}
  mkdir -p ${OUTPUT_DIR}/${config}

  cd ${BUILD_DIR}/${config}

  cmake ${SOURCE_DIR} -G"Unix Makefiles" -DCMAKE_INSTALL_PREFIX=${OUTPUT_DIR}/${config} ${cmake_args[@]}

  make ${MAKE_FLAGS[@]} VERBOSE=1
  make ${MAKE_FLAGS[@]} install VERBOSE=1
}

if [[ ${CONF_ENABLE} ]]; then
    for i in ${CONF_ENABLE//,/ }
    do
        CMAKE_FLAGS+=("-DWITH_$i=ON")
        CMAKE_FLAGS64+=("-DWITH_$i=ON")
    done
fi

if [[ ${CONF_DISABLE} ]]; then
    for i in ${CONF_DISABLE//,/ }
    do
        CMAKE_FLAGS+=("-DWITH_$i=OFF")
        CMAKE_FLAGS64+=("-DWITH_$i=OFF")
    done
fi

CMAKE_FLAGS+=("-DWITH_UI=${WITH_UI}")

if [[ $COMPILER = 'gcc' ]] ; then
    CMAKE_FLAGS+=("-DCMAKE_CXX_COMPILER=g++")
    CMAKE_FLAGS64+=("-DCMAKE_CXX_COMPILER=g++")
    CMAKE_FLAGS+=("-DCMAKE_C_COMPILER=gcc")
    CMAKE_FLAGS64+=("-DCMAKE_C_COMPILER=gcc")
elif [[ $COMPILER = 'clanf' ]] ; then
    CMAKE_FLAGS+=("-DCMAKE_CXX_COMPILER=clang++")
    CMAKE_FLAGS64+=("-DCMAKE_CXX_COMPILER=clang++")
    CMAKE_FLAGS+=("-DCMAKE_C_COMPILER=clang")
    CMAKE_FLAGS64+=("-DCMAKE_C_COMPILER=clang")
fi

if [[ $TRAVIS_OS_NAME = 'osx' ]]; then
    CMAKE_FLAGS+=("-DCMAKE_PREFIX_PATH=/usr/local/opt/qt5")
fi

case "$DOCKER_IMAGE" in
    mingw-qt5)
        CMAKE_FLAGS+=("-DCMAKE_TOOLCHAIN_FILE=${SOURCE_DIR}/build/cmake/mingw64.cmake")
        CMAKE_FLAGS+=("-DPREBUILT_PATH=${WORKSPACE}/prebuilt")
        ;;

    mingw-gtk)

        case "$CONFIG" in
            vs)
                CMAKE_FLAGS+=("-DCMAKE_TOOLCHAIN_FILE=${SOURCE_DIR}/build/cmake/mingw32.cmake")
                CMAKE_FLAGS+=("-DPREBUILT_PATH=${WORKSPACE}/prebuilt")
                ;;

            *)
                CMAKE_FLAGS+=("-DCMAKE_TOOLCHAIN_FILE=${SOURCE_DIR}/build/cmake/mingw32.cmake")
                CMAKE_FLAGS+=("-DPREBUILT_PATH=${OUTPUT_DIR}/.64")

                CMAKE_FLAGS64+=("-DCMAKE_TOOLCHAIN_FILE=${SOURCE_DIR}/build/cmake/mingw64.cmake")
                CMAKE_FLAGS64+=("-DWITH_UI=None")
                CMAKE_FLAGS64+=("-DCMAKE_BUILD_TYPE=Release")

                build ".64" CMAKE_FLAGS64[@]
                ;;
        esac
        ;;
esac

build "" CMAKE_FLAGS[@]