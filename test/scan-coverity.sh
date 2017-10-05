 #!/bin/sh

set -o errexit
set -o nounset

if [ -z "${COVERITY_HOST}" ]; then
    echo "COVERITY_HOST must be set"
    exit 1
fi

if [ -z "${COVERITY_STREAM}" ]; then
    echo "COVERITY_STREAM must be set"
    exit 1
fi

if [ -z "${COVERITY_USER}" ]; then
    echo "COVERITY_USER must be set"
    exit 1
fi

if [ -z "${COVERITY_PASSWORD}" ]; then
    echo "COVERITY_PASSWORD must be set"
    exit 1
fi

if [ -z "${ARCH}" ]; then
    echo "ARCH must be set"
    exit 1
fi

if [ -z "${VERSION}" ]; then
    echo "VERSION must be set"
    exit 1
fi

if [ -z "${NAME}" ]; then
    echo "NAME must be set"
    exit 1
fi

if [ -z "${SRC_DIRS}" ]; then
    echo "SRC_DIRS must be set"
    exit 1
fi



echo "Running Coverity"
echo ""

mkdir -p /coverity/idir

/cov/bin/cov-configure 					            \
            --config /coverity/idir/c-conf   			    \
            --gcc

/cov/bin/cov-translate                              		    \
            --config /coverity/idir/c-conf          		    \
            --dir /coverity/idir                    		    \
            $(make -n -f build/Makefile.gcc | grep -e gcc -e g++ -e "cc ")   \
            --compiler-outputs "bin/$ARCH/$NAME" 			        
            
/cov/bin/cov-analyze 					            \
            --ticker-mode no-spin				    \
            --dir /coverity/idir;				    \

/cov/bin/cov-commit-defects 				            \
            --dir /coverity/idir 				    \
            --ticker-mode no-spin				    \
            --host $COVERITY_HOST 				    \
            --https-port $COVERITY_PORT				    \
            --stream $COVERITY_STREAM				    \
            --user $COVERITY_USER	 			    \
            --password $COVERITY_PASSWORD 
