#! /bin/bash -e

#===============================================================================
#
# Linux script file to aggregate and sort log files from the test media
#
#===============================================================================
#

# Check arguments and print usage information
if [ $# -ne 2 ]; then
    echo "Usage: $0 <ESP partition e.g. /dev/sdb1> <output file>"
    exit 1
else
    installdev=$1
    outputfile=$2
fi

# Check that $2 is writable
rm -rf $outputfile

# Setup the GPT on the device
mkdir -p "test/espmount"
sudo mount ${installdev} "test/espmount/"

rm -rf "test/.tmplog"
mkdir "test/.tmplog"
mkdir "test/.tmplog/root"
mkdir "test/.tmplog/primary"
mkdir "test/.tmplog/backup"

cp "test/espmount/root/log/"* "test/.tmplog/root/"
cp "test/espmount/primary/log/"* "test/.tmplog/primary/"
cp "test/espmount/backup/log/"* "test/.tmplog/backup/"
rm -f "test/.tmplog/root/logline.txt"
rm -f "test/.tmplog/primary/logline.txt"
rm -f "test/.tmplog/backup/logline.txt"
rm -f "test/.tmplog/root/LOGLINE.TXT"
rm -f "test/.tmplog/primary/LOGLINE.TXT"
rm -f "test/.tmplog/backup/LOGLINE.TXT"
cat "test/.tmplog/"*"/"* | sort > $outputfile

rm -rf "test/.tmplog"

sudo umount "test/espmount"
rmdir "test/espmount/"

