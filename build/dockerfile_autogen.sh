#!/bin/bash

# Parse input arguments
while [ "${1+defined}" ]; do
    case $1 in
        --build-type) Build_Type=$2 ; shift ;  shift ;;
        --arch) Arch=$2 ; shift ; shift ;;
        --name) Name=$2 ; shift ; shift ;;
        --baseimage) Base_Image=$2 ; shift ; shift ;;
        --version) Version=$2 ; shift ; shift ;;
        *)
            echo "Usage: {build-type|arch|name|baseimage}"
            exit 1
        ;;
    esac
done

case "$Build_Type" in
    executable)
        # Executable: add to image, and set as entrypoint
        sed  \
            -e "s|ARG_NAME|$Name|g" \
            -e "s|TARGET_NAME|$Name|g" \
            -e "s|ARG_ARCH|$Arch|g" \
            -e "s|ARG_FROM|$Base_Image|g" \
            -e "s|RUN RUN_CMD||g" \
            Dockerfile.in > .dockerfile-$Arch
    ;;
    static-library)
        # static library: add to image, no entrypoint
        sed  \
            -e "s|ENTRYPOINT \[\"/TARGET_NAME\"\]||" \
            -e "s|ARG_NAME||g" \
            -e "s|TARGET_NAME|${Name}/|g" \
            -e "s|ARG_ARCH|$Arch|g" \
            -e "s|ARG_FROM|$Base_Image|g" \
            -e "s|RUN RUN_CMD||" \
            Dockerfile.in > .dockerfile-$Arch
    ;;
    shared-library)
        # shared library (.so lib): add to image & set up links, no entrypoint
        real_name="lib${Name}.so.${Version}"
        so_name="lib${Name}.so.${Version%%.*}"
        linker_name="lib${Name}.so"
        run_cmd="ln -s /usr/local/lib/$real_name /usr/local/lib/$so_name \&\& ln -s /usr/local/lib/$so_name /usr/local/lib/$linker_name"
        sed  \
            -e "s|ENTRYPOINT \[\"/TARGET_NAME\"\]||" \
            -e "s|ARG_NAME|$lib_name|g" \
            -e "s|TARGET_NAME|usr/local/lib/$lib_name|g" \
            -e "s|ARG_ARCH|$Arch|g" \
            -e "s|ARG_FROM|$Base_Image|g" \
            -e "s|RUN_CMD|$run_cmd|" \
            Dockerfile.in > .dockerfile-$Arch
    ;;
    dynamic-library)
        # dynamic library: add to image, no entrypoint
        sed  \
            -e "s|ENTRYPOINT \[\"/TARGET_NAME\"\]||" \
            -e "s|ARG_NAME||g" \
            -e "s|TARGET_NAME|${Name}/|g" \
            -e "s|ARG_ARCH|$Arch|g" \
            -e "s|ARG_FROM|$Base_Image|g" \
            -e "s|RUN RUN_CMD||" \
            Dockerfile.in > .dockerfile-$Arch
    ;;
    *) 
        echo "Usage dockerfile_autogen: BUILD_TYPE must have value in {executable|static-library|shared-library|dynamic-library}"
        exit 1
    ;;
esac