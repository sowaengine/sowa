set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/modules)

add_library(Ease.Core SHARED Ease.Core.cc)
set_target_properties(Ease.Core PROPERTIES PREFIX "")


add_library(Ease.Editor SHARED Ease.Editor.cc)
set_target_properties(Ease.Editor PROPERTIES PREFIX "")