Directories:
	* board........: for boards configurations
	* build........: for unpacked sources
	* conf.........: for storing configurations of kernels, tools, etc.
	* -> packets...: configurations for building application and libraries
	* dl...........: for storing downloaded files
	* log..........: for storing logfiles
	* output.......: for storing results of buildings
	* src..........: sources of special utils for boards
	* -> etc.......: config files for /etc directory of target system
	* -> patches...: patches for anything that can be build
	* -> shell.....: shell scripts for target system
	* -> utils.....: sources of special utilities, developed within RegiBoard
	* -> firmware..: binary files of firmwares for boards
	* tmp..........: for temporary files, caches etc.
	
Units:
	* rb_build.sh................: Main script, all commands are executed through it
	* rb_core.sh.................: Core functions for primary initialisation
	* rb_helpers.sh..............: Pack of commonly used functions
	* rb_initramfs.sh............: Functions for building initramfs
	* rb_kernel.sh...............: Functions for building Linux kernel
	* rb_mmc.sh..................: Functions for preparing bootable MMC/SD
	* rb_packet.sh...............: Functions for building packets
	* rb_patch_rootfs_image.sh...: Functions for patching 'BuildRoot' rootfs image
	* rb_rootfs.sh...............: Functions for building rootfs image, using 'BuildRoot'
	* rb_toolchain.sh............: Functions for building toolchain
	* rb_uboot.sh................: Functions for building bootloader 'U-Boot'
	
How to use:
	Format....: ./rb_build <unit> <args>
		<unit>..: name of executable unit
		  * help.....: for printing this help
		  * board....: for selecting target board
			* toolchain:
			* uboot....:
			* kernel...:
			* rootfs...:
			* initramfs:
			* packets..:
			* image....: [NOT READY]
			* mmc......: for writing all binaries and images to MMC
		<args>..: arguments for executable unit, if it is empty then default build 
		          programm will be executed.
			* config...: for configurating of unit
			* mkpatch..: for generating of patch file, it will check only *.orig files.
			             This patch will be automaticaly applyed in future builds!
			* clean....: for cleaning of build files.
			             After cleaning building of programm will be executed.
