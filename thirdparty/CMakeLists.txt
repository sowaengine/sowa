add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/curl-7.88.1)

set(
    CURL_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/curl-7.88.1/include

    PARENT_SCOPE
)