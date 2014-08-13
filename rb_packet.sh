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
	PrintNotice 'Configuring...'
	# Running: autogen
	IsFileExists "$build_dir/autogen.sh" && ./autogen.sh
	# Running: configure
	if IsFileExists "$build_dir/configure"; then
	  # host
	  IsPacketForHost && ./configure ${PACKET_CONFIGURE} && return
    # target
	  local pkg_config="PKG_CONFIG=${RFS_ROOT_DIR}/host/usr/bin/pkg-config"
	  local conf_flags="--host=${TC_PREFIX} --prefix=${RFS_ROOT_DIR}/target"
	  if [ "${PACKET_ENV_VARS}" != '' ]; then
		  export ${PACKET_ENV_VARS}
	  fi
	  ./configure $conf_flags ${TC_CONFIGURE_FLAGS} ${PACKET_CONFIGURE} $pkg_config # || \
      #./configure ${PACKET_CONFIGURE}
    PACKET_CONFIGURE=''
		return
	fi
	# Running: make
	IsFileExists "$build_dir/Makefile" && return
	# Running: cmake
  IsFileExists "$build_dir/CMakeLists.txt" || \
    PrintAndDie 'Do not know how to configurate Automake'
  # host
	IsPacketForHost && cmake ./ && return
  # target
	FLAGS="-I'${INCLUDE_DIR}' -I'${rfs_include}' -L'${rfs_lib}' ${PACKET_CONFIGURE}"
	export CC=${TC_C} CXX=${TC_CXX} CPP=${TC_CPP} \
	  CFLAGS="${FLAGS}" CXXFLAGS="${FLAGS}" && cmake ./
}

PacketClean() {
	local build_dir=$1
	PrintNotice 'Cleaning...'
	IsDefined ${PACKET_CLEAN} || \
	( IsPacketForHost && TcHostCleanSources ${build_dir} || \
	  TcTargetCleanSources ${build_dir} && return)
	${PACKET_CLEAN}
}

PacketMake() {
	local build_dir=$1
	PrintNotice "Building..."
  IsPacketForHost && TcHostMakeSources ${build_dir} \
  || TcTargetMakeSources ${build_dir}
}

SetPacketControl() {
	DieIfNotDefined "$1" 'Path to "control" file of packet'
	echo -e \
"Package: ${PACKET_NAME}\n"\
"Version: ${PACKET_VERSION}\n"\
"Filename: ${PACKET_NAME}.ipk\n"\
"Description: ${PACKET_DESCRIPTION}\n"\
"Section: regiboard/apps\n"\
"Priority: optional\n"\
"Maintainer: \"Vibrator\"\n"\
"Architecture: ${BOARD_ARCH}\n"\
"Homepage: http://vbrspb.ru\n"\
"Source: \n"\
"Depends: ${PACKET_DEPENDS_ON}\n"\
> "$1/control"
}

MakeIpkg() {
	DieIfNotDefined "${PACKET_NAME}" 'Name of packet'
	local ipk_dir="${TMP_DIR}/${PACKET_NAME}.ipk"
	local ctl_dir="$ipk_dir/CONTROL"
	PrintNotice 'Creating package...'
	mkdir $ipk_dir 2> ${_DEV_NULL}
	mkdir $ctl_dir 2> ${_DEV_NULL}
	# control file
	SetPacketControl "$ctl_dir"
	# install binary files
	PacketInstall $ipk_dir
	# creating ipkg
	# archive is indeed a Debian[esque] package
#	echo '2.0' > "$ipk_dir/debian-binary"
	tar -C $ipk_dir -czf $ipk_dir/data.tar.gz --exclude=./CONTROL --exclude=./*.tar.gz .
	tar -C $ipk_dir/CONTROL -czf $ipk_dir/control.tar.gz .
	ar -r "${PACKETS_DIR}/${PACKET_NAME}.ipk" $ipk_dir/data.tar.gz $ipk_dir/control.tar.gz > ${_DEV_NULL}
}

PacketResetConfig() {
	unset PACKET_NAME
	unset PACKET_VERSION
	unset PACKET_DESCRIPTION
	unset PACKET_TARBALL
	unset PACKET_URL
	unset PACKET_CLEAN
	unset PACKET_BUILD
	unset PACKET_CONFIGURE
}

PacketBuild() {
  PacketResetConfig
	PacketInstall() {
		PrintWarn "Skipping installation to: $1"
	}
	local packet=$1
	. "${CONF_PAK_DIR}/$packet"
	DieIfNotDefined "${PACKET_NAME}"    'Name of packet'
	PrintNotice "Packet: ---==< ${PACKET_NAME} >==---"
	# Maybe there is no need to build the packet. It will be marked as external!
	if [ "$PACKET_EXTERN" = 'true' ]; then
	  PacketInstall
	  return
	fi
	# Trying to build the packet...
	DieIfNotDefined "${PACKET_TARBALL}" 'Name of tarball'
	local build_dir=$(GetPacketBuildDir $PACKET_TARBALL)
	local tarball="${DOWNLOAD_DIR}/${PACKET_TARBALL}"
	local rebuild=false
	local unpacked=false
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
			fi
		fi
		# Unpacking and patching
		UnpackArchive "${tarball}" "${BUILD_DIR}"
		ApplyAllPatchesFor "${build_dir}" "${BUILD_DIR}/${build_dir}"
		if ! IsPacketForHost; then
		  MoveDir "${BUILD_DIR}/${build_dir}" "${BUILD_DIR}/${build_dir}.${BOARD_PREFIX}"
		else
		  MoveDir "${BUILD_DIR}/${build_dir}" "${BUILD_DIR}/${build_dir}.${_HOST_ARCH}"
		fi
		rebuild=true
		unpacked=true
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
		PacketConfigure $build_dir
		# Clearing
		if ! $unpacked; then
      PacketClean $build_dir
    fi
		# Copy registered configuration
		IsDefined $PACKET_CONFIG_FILE && \
		  cp ${CONF_DIR}/${PACKET_NAME}/${PACKET_CONFIG_FILE} ${build_dir}/.config
		# Building
    PacketMake $build_dir
	fi
	if IsPacketForHost; then
  	PrintNotice 'Installing to "bin" directory...'
  	PacketInstall
	  return
	fi
	# Making ipkg packet
	MakeIpkg
}

PacketsList() {
	Print "Available packets:"
	for packet in $(ls "${CONF_PAK_DIR}" | sed -r 's:(.+)\.conf:\1:'); do
		PrintNotice "$packet"
	done
}

BuildAllPackets() {
	Print "Building packets:"
	local packet=''
	for packet in $(ls "${CONF_PAK_DIR}"); do
		PacketBuild $packet
	done
}

BuidPackets() {
  IsFileExists "${CONF_PAK_DIR}" || \
		PrintAndDie "Packages config files was not found: ${CONF_PAK_DIR}"
	case "$SUBPROG_ARG" in
		''    ) PacketsList;;
		'all' ) BuildAllPackets;;
		*     ) PacketBuild "$SUBPROG_ARG.conf";;
	esac
}
