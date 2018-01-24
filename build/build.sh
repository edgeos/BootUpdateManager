#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

if [ -z "${ARCH}" ]; then
    echo "ARCH must be set"
    exit 1
fi

if [ -z "${BUILD_TYPE}" ]; then
    echo "BUILD_TYPE must be set"
    exit 1
fi

function ExecutableTarget () {

    export cc_flags='-Wall -static '
    make -f build/Makefile.gcc

}

function StaticLib() {

    export cc_flags='-static -c -Wall'
    export post_build='mv *.o $(arch_dir); ar r $(arch_dir)/lib${NAME}.a.${VERSION} $(arch_dir)/*.o'
    make -f build/Makefile.gcc

}

function SharedLib() {

    export cc_flags='-fPIC -g -c -Wall'
    export post_build='mv *.o $(arch_dir)'
    make -f build/Makefile.gcc
    export post_build=''

    export cc_flags="-shared -Wl,-soname,lib${NAME}.so.${VERSION%%.*} -o \$(arch_dir)/lib${NAME}.so.${VERSION}"
    export source_files='$(arch_dir)/*.o'
    export ld_flags='-lc'
    export post_build='rm $(arch_dir)/*.o'
    make -f build/Makefile.gcc
    

}

case "$BUILD_TYPE" in
    executable) 
        ExecutableTarget
    ;;
#    static-library) 
#        StaticLib
#    ;;
#    shared-library)
#        SharedLib
#    ;;
#    dynamic-library) 
#        DynamicLib
#    ;;
    *)
        echo "Usage: $0 {executable|static-library|shared-library|dynamic-library}"
        exit 1
esac
