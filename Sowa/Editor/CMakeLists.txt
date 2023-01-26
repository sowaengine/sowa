file(GLOB_RECURSE EDITOR_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)

set(LD_FLAG_STATIC "")

if(WIN32)
  set(LD_FLAG_STATIC -static)
endif()

add_executable(editor ${EDITOR_SOURCES})
target_link_libraries(editor PRIVATE sowa -static-libgcc -static-libstdc++
                                     ${LD_FLAG_STATIC} ${CMAKE_DL_LIBS})
target_include_directories(editor PRIVATE ${SOWA_INCLUDE_DIRS})

if(CMAKE_BUILD_TYPE MATCHES Release)
  target_compile_options(editor PRIVATE -O2)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_options(editor PRIVATE -O0 -g)
endif()

# Platform detection
if(WIN32)
  target_compile_definitions(editor PRIVATE SW_WINDOWS)
elseif(UNIX AND NOT APPLE)
  target_compile_definitions(editor PRIVATE SW_LINUX)
else()
  message(FATAL_ERROR "Sowa Editor is not supported on target platform "
                      ${CMAKE_SYSTEM})
endif()
