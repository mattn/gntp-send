cmake_minimum_required( VERSION 2.6 )


option(WITH_STATIC    "Build static growl library"           OFF)

include_directories ( ./headers )

if( WIN32 )
    link_libraries ( ws2_32 )
endif( WIN32 )

set( GROWL_NTP_C_SRC 
	source/growl.c
	source/tcp.c
	source/md5.c )

add_library( growl SHARED  ${GROWL_NTP_C_SRC} )
set_target_properties( growl PROPERTIES COMPILE_FLAGS "-DGROWL_DLL" )

if( WITH_STATIC )
add_library( growl-static  STATIC ${GROWL_NTP_C_SRC} )
endif( WITH_STATIC )

add_library( growl++ SHARED source/growl++.cpp )
target_link_libraries ( growl++  growl)
set_target_properties( growl++ PROPERTIES COMPILE_FLAGS "-DGROWL_CPP_DLL" )



add_executable( gntp-send source/gntp-send.c )
target_link_libraries ( gntp-send growl)

add_executable( gntp-send++ source/gntp-send++.cpp )
target_link_libraries ( gntp-send++ growl growl++)

