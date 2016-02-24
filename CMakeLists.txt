cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

set(PROJECT_NAME mlogind)
project(${PROJECT_NAME})

#Pretty colors
set(CMAKE_COLOR_MAKEFILE ON)
#Dont force verbose
set(CMAKE_VERBOSE_MAKEFILE ON)
#Include current dir
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

INCLUDE(CheckIncludeFile)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckTypeSize)
INCLUDE(GNUInstallDirs)

# Version
set(MLOGIND_VERSION_MAJOR "1")
set(MLOGIND_VERSION_MINOR "3")
set(MLOGIND_VERSION_PATCH "6")
set(MLOGIND_VERSION "${MLOGIND_VERSION_MAJOR}.${MLOGIND_VERSION_MINOR}.${MLOGIND_VERSION_PATCH}")

set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation Directory")
set(PKGDATADIR "${CMAKE_INSTALL_PREFIX}/share/mlogind")
if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR
		${CMAKE_SYSTEM_NAME} MATCHES "MidnightBSD"
		)
	set(SYSCONFDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}")
	set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
	set(MANDIR "${CMAKE_INSTALL_PREFIX}/man")
else()
        set(SYSCONFDIR "/etc")
        set(LIBDIR "/lib")
        set(MANDIR "${CMAKE_INSTALL_PREFIX}/share/man")
endif()

set(MLOGIND_DEFINITIONS)
if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR
		${CMAKE_SYSTEM_NAME} MATCHES "MidnightBSD" OR
		${CMAKE_SYSTEM_NAME} MATCHES "MirBSD" OR
		${CMAKE_SYSTEM_NAME} MATCHES "NetBSD" OR
		${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD"
		)
	set(MLOGIND_DEFINITIONS ${MLOGIND_DEFINITIONS} "-DNEEDS_BASENAME")
else()
	set(MLOGIND_DEFINITIONS ${MLOGIND_DEFINITIONS} "-DHAVE_SHADOW")
endif()

set(MLOGIND_DEFINITIONS ${MLOGIND_DEFINITIONS} "-DPACKAGE=\"mlogind\"")
set(MLOGIND_DEFINITIONS ${MLOGIND_DEFINITIONS} "-DVERSION=\"${MLOGIND_VERSION}\"")
set(MLOGIND_DEFINITIONS ${MLOGIND_DEFINITIONS} "-DPKGDATADIR=\"${PKGDATADIR}\"")
set(MLOGIND_DEFINITIONS ${MLOGIND_DEFINITIONS} "-DSYSCONFDIR=\"${SYSCONFDIR}\"")

# Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -O2")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -Wall -g -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O2")

# source 
set(mlogind_srcs
	main.cpp
	app.cpp
	numlock.cpp
	switchuser.cpp
	png.c
	jpeg.c
)

set(mloginlock_srcs
	mloginlock.cpp
)

set(common_srcs
    cfg.cpp
    image.cpp
    log.cpp
    panel.cpp
    util.cpp
)
if(USE_PAM)
	set(common_srcs ${common_srcs} PAM.cpp)
    # for now, only build mloginlock if we are using PAM.
	if(BUILD_MLOGINLOCK)
		set(BUILD_MLOGINLOCK 1)
	endif(BUILD_MLOGINLOCK)
endif(USE_PAM)

# Build common library
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries")

if (BUILD_SHARED_LIBS)
	message(STATUS "Enable shared library building")
	add_library(libmlogin ${common_srcs})
else(BUILD_SHARED_LIBS)
	message(STATUS "Disable shared library building")
	add_library(libmlogin STATIC ${common_srcs})
endif(BUILD_SHARED_LIBS)

if(USE_CONSOLEKIT)
	set(mlogind_srcs ${mlogind_srcs} Ck.cpp)
endif(USE_CONSOLEKIT)

add_executable(${PROJECT_NAME} ${mlogind_srcs})
if(BUILD_MLOGINLOCK)
    add_executable(mloginlock ${mloginlock_srcs})
endif(BUILD_MLOGINLOCK)

#Set the custom CMake module directory where our include/lib finders are
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

find_package(X11 REQUIRED)
find_package(Freetype REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

# Fontconfig
set(FONTCONFIG_DIR ${CMAKE_MODULE_PATH})
find_package(FONTCONFIG REQUIRED)
if(FONTCONFIG_FOUND)
	message("\tFontConfig Found")
	target_link_libraries(${PROJECT_NAME} ${FONTCONFIG_LIBRARY})
	include_directories(${FONTCONFIG_INCLUDE_DIR})
endif(FONTCONFIG_FOUND)

# PAM
if(USE_PAM)
	message("\tPAM Enabled")
	find_package(PAM)
	if(PAM_FOUND)
		message("\tPAM Found")
		set(MLOGIND_DEFINITIONS ${MLOGIND_DEFINITIONS} "-DUSE_PAM")
		target_link_libraries(${PROJECT_NAME} ${PAM_LIBRARY})
		if(BUILD_MLOGINLOCK)
			target_link_libraries(mloginlock ${PAM_LIBRARY})
		endif(BUILD_MLOGINLOCK)
		include_directories(${PAM_INCLUDE_DIR})
	else(PAM_FOUND)
		message("\tPAM Not Found")
	endif(PAM_FOUND)
else(USE_PAM)
	message("\tPAM disabled")
endif(USE_PAM)

# ConsoleKit
if(USE_CONSOLEKIT)
	find_package(CkConnector)
	message("\tConsoleKit Enabled")
	if(CKCONNECTOR_FOUND)
		message("\tConsoleKit Found")
		# DBus check
		find_package(DBus REQUIRED)
		if(DBUS_FOUND)
			message("\tDBus Found")
			target_link_libraries(${PROJECT_NAME} ${DBUS_LIBRARIES})
			include_directories(${DBUS_ARCH_INCLUDE_DIR})
			include_directories(${DBUS_INCLUDE_DIR})
			set(MLOGIND_DEFINITIONS ${MLOGIND_DEFINITIONS} "-DUSE_CONSOLEKIT")
			target_link_libraries(${PROJECT_NAME} ${CKCONNECTOR_LIBRARIES})
			include_directories(${CKCONNECTOR_INCLUDE_DIR})
		else(DBUS_FOUND)
			message("\tDBus Not Found")
		endif(DBUS_FOUND)
	else(CKCONNECTOR_FOUND)
		message("\tConsoleKit Not Found")
		message("\tConsoleKit disabled")
	endif(CKCONNECTOR_FOUND)
else(USE_CONSOLEKIT)
	message("\tConsoleKit disabled")
endif(USE_CONSOLEKIT)

# system librarys
find_library(M_LIB m)
find_library(RT_LIB rt)
find_library(CRYPTO_LIB crypt)
find_package(Threads)

add_definitions(${MLOGIND_DEFINITIONS})

#Set up include dirs with all found packages
include_directories(
	${X11_INCLUDE_DIR}
	${X11_Xft_INCLUDE_PATH}
	${X11_Xrender_INCLUDE_PATH}
	${X11_Xrandr_INCLUDE_PATH}
	${FREETYPE_INCLUDE_DIR_freetype2}
	${X11_Xmu_INCLUDE_PATH}
	${ZLIB_INCLUDE_DIR}
	${JPEG_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
)

target_link_libraries(libmlogin
    ${JPEG_LIBRARIES}
	${PNG_LIBRARIES}
)

#Set up library with all found packages for mlogind
target_link_libraries(${PROJECT_NAME}
	${M_LIB}
	${RT_LIB}
	${CRYPTO_LIB}
	${X11_X11_LIB}
	${X11_Xft_LIB}
	${X11_Xrender_LIB}
	${X11_Xrandr_LIB}
	${X11_Xmu_LIB}
	${FREETYPE_LIBRARY}
	${JPEG_LIBRARIES}
	${PNG_LIBRARIES}
	util
    libmlogin
)

if(BUILD_MLOGINLOCK)
    #Set up library with all found packages for mloginlock
    target_link_libraries(mloginlock
        ${M_LIB}
        ${RT_LIB}
        ${CRYPTO_LIB}
        ${X11_X11_LIB}
        ${X11_Xft_LIB}
        ${X11_Xrender_LIB}
        ${X11_Xrandr_LIB}
        ${X11_Xmu_LIB}
        ${X11_Xext_LIB}
        ${FREETYPE_LIBRARY}
        ${JPEG_LIBRARIES}
        ${PNG_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        libmlogin
    )
endif(BUILD_MLOGINLOCK)

####### install
# mlogind
install(TARGETS mlogind RUNTIME DESTINATION bin)
if(BUILD_MLOGINLOCK)
	install(TARGETS mloginlock RUNTIME DESTINATION bin)
endif(BUILD_MLOGINLOCK)

if (BUILD_SHARED_LIBS)
	set_target_properties(libmlogin PROPERTIES
		OUTPUT_NAME mlogind
		SOVERSION ${MLOGIND_VERSION})

	install(TARGETS libmlogin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
endif (BUILD_SHARED_LIBS)

# man file
install(FILES mlogind.1 DESTINATION ${MANDIR}/man1/)
if(BUILD_MLOGINLOCK)
	install(FILES mloginlock.1 DESTINATION ${MANDIR}/man1/)
endif(BUILD_MLOGINLOCK)
# configure
install(FILES mlogind.conf.sample DESTINATION ${SYSCONFDIR})
# systemd service file
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	install(FILES mlogind.service DESTINATION ${LIBDIR}/systemd/system)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
# themes directory
subdirs(themes)
