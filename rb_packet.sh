#!/bin/sh

IsPacketForHost() {
  if [ "${PACKET_FOR_HOST}" = "true" ]; then
    return 0
  fi
  return 1
}

GetPacketBuildDir() {
	DieIfNotDefined "$1" 'Name of tarball'
	echo $(echo "$1" | sed -E 's:([^/]+).tar(.*)$:\1:') #'
}

PacketConfigure() {
	local build_dir="$1"
	local rfs_include="${RFS_ROOT_DIR}/staging/usr/include"
	local rfs_lib="${RFS_ROOT_DIR}/target/usr/lib"
	if IsFileExists "$build_dir/autogen.sh"; then
		./autogen.sh
	fi
	if IsFileExists "$build_dir/configure"; then
	  if ! IsPacketForHost; then
	    # target
		  local pkg_config="PKG_CONFIG=${RFS_ROOT_DIR}/host/usr/bin/pkg-config"
		  local conf_flags="--host=${TC_PREFIX} --prefix=${RFS_ROOT_DIR}/target"
		  if [ "${PACKET_ENV_VARS}" != '' ]; then
			  export ${PACKET_ENV_VARS}
		  fi
		  ./configure $conf_flags ${TC_CONFIGURE_FLAGS} ${PACKET_CONFIGURE} $pkg_config || \
  ./configure ${PACKET_CONFIGURE}
    else
      # host
      ./configure ${PACKET_CONFIGURE}
    fi
		return
	fi
	if IsFileExists "$build_dir/Makefile"; then
		return
	fi
	if IsFileExists "$build_dir/CMakeLists.txt"; then
	  if ! IsPacketForHost; then
	    # target
		  FLAGS="-I'${INCLUDE_DIR}' -I'${rfs_include}' -L'${rfs_lib}' ${PACKET_CONFIGURE}"
		  export CC=${TC_C} CXX=${TC_CXX} CPP=${TC_CPP} CFLAGS="${FLAGS}" CXXFLAGS="${FLAGS}" && cmake ./
		else
		  # host
  		cmake ./
		fi
		return
	fi
	PrintAndDie 'Do not know how to configurate Automake'
}

PacketClean() {
	local build_dir=$1
	#TcTargetCleanSources ${build_dir}
}

PacketMake() {
	local build_dir=$1
	if ! IsPacketForHost; then
  	TcTargetMakeSources ${build_dir}
  else
    TcHostMakeSources ${build_dir}
  fi
}

SetPacketControl() {
	DieIfNotDefined "$1" 'Path to "control" file of packet'
	echo -e \
"Package: ${PACKET_NAME}\n"\
"Version: ${PACKET_VERSION}\n"\
"Description: ${PACKET_DESCRIPTION}\n"\
"Section: regiboard/apps\n"\
"Priority: optional\n"\
"Maintainer: \"Vibrator\"\n"\
"Architecture: ${BOARD_ARCH}\n"\
"Homepage: http://vbrspb.ru\n"\
"Source: \n"\
"Depends: \n"\
> "$1/control"
}

MakeIpkg() {
	DieIfNotDefined "${PACKET_NAME}" 'Name of packet'
	local ipk_dir="${TMP_DIR}/${PACKET_NAME}.ipk"
	local ctl_dir="$ipk_dir/CONTROL"
	mkdir $ipk_dir 2> ${_DEV_NULL}
	mkdir $ctl_dir 2> ${_DEV_NULL}
	# control file
	SetPacketControl "$ctl_dir"
	# binary files
	PacketInstall $ipk_dir
	# creating ipkg
	# archive is indeed a Debian[esque] package
#	echo '2.0' > "$ipk_dir/debian-binary"
	tar -C $ipk_dir -czf $ipk_dir/data.tar.gz --exclude=./CONTROL .
	tar -C $ipk_dir/CONTROL -czf $ipk_dir/control.tar.gz .
	ar -r "${PACKETS_DIR}/${PACKET_NAME}.ipk" $ipk_dir/data.tar.gz $ipk_dir/control.tar.gz > ${_DEV_NULL}
}

PacketBuild() {
	unset PACKET_NAME
	unset PACKET_VERSION
	unset PACKET_DESCRIPTION
	unset PACKET_TARBALL
	unset PACKET_URL
	unset PACKET_CLEAN
	unset PACKET_BUILD
	unset PACKET_CONFIGURE
	PacketInstall() {
		PrintWarn "Skipping installation to: $1"
	}
	local packet=$1
	. "${PAK_CONF_DIR}/$packet"
	DieIfNotDefined "${PACKET_NAME}"    'Name of packet'
	DieIfNotDefined "${PACKET_TARBALL}" 'Name of tarball'

	PrintNotice "Packet: ---==< ${PACKET_NAME} >==---"
	local build_dir=$(GetPacketBuildDir $PACKET_TARBALL)
	local tarball="${DOWNLOAD_DIR}/${PACKET_TARBALL}"
	local rebuild=false
	if [ "${SUBPROG_ARG1}" = 'clean' ]; then
		rebuild=true
	fi
	if ! IsFileExists "${BUILD_DIR}/${build_dir}.${BOARD_PREFIX}"; then
		if ! IsFileExists "$tarball"; then
			# Trying to find sources at src/utils/
			if [ ! -d "${SRC_UTILS_DIR}/${PACKET_NAME}" ]; then
				DieIfNotDefined "${PACKET_URL}" 'URL for downloading tarball of packet'
				# TODO: downloading from PACKET_URL
				PrintAndDie "Tarball was not found: ${PACKET_TARBALL}"
			else
				tarball="${SRC_UTILS_DIR}/${PACKET_NAME}"
				build_dir=${PACKET_NAME}
				rm -rf "${BUILD_DIR}/${build_dir}.${BOARD_PREFIX}"
			fi
		fi
		# Unpacking and patching
		UnpackArchive "${tarball}" "${BUILD_DIR}"
		ApplyAllPatchesFor "${build_dir}" "${BUILD_DIR}/${build_dir}"
		if ! IsPacketForHost; then
		  mv "${BUILD_DIR}/${build_dir}" "${BUILD_DIR}/${build_dir}.${BOARD_PREFIX}"
		else
		  mv "${BUILD_DIR}/${build_dir}" "${BUILD_DIR}/${build_dir}.${_HOST_ARCH}"
		fi
		rebuild=true
	fi
	if ! IsPacketForHost; then
  	build_dir="${BUILD_DIR}/${build_dir}.${BOARD_PREFIX}"
  else
    build_dir="${BUILD_DIR}/${build_dir}.${_HOST_ARCH}"
	fi
	if [ "${SUBPROG_ARG1}" = 'mkpatch' ]; then
		CreatePatch "${build_dir}"
		return 0
	fi
	cd $build_dir
	# Rebuilding if it is necessary
	if $rebuild; then
		# Configuring
		PrintNotice 'Configuring...'
		PacketConfigure $build_dir
		# Clearing
		PrintNotice 'Cleaning...'
		if [ "${PACKET_CLEAN}" != "" ]; then
			${PACKET_CLEAN}
		else
			PacketClean $build_dir
		fi
		# Building
		PrintNotice 'Building...'
		if [ "${PACKET_BUILD}" != "" ]; then
			${PACKET_BUILD}
		else
			PacketMake $build_dir
		fi
	fi
	if IsPacketForHost; then
  	PrintNotice 'Installing to "bin" directory...'
  	PacketInstall
	  return
	fi
	# Making opkg packet
	PrintNotice 'Creating package...'
	MakeIpkg
	# Installing to rootfs
	PrintNotice 'Installing to rootfs...'
	PacketInstall "${RFS_ROOT_DIR}/target"
}

PacketsList() {
	Print "Available packets:"
	for packet in $(ls "${PAK_CONF_DIR}" | sed -r 's:(.+)\.conf:\1:'); do
		PrintNotice "$packet"
	done
}

BuildAllPackets() {
	Print "Building packets:"
	local packet=''
	for packet in $(ls "${PAK_CONF_DIR}"); do
		PacketBuild $packet
	done
}

BuidPackets() {
	if ! IsFileExists "${PAK_CONF_DIR}"; then
		PrintAndDie "Directory with config files for packets was not found: ${PAK_CONF_DIR}"
	fi
	case "$SUBPROG_ARG" in
		''    ) PacketsList;;
		'all' ) BuildAllPackets;;
		*     ) PacketBuild "$SUBPROG_ARG.conf";;
	esac
}
