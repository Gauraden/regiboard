#!/bin/sh

IsPacketForHost() {
  if [ "${PACKET_FOR_HOST}" == "true" ]; then
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
	local staging_dir="${RFS_ROOT_DIR}/staging"
	local rfs_include="${staging_dir}/usr/include"
	local rfs_lib="${staging_dir}/usr/lib"
	PrintNotice 'Configuring...'
	# Running: autogen
	IsFileExists "$build_dir/autogen.sh" && ./autogen.sh
	# Running: configure
	if IsFileExists "$build_dir/configure"; then
	  # host
	  IsPacketForHost && ./configure ${PACKET_CONFIGURE} && return
    # target
	  local pkg_config="PKG_CONFIG=${RFS_ROOT_DIR}/host/usr/bin/pkg-config"
#	  local 
	  local conf_flags="--host=${TC_PREFIX} --prefix=${staging_dir}/usr --exec-prefix=${staging_dir}/usr"
	  if [ "${PACKET_ENV_VARS}" != '' ]; then
		  export ${PACKET_ENV_VARS}
	  fi
	  ./configure $conf_flags $pkg_config ${PACKET_CONFIGURE}
    PACKET_CONFIGURE=''
		return
	fi
	local cmake_cxx_compiler="${TC_CXX}"
	local cmake_c_compiler="${TC_C}"
	local cmake_find_root_path="${RFS_HOST_DIR}"
	local rb_cross_conf="${SRC_UTILS_DIR}/CMakeList.base.txt"
	local rb_include_dir="${INCLUDE_DIR};${rfs_include};${PACKET_INCLUDE}"
	local rb_link_dir="${rfs_lib};${staging_dir}/lib"
	# Running: make
	if IsFileExists "$build_dir/Makefile"; then
    export CMAKE_CXX_COMPILER=${cmake_cxx_compiler}
    export CMAKE_C_COMPILER=${cmake_c_compiler}
    export CMAKE_FIND_ROOT_PATH=${cmake_find_root_path}
    export RB_CROSS_CONF=${rb_cross_conf}
    export RB_INCLUDE_DIR=${rb_include_dir}
    export RB_LINK_DIR=${rb_link_dir}
	  return
	fi
	# Running: cmake
  IsFileExists "$build_dir/CMakeLists.txt" || \
    PrintAndDie 'Do not know how to configurate Automake'
  # host
	if IsPacketForHost; then
	  cmake ./
	  return
	fi
  # target
  cmake -DCMAKE_CXX_COMPILER=${cmake_cxx_compiler} \
        -DCMAKE_C_COMPILER=${cmake_c_compiler} \
        -DCMAKE_FIND_ROOT_PATH=${cmake_find_root_path} \
        -DRB_CROSS_CONF=${rb_cross_conf} \
        -DRB_INCLUDE_DIR=${rb_include_dir} \
        -DRB_LINK_DIR=${rb_link_dir}
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
  if IsPacketForHost; then
    TcHostMakeSources ${build_dir}
  else
    TcTargetMakeSources ${build_dir}
  fi
}

SetPacketControl() {
	DieIfNotDefined "$1" 'Path to "control" file of packet'
	local version=${PACKET_VERSION}
	if [ "$PACKET_EXTERN" != 'true' ]; then
    version="${version}-${RB_BUILD_ID}"
  fi
	echo -e \
"Package: ${PACKET_NAME}\n"\
"Version: ${version}\n"\
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
	SetPacketControl "$ctl_dir"
	# подготовка скриптов-обработчиков событий начала и завершения установки пакета
  local pre_install=$ctl_dir/preinst
  local post_install=$ctl_dir/postinst
  rm ${pre_install} ${post_install} 2> /dev/null
  PacketPreInstallHandler ${post_install} && chmod o+x ${post_install}
  PacketPostInstallHandler ${pre_install} && chmod o+x ${pre_install}
	# подготовка программных файлов
	PacketInstall $ipk_dir 2> $RB_INSTALL_LOG
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
	PacketEditConfigLine() {
	  echo "$1"
	}
	PacketPreInstallHandler() {
	  return 1
	}
	PacketPostInstallHandler() {
	  return 1
	}
	local packet=$1
	. "${CONF_PAK_DIR}/$packet"
  RB_INSTALL_LOG="${LOG_DIR}/${PACKET_NAME}.install.log"
	DieIfNotDefined "${PACKET_NAME}"    'Name of packet'
	PrintNotice "Packet: ---==< ${PACKET_NAME} >==---"
	# Maybe there is no need to build the packet. It will be marked as external!
	if [ "$PACKET_EXTERN" = 'true' ]; then
    PacketInstall 2> $RB_INSTALL_LOG
	  return
	fi
	DieIfNotDefined "${PACKET_TARBALL}" 'Name of tarball'
	local build_dir=$(GetPacketBuildDir $PACKET_TARBALL)
	local tarball="${DOWNLOAD_DIR}/${PACKET_TARBALL}"
	local rebuild=false
	local unpacked=false
	if [ "${SUBPROG_ARG1}" = 'clean' ]; then
		rebuild=true
	fi
	GetDirectoryForBuild() {
		if ! IsPacketForHost; then
		  echo "${BUILD_DIR}/${build_dir}.${BOARD_PREFIX}"
		else
		  echo "${BUILD_DIR}/${build_dir}.${_HOST_ARCH}"
		fi	
	}
	if [ -d "${SRC_UTILS_DIR}/${PACKET_NAME}/.git" ]; then
	  cd ${SRC_UTILS_DIR}/${PACKET_NAME} && git pull origin master
	fi
	if ! IsFileExists $(GetDirectoryForBuild); then
		if ! IsFileExists "$tarball"; then
			# Trying to find sources at src/utils/
			if [ ! -d "${SRC_UTILS_DIR}/${PACKET_NAME}" ]; then
			  if [ "${PACKET_REPOSITORY}" != "" ]; then
			    cd ${SRC_UTILS_DIR} && git clone ${PACKET_REPOSITORY} ${PACKET_NAME}
			  else
  				DieIfNotDefined "${PACKET_URL}" 'URL for downloading tarball of packet'
	  			# TODO: downloading from PACKET_URL
	  			PrintAndDie "Tarball was not found: ${PACKET_TARBALL}"
	  	  fi
      fi
			tarball="${SRC_UTILS_DIR}/${PACKET_NAME}"
  		build_dir=${PACKET_NAME}
		fi
		UnpackArchive "${tarball}" "${BUILD_DIR}"
		ApplyAllPatchesFor "${build_dir}" "${BUILD_DIR}/${build_dir}"
    MoveDir "${BUILD_DIR}/${build_dir}" $(GetDirectoryForBuild)
		rebuild=true
		unpacked=true
	fi
	build_dir=$(GetDirectoryForBuild)
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
		if IsDefined $PACKET_CONFIG_FILE; then
		  cp ${CONF_DIR}/${PACKET_NAME}/${PACKET_CONFIG_FILE} ${build_dir}/.config_orig
      # редактирование конфигурации
      rm -f ${build_dir}/.config
      while read config_line
      do           
        echo $(PacketEditConfigLine "$config_line") >> ${build_dir}/.config
      done < ${build_dir}/.config_orig
      rm -f ${build_dir}/.config_orig
    fi
		# Building
    PacketMake $build_dir
	fi
	if IsPacketForHost; then
  	PrintNotice 'Installing to "bin" directory...'
  	PacketInstall $BIN_DIR
	  return
	fi
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

