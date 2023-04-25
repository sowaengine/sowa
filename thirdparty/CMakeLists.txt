set(BUILD_CURL_EXE OFF)

if(UNIX AND NOT APPLE)
    set(ZLIB_LIBRARY ${CMAKE_SOURCE_DIR}/bin/zlib/lib/libz.a)
else()
    set(ZLIB_LIBRARY ${CMAKE_SOURCE_DIR}/bin/zlib/lib/libzlibstatic.a)
endif()

set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/bin/zlib/include/ CACHE PATH "zlib include dir")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/curl-7.88.1)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.7.0)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libzip-1.9.2)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.0/sdk/angelscript/projects/cmake)

add_library(tinyfiledialogs "${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs/tinyfiledialogs.c")

set(
    CURL_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/curl-7.88.1/include/

    PARENT_SCOPE
)


set(
    YAML_CPP_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.7.0/include/

    PARENT_SCOPE
)

set(TINYFD_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs/
    
    PARENT_SCOPE
)

set(
    LIBZIP_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/libzip-1.9.2/lib/

    PARENT_SCOPE
)


set(
    ANGELSCRIPT_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.0/sdk/angelscript/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.0/sdk/
    
    PARENT_SCOPE
    )