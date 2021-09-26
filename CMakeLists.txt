cmake_minimum_required(VERSION 3.5.1)

project(adb)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/openssl)

if (MSVC)
set(C11_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/c11
	${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/c11/c11)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/base)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/libcrypto_utils)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/libcutils)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/liblog)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/libbuildversion)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/diagnose_usb)
add_subdirectory(src)
