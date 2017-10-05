#!/bin/sh

set -o errexit
set -o nounset
set -o pipefail

TARGETS=$(for d in "$@"; do echo ./$d/*; done)

echo $TARGETS

echo "Running tests:"
echo "TODO - Tests"
echo

echo "Checking C formatting:"
echo "TODO - Lint"
echo
