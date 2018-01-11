
# Create dummy .tar image for embedding into payload.efi
# tar size is determined by parameter below.
# NOTE: The VxWorks build process must use grub_memdisk.exe to update
# embedded .tar before signing
truncate -s 40k test/memdisk.tar

tar -rf test/memdisk.tar grub.cfg

# grub-mkimage builds payload.efi below:
grub-mkimage -v -c grub_early.cfg -m test/memdisk.tar --prefix '' -O x86_64-efi -o test/payload.efi disk part_msdos part_gpt linux linux16 loopback normal configfile test search search_fs_uuid search_fs_file true search_label efi_uga efi_gop gfxterm gfxmenu gfxterm_menu fat ext2 ntfs cat echo ls help multiboot2 all_video efifwsetup gzio usbms password_pbkdf2 pbkdf2 verify gcry_rsa gcry_dsa gcry_sha512 gcry_sha1 crypto gcry_sha256 chain loadenv halt hashsum memdisk tar sleep lvm png multiboot
