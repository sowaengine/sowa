file(GLOB_RECURSE RUNNER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(runner ${RUNNER_SOURCES})
set_target_properties(runner
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../pkg/"
)
set_target_properties(runner PROPERTIES OUTPUT_NAME "sowa")

target_link_libraries(
    runner
        PRIVATE
        -static-libgcc
        -static-libstdc++
        CURL::libcurl
        yaml-cpp
        zip
    )
target_include_directories(runner
    PRIVATE
        ${CMAKE_SOURCE_DIR}/thirdparty/curl-7.88.1/include
        ${CMAKE_SOURCE_DIR}/Sowa/App/src
        ${YAML_CPP_INCLUDE}
        ${LIBZIP_INCLUDE}
)

install(TARGETS runner RUNTIME DESTINATION .)