#! /bin/bash -e

#===============================================================================
#
# Linux script file to install (sign and copy) BUM image and GRUB image to the
# primary boot path.
#
#===============================================================================
#

BLOCKSIZE="512"
PARTOFFSET="2048"
PARTSIZE="$(( 1024 * 1024 * 1024 / BLOCKSIZE ))" #256MB
INSTALL_ROOTPARTSIZE="$((INSTALL_IMGSIZE - PARTOFFSET))" #256MB - (the GPT table size)

# Check arguments and print usage information
if [ $# -ne 1 ]; then
    echo "Usage: $0 <install media e.g. /dev/sdb>"
    exit 1
e
    installdev=$1
fi

# Setup the GPT on the device
parted ${installdev} "mktable gpt"
parted ${installdev} "mkpart p fat32 ${PARTOFFSET}s ${PARTSIZE}s"
parted ${installdev} "toggle 1 boot"
parted ${installdev} "name 1 UEFI"

# Write the image to disk
mkfs -t vfat -n UEFI "${installdev}1"

