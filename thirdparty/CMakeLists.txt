set(BUILD_CURL_EXE OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/curl-7.88.1)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.7.0)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libzip-1.9.2)

set(
    CURL_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/curl-7.88.1/include

    PARENT_SCOPE
)


set(
    YAML_CPP_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.7.0/include

    PARENT_SCOPE
)

set(
    LIBZIP_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/libzip-1.9.2/lib

    PARENT_SCOPE
)