add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/curl-7.88.1)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.7.0)

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