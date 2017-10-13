#! /bin/bash -xe

#===============================================================================
#
# Linux script file to install (sign and copy) BUM image and GRUB image to the
# primary boot path.
#
#===============================================================================
#

# Check arguments and print usage information
if [ $# -ne 1 ]; then
    echo "Usage: $0 <install media partition e.g. /dev/sdb1>"
    exit 1
else
    installdev=$1
fi

mkdir -p "test/espmount"
sudo mount ${installdev} "test/espmount/"
sudo rm -rf "test/espmount/*"
sudo tar -zxvf "test/testesp.tar.gz" -C "test/espmount"
sudo umount "test/espmount"
rmdir "test/espmount/"

