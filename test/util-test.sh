#/bin/bash
set +x
echo "*************************************************************************"
echo "**** BUM utility tests"
echo "*************************************************************************"
echo
echo "**** Argument-validation tests ****"
echo

set -x

rm -rf test/statusdir

bin/amd64/bumstate-init

bin/amd64/bumstate-init test/statusdir

bin/amd64/bumstate-init test/statusdir sda2

mkdir test/statusdir

bin/amd64/bumstate-print

bin/amd64/bumstate-print test/statusdir

bin/amd64/bumstate-boottime-test

bin/amd64/bumstate-boottime-test test/statusdir/

bin/amd64/bumstate-runtime-init 

bin/amd64/bumstate-runtime-init test/statusdir/

bin/amd64/bumstate-update-start

bin/amd64/bumstate-update-start test/statusdir/

bin/amd64/bumstate-update-complete

bin/amd64/bumstate-update-complete test/statusdir/

bin/amd64/bumstate-update-complete test/statusdir/ 3

bin/amd64/bumstate-update-complete test/statusdir/ three sda3

bin/amd64/bumstate-update-complete test/statusdir/ 3 ""

bin/amd64/bumstate-update-complete test/statusdir/ 3 sda3

bin/amd64/bumstate-currconfig-get

bin/amd64/bumstate-currconfig-get test/statusdir/

bin/amd64/bumstate-noncurrconfig-get

bin/amd64/bumstate-noncurrconfig-get test/statusdir/

set +x

echo "*************************************************************************"
echo "**** Functionality tests ****"
echo

echo "*************************************************************************"
echo "* Initialization and first boot tests"
echo

set -x
bin/amd64/bumstate-init test/statusdir sda2
set +x

bin/amd64/bumstate-print test/statusdir

set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
bin/amd64/bumstate-runtime-init test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

echo "*************************************************************************"
echo "* Reboot after update start, before update completes"
echo

set -x
bin/amd64/bumstate-update-start test/statusdir
set +x

bin/amd64/bumstate-print test/statusdir

set -x
CURRCONFIG=`bin/amd64/bumstate-currconfig-get test/statusdir/`
NONCURRCONFIG=`bin/amd64/bumstate-noncurrconfig-get test/statusdir/`
set +x
bin/amd64/bumstate-print test/statusdir
echo "    CURRCONFIG    =\"${CURRCONFIG}\""
echo "    NONCURRCONFIG =\"${NONCURRCONFIG}\""

set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
bin/amd64/bumstate-runtime-init test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
CURRCONFIG=`bin/amd64/bumstate-currconfig-get test/statusdir/`
NONCURRCONFIG=`bin/amd64/bumstate-noncurrconfig-get test/statusdir/`
set +x
bin/amd64/bumstate-print test/statusdir
echo "    CURRCONFIG    =\"${CURRCONFIG}\""
echo "    NONCURRCONFIG =\"${NONCURRCONFIG}\""

echo "*************************************************************************"
echo "* Failed complete update "
echo

set -x
bin/amd64/bumstate-update-start test/statusdir
set +x

bin/amd64/bumstate-print test/statusdir

set -x
CURRCONFIG=`bin/amd64/bumstate-currconfig-get test/statusdir/`
NONCURRCONFIG=`bin/amd64/bumstate-noncurrconfig-get test/statusdir/`
set +x
bin/amd64/bumstate-print test/statusdir
echo "    CURRCONFIG    =\"${CURRCONFIG}\""
echo "    NONCURRCONFIG =\"${NONCURRCONFIG}\""

set -x
bin/amd64/bumstate-update-complete test/statusdir/ 2 sda3
set +x

bin/amd64/bumstate-print test/statusdir

#First reboot
set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

#Second reboot
set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

#Third reboot (Alternate)
set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
bin/amd64/bumstate-runtime-init test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
CURRCONFIG=`bin/amd64/bumstate-currconfig-get test/statusdir/`
NONCURRCONFIG=`bin/amd64/bumstate-noncurrconfig-get test/statusdir/`
set +x
bin/amd64/bumstate-print test/statusdir
echo "    CURRCONFIG    =\"${CURRCONFIG}\""
echo "    NONCURRCONFIG =\"${NONCURRCONFIG}\""

echo "*************************************************************************"
echo "* Successful complete update "
echo

set -x
bin/amd64/bumstate-update-start test/statusdir
set +x

bin/amd64/bumstate-print test/statusdir

set -x
CURRCONFIG=`bin/amd64/bumstate-currconfig-get test/statusdir/`
NONCURRCONFIG=`bin/amd64/bumstate-noncurrconfig-get test/statusdir/`
set +x
bin/amd64/bumstate-print test/statusdir
echo "    CURRCONFIG    =\"${CURRCONFIG}\""
echo "    NONCURRCONFIG =\"${NONCURRCONFIG}\""

set -x
bin/amd64/bumstate-update-complete test/statusdir/ 2 sda3
set +x

bin/amd64/bumstate-print test/statusdir

#First reboot
set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
bin/amd64/bumstate-runtime-init test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
CURRCONFIG=`bin/amd64/bumstate-currconfig-get test/statusdir/`
NONCURRCONFIG=`bin/amd64/bumstate-noncurrconfig-get test/statusdir/`
set +x
bin/amd64/bumstate-print test/statusdir
echo "    CURRCONFIG    =\"${CURRCONFIG}\""
echo "    NONCURRCONFIG =\"${NONCURRCONFIG}\""

echo "*************************************************************************"
echo "* Successful reboot without updates "
echo

set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
bin/amd64/bumstate-runtime-init test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
CURRCONFIG=`bin/amd64/bumstate-currconfig-get test/statusdir/`
NONCURRCONFIG=`bin/amd64/bumstate-noncurrconfig-get test/statusdir/`
set +x
bin/amd64/bumstate-print test/statusdir
echo "    CURRCONFIG    =\"${CURRCONFIG}\""
echo "    NONCURRCONFIG =\"${NONCURRCONFIG}\""

echo "*************************************************************************"
echo "* Failed reboot without updates "
echo

#First reboot
set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

#Second reboot
set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

#Third reboot (Alternate)
set -x
bin/amd64/bumstate-boottime-test test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
bin/amd64/bumstate-runtime-init test/statusdir/
set +x

bin/amd64/bumstate-print test/statusdir

set -x
CURRCONFIG=`bin/amd64/bumstate-currconfig-get test/statusdir/`
NONCURRCONFIG=`bin/amd64/bumstate-noncurrconfig-get test/statusdir/`
set +x
bin/amd64/bumstate-print test/statusdir
echo "    CURRCONFIG    =\"${CURRCONFIG}\""
echo "    NONCURRCONFIG =\"${NONCURRCONFIG}\""

echo "*************************************************************************"
echo "**** End tests"

set -x
rm -rf test/statusdir
set +x

echo "*************************************************************************"
