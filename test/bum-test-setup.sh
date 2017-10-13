#! /bin/bash -e

#===============================================================================
#
# Script to setup the basic tests for the BUM boot-time component
#
#===============================================================================

#===============================================================================
#
# Directory and file paths
#
#===============================================================================
#

#Source directory
BIN_DIR="bin"

#Main test directories
TEST_DIR="test"
EFI_TEST_DIR="${TEST_DIR}/efi"
TMP_KEYDIR="${EFI_TEST_DIR}/tmpkey"

#Source file names
shellimage=${TEST_DIR}/assets/Shell.efi
list=${BIN_DIR}/edk2/*/X64/BootUpdateManager.efi
bumimage=${list[0]}
list=${BIN_DIR}/edk2/*/X64/TestPayload.efi
payloadimage=${list[0]}

#Final Tarball
TAR_FILE="${TEST_DIR}/testesp.tar.gz"

#ESP directories
EFI_TEST_ESPDIR="${EFI_TEST_DIR}/esp"
TEST_BOOT_DIR="${EFI_TEST_ESPDIR}/efi/boot"
TEST_ROOT_DIR="${EFI_TEST_ESPDIR}/root"
TEST_PRIM_DIR="${EFI_TEST_ESPDIR}/primary"
TEST_BKUP_DIR="${EFI_TEST_ESPDIR}/backup"

KEYDIR_NAME="keys"
LOGDIR_NAME="log"

#ESP file names
EFIBIN_NAME="bootx64.efi"
PAYLD_NAME="grubx64.efi"

PKUPDATE_NAME="PK.update.auth"
KEKUPDATE_NAME="KEK.update.auth"
DBUPDATE_NAME="db.update.auth"
DBAPPEND_NAME="db.append.auth"

#Key file names
guidfile=${TMP_KEYDIR}/guid


#===============================================================================
#
# Library of miscellaneous bash functions
#
#===============================================================================
#

print_error_exit () {
    echo "*** bum-tes-setup.sh error (line ${1}): $2" 1>&2
    exit 1
}

#===============================================================================
#
# Library of bash functions for generating UEFI keys, EFI signature lists and
# signed updates for UEFI varriables.
#
#===============================================================================
#

#-------------------------------------------------------------------------------
# createKey() - create a self signed X509 certificate .crt using unprotected key .key
#   key = private key - Base64-encoded
#   crt = public key - Base64-encoded
#   cer = public key - DER-encoded binary
#
createKey()
{
	keyname=$1	# key name
	name=$2		# common name
	openssl req -new -x509 -newkey rsa:2048 -subj "/CN=$name/" -keyout $keyname.key -out $keyname.crt -days 3650 -nodes -sha256
	openssl x509 -in $keyname.crt -out $keyname.cer -outform DER
}
#-------------------------------------------------------------------------------
# createEsl() - create EFI signiture list from public key (EfiTools)
#    esl - EFI Signature List
#
createEsl()
{
	keyname=$1	# key name
	if [[ $# -gt 1 ]]; then
		guidfile=$2	# where "uuidgen > $4" was executed at some point in the past
		guid="$(cat $guidfile)"
		cert-to-efi-sig-list -g "${guid}" $keyname.crt $keyname.esl
	else
		cert-to-efi-sig-list $keyname.crt $keyname.esl
	fi
}
#-------------------------------------------------------------------------------
# createAuthUpdate() - create authorised update (EfiTools)
#    auth - signed EFI signature list with authentication header
#
createAuthUpdate()
{
	varname=$1	# the name of the variable (PK, KEK, db, or dbx)
	keyname=$2	# ouput key name
	signingkey=$3	# key used to sign update
	if [[ $# -gt 3 ]]; then
		guidfile=$4	# where "uuidgen > $4" was executed at some point in the past
		guid="$(cat $guidfile)"
		if [[ $# -gt 4 ]]; then
			tstamp="${5}"
			sign-efi-sig-list -g "${guid}" -t "${tstamp}" -k $signingkey.key -c $signingkey.crt $varname $keyname.esl $keyname.update.auth
		else
			sign-efi-sig-list -g "${guid}" -k $signingkey.key -c $signingkey.crt $varname $keyname.esl $keyname.update.auth
		fi
	else
		sign-efi-sig-list -k $signingkey.key -c $signingkey.crt $varname $keyname.esl $keyname.update.auth
	fi
}
#-------------------------------------------------------------------------------
# createAuthAppendUpdate() - create authorised append update (EfiTools)
#    auth - signed EFI signature list with authentication header
#
createAuthAppendUpdate()
{
	varname=$1	# the name of the variable (PK, KEK, db, or dbx)
	keyname=$2	# ouput key name
	signingkey=$3	# key used to sign update
	if [[ $# -gt 3 ]]; then
		guidfile=$4	# where "uuidgen > $4" was executed at some point in the past
		guid="$(cat $guidfile)"
		if [[ $# -gt 4 ]]; then
			tstamp="${5}"
			sign-efi-sig-list -a -g "$guid" -t "${tstamp}" -k $signingkey.key -c $signingkey.crt $varname $keyname.esl $keyname.append.auth
		else
			sign-efi-sig-list -a -g "$guid" -k $signingkey.key -c $signingkey.crt $varname $keyname.esl $keyname.append.auth
		fi
	else
		sign-efi-sig-list -a -k $signingkey.key -c $signingkey.crt $varname $keyname.esl $keyname.append.auth
	fi
}

#===============================================================================
#
# Main script
#
#===============================================================================

echo "****************************************"
echo "  $0"
echo "****************************************"

# Chek for required binary files

#Test for the test directory and write permissions
if [ ! -d $TEST_DIR ] || [ ! -w $TEST_DIR ]; then
    print_error_exit "$LINENO" "Unable to write to $TEST_DIR"
fi

#Test for the three EFI images

if [ ! -f $shellimage ] || [ ! -r $shellimage ]; then
    print_error_exit "$LINENO" "Unable to read $shellimage"
fi

if [ ! -f $bumimage ] || [ ! -r $bumimage ]; then
    print_error_exit "$LINENO" "Unable to read $bumimage"
fi

if [ ! -f $payloadimage ] || [ ! -r $payloadimage ]; then
    print_error_exit "$LINENO" "Unable to read $payloadimage"
fi

# Create the directory structure
echo "********************"
echo
echo "  Resetting the test directory"
echo
rm -rf ${EFI_TEST_DIR}
rm -rf ${TAR_FILE}
mkdir -p ${EFI_TEST_DIR}
echo

echo "********************"
echo
echo "  ESP Setup:"
echo
mkdir -p ${EFI_TEST_ESPDIR}

mkdir -p ${TEST_BOOT_DIR}

mkdir -p ${TEST_ROOT_DIR}
mkdir -p "${TEST_ROOT_DIR}/${LOGDIR_NAME}"
mkdir -p "${TEST_ROOT_DIR}/${KEYDIR_NAME}"

mkdir -p ${TEST_PRIM_DIR}
mkdir -p "${TEST_PRIM_DIR}/${LOGDIR_NAME}"
mkdir -p "${TEST_PRIM_DIR}/${KEYDIR_NAME}"

mkdir -p ${TEST_BKUP_DIR}
mkdir -p "${TEST_BKUP_DIR}/${LOGDIR_NAME}"
mkdir -p "${TEST_BKUP_DIR}/${KEYDIR_NAME}"

# Create the 5 keys
echo "********************"
echo
echo "  Key Setup:"
echo
mkdir -p ${TMP_KEYDIR}
uuidgen -t > ${guidfile}

# PK
echo "        Generating PK ..."
createKey $TMP_KEYDIR/PK "BUM Testing Platform Key"
createEsl $TMP_KEYDIR/PK ${guidfile}
timestamp=$(date -u +'%F %T')
createAuthUpdate PK $TMP_KEYDIR/PK $TMP_KEYDIR/PK ${guidfile} "${timestamp}"

# KEK
echo "        Generating KEK ..."
createKey $TMP_KEYDIR/KEK "BUM Testing Key Exchange Key"
createEsl $TMP_KEYDIR/KEK ${guidfile}
sleep 1s
timestamp=$(date -u +'%F %T')
createAuthUpdate KEK $TMP_KEYDIR/KEK $TMP_KEYDIR/PK ${guidfile} "${timestamp}"

# db0
echo "        Generating db keys ..."
createKey $TMP_KEYDIR/db0 "BUM Testing db Key 0"
createEsl $TMP_KEYDIR/db0 ${guidfile}
sleep 1s
timestamp=$(date -u +'%F %T')
createAuthUpdate db $TMP_KEYDIR/db0 $TMP_KEYDIR/KEK ${guidfile} "${timestamp}"

# db1
createKey $TMP_KEYDIR/db1 "BUM Testing db Key 1"
createEsl $TMP_KEYDIR/db1 $guidfile
createAuthAppendUpdate db $TMP_KEYDIR/db1 $TMP_KEYDIR/KEK $guidfile

# db1
createKey $TMP_KEYDIR/db2 "BUM Testing db Key 2"
createEsl $TMP_KEYDIR/db2 $guidfile
createAuthAppendUpdate db $TMP_KEYDIR/db2 $TMP_KEYDIR/KEK $guidfile

echo "        Deploying keys ..."
cp "$TMP_KEYDIR/PK.update.auth" "${TEST_ROOT_DIR}/${KEYDIR_NAME}/PK.update.auth"
cp "$TMP_KEYDIR/KEK.update.auth" "${TEST_ROOT_DIR}/${KEYDIR_NAME}/KEK.update.auth"
cp "$TMP_KEYDIR/db0.update.auth" "${TEST_ROOT_DIR}/${KEYDIR_NAME}/db.update.auth"
cp "$TMP_KEYDIR/db1.append.auth" "${TEST_PRIM_DIR}/${KEYDIR_NAME}/db.append.auth"
cp "$TMP_KEYDIR/db2.append.auth" "${TEST_BKUP_DIR}/${KEYDIR_NAME}/db.append.auth"

echo "********************"
echo
echo "  Image Setup:"
echo

echo "      Signing/Deploying images ..."
sbsign --key ${TMP_KEYDIR}/db0.key --cert ${TMP_KEYDIR}/db0.crt --output "${TEST_BOOT_DIR}/${EFIBIN_NAME}" ${shellimage}

sbsign --key ${TMP_KEYDIR}/db0.key --cert ${TMP_KEYDIR}/db0.crt --output "${TEST_ROOT_DIR}/${EFIBIN_NAME}" ${bumimage}

sbsign --key ${TMP_KEYDIR}/db1.key --cert ${TMP_KEYDIR}/db1.crt --output "${TEST_PRIM_DIR}/${EFIBIN_NAME}" ${bumimage}

cp ${payloadimage} "${TEST_PRIM_DIR}/${PAYLD_NAME}"

sbsign --key ${TMP_KEYDIR}/db1.key --cert ${TMP_KEYDIR}/db1.crt --output "${TEST_BKUP_DIR}/${EFIBIN_NAME}" ${bumimage}

sbsign --key ${TMP_KEYDIR}/db2.key --cert ${TMP_KEYDIR}/db2.crt --output "${TEST_BKUP_DIR}/${PAYLD_NAME}" ${payloadimage}

echo "********************"
echo
echo "  Packing:"
echo
tar -zcvf "${TAR_FILE}" -C ${EFI_TEST_ESPDIR} "efi" "root" "primary" "backup" --owner=0 --group=0

echo "********************"
echo
echo "  Cleanup:"
echo
rm -rf ${EFI_TEST_DIR}

echo "****************************************"
echo "  END $0"
echo "****************************************"
