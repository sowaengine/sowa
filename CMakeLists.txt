cmake_minimum_required(VERSION 3.0.0)
project(Sowa VERSION 0.1.1 LANGUAGES CXX C)


# if(CMAKE_BUILD_TYPE STREQUAL "")
#    set(CMAKE_BUILD_TYPE Debug)
#    message("No CMAKE_BUILD_TYPE specified. Selected 'Debug'")
#    message("Possible values: Release | Debug")
# endif()



# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

# set(CMAKE_C_STANDARD 17)
# set(CMAKE_C_STANDARD_REQUIRED ON)
# set(CMAKE_C_EXTENSIONS OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/Sowa)