if(NOT ${TARGET_PLATFORM} STREQUAL "Web")
  set(SOWA_THIRDPARTY_INCLUDES
      ${SOWA_THIRDPARTY_INCLUDES}
      "${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.3.8/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")

  set(SOWA_THIRDPARTY_SOURCES ${SOWA_THIRDPARTY_SOURCES}
                              "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c"

                              # angelscript addons
                              "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/add_on/scriptstdstring/scriptstdstring.cpp"
                              "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/add_on/scriptbuilder/scriptbuilder.cpp"
                              )

  set(GLFW_BUILD_EXAMPLES OFF)
  set(GLFW_BUILD_TESTS OFF)
  set(GLFW_BUILD_DOCS OFF)
  set(GLFW_INSTALL OFF)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.3.8" EXCLUDE_FROM_ALL)
endif()

set(SOWA_THIRDPARTY_INCLUDES
    ${SOWA_THIRDPARTY_INCLUDES}
    "${CMAKE_CURRENT_SOURCE_DIR}/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/tomlplusplus-3.3.0"
    "${CMAKE_CURRENT_SOURCE_DIR}/eventpp-0.1.2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/glm-0.9.9.8/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.13.1/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/lunasvg-2.3.8/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.8.0/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/angelscript/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/add_on"
)

set(SOWA_THIRDPARTY_INCLUDES
    ${SOWA_THIRDPARTY_INCLUDES}
    PARENT_SCOPE)
set(SOWA_THIRDPARTY_SOURCES
    ${SOWA_THIRDPARTY_SOURCES}
    PARENT_SCOPE)


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/eventpp-0.1.2" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.13.1" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lunasvg-2.3.8" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.8.0" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/angelscript/projects/cmake" EXCLUDE_FROM_ALL)