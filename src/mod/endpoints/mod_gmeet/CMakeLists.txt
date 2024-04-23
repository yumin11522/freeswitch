cmake_minimum_required(VERSION 3.16)
set(PROJECT_NAME "mod_xmpp")
project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)
if (UNIX)
	# https://blog.csdn.net/liwenlong_only/article/details/100696680
	set(CMAKE_CXX_FLAGS "-g -Wall -std=c++11 -Wl,--copy-dt-needed-entries ${ERIZO_CMAKE_CXX_FLAGS}")
endif ()
INCLUDE_DIRECTORIES(..)

set(my_lib_name ${PROJECT_NAME})

# generate version.c
configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/version.c.in
		${CMAKE_CURRENT_SOURCE_DIR}/version.c
		@ONLY
)

include(FetchContent)

FetchContent_Declare(
		gloox
		SVN_REVISION -r13755
		SVN_REPOSITORY https://svn2.genew.cn:18080/svn/numax/tool/gloox
		SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/gloox # 指定库下载地址
)
FetchContent_MakeAvailable(gloox)
target_include_directories(gloox PUBLIC ${gloox_SOURCE_DIR}/src ${gloox_SOURCE_DIR}/)

FetchContent_Declare(
		sdptransform
		GIT_REPOSITORY https://github.com/ibc/libsdptransform.git	# 仓库地址
		#        GIT_TAG a721f3a298c417941a090b501093d78b43653cf3 # 库版本
		GIT_TAG e33aba7005c563286b19a8c90b9520a4384cc259 # 库版本
		SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/sdptransform # 指定库下载地址
)
FetchContent_MakeAvailable(sdptransform)
target_include_directories(sdptransform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ext/sdptransform/include)

#add_definitions(-DNDEBUG)
#INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/libs/apr-util/include ${CMAKE_SOURCE_DIR}/libs/apr-util/include/private  ${CMAKE_SOURCE_DIR}/libs/apr/include)

SET (${PROJECT_NAME}_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/sdputil.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/jinglebridgesession.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/jingledescription.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/jinglegroup.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/jingletransport.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/jinglepresence.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/xmppconnection.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/logger.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/xmppclient.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp/xmppclientmgr.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp_client.cpp

		${CMAKE_CURRENT_SOURCE_DIR}/mod_xmpp.c
		${CMAKE_CURRENT_SOURCE_DIR}/xmpp_rtc.c
		${CMAKE_CURRENT_SOURCE_DIR}/version.c
)

#LINK_LIBRARIES ("-ldl -lpthread")

ADD_LIBRARY(${my_lib_name} SHARED ${${PROJECT_NAME}_SRCS})

#target_compile_options(${my_lib_name} PRIVATE -shared)
target_include_directories(${my_lib_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${my_lib_name} PUBLIC ${gloox_SOURCE_DIR}/src)

target_link_libraries(${my_lib_name} PUBLIC ${GCAS_SERVER_LIB} gloox sdptransform)

get_target_property(LIB_INCLUDE_DIRS ${my_lib_name} INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "${my_lib_name} include directories: ${LIB_INCLUDE_DIRS}")
get_target_property(LIB_LINK_LIBRARIES ${my_lib_name} LINK_LIBRARIES)
message(STATUS "${my_lib_name} Link Libraries: ${LIB_LINK_LIBRARIES}")

#add_custom_target(version.c ALL COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/gitversion.sh)

set_target_properties(${my_lib_name} PROPERTIES
		PREFIX ""
		LIBRARY_OUTPUT_NAME ${my_lib_name}
)

INSTALL ( TARGETS ${my_lib_name}
		LIBRARY				DESTINATION ${CMAKE_PREFIX_PATH}/lib/freeswitch/mod RUNTIME DESTINATION bin
)




