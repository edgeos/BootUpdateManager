#/bin/bash
set -x
rm -rf test/statusdir

bin/amd64/bumstate-init

bin/amd64/bumstate-init test/statusdir

bin/amd64/bumstate-init test/statusdir sda2

mkdir test/statusdir

bin/amd64/bumstate-print

bin/amd64/bumstate-print test/statusdir

bin/amd64/bumstate-update-start

bin/amd64/bumstate-update-start test/statusdir/

bin/amd64/bumstate-boottime-test

bin/amd64/bumstate-boottime-test test/statusdir/

bin/amd64/bumstate-update-complete

bin/amd64/bumstate-update-complete test/statusdir/

bin/amd64/bumstate-update-complete test/statusdir/ 3

bin/amd64/bumstate-update-complete test/statusdir/ three sda3

bin/amd64/bumstate-update-complete test/statusdir/ 3 ""

bin/amd64/bumstate-update-complete test/statusdir/ 3 sda3

bin/amd64/bumstate-init test/statusdir sda2

bin/amd64/bumstate-print test/statusdir

bin/amd64/bumstate-boottime-test test/statusdir/

bin/amd64/bumstate-print test/statusdir

bin/amd64/bumstate-update-start test/statusdir

bin/amd64/bumstate-print test/statusdir

bin/amd64/bumstate-boottime-test test/statusdir/

bin/amd64/bumstate-print test/statusdir

bin/amd64/bumstate-update-complete test/statusdir/ 2 sda3

bin/amd64/bumstate-print test/statusdir

bin/amd64/bumstate-boottime-test test/statusdir/

bin/amd64/bumstate-print test/statusdir

bin/amd64/bumstate-boottime-test test/statusdir/

bin/amd64/bumstate-print test/statusdir

bin/amd64/bumstate-boottime-test test/statusdir/

bin/amd64/bumstate-print test/statusdir

rm -rf test/statusdir
