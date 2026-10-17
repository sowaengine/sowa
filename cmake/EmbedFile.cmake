# Convert a binary asset to a C++ byte array at build time. No external tool or
# runtime source-tree path is needed; dependent targets link the generated data.
foreach(required INPUT OUTPUT SYMBOL)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "EmbedFile requires ${required}")
  endif()
endforeach()
file(READ "${INPUT}" bytes HEX)
string(REGEX REPLACE "(..)" "0x\\1," bytes "${bytes}")
string(REGEX REPLACE "(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n" bytes "${bytes}")
get_filename_component(output_dir "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
file(WRITE "${OUTPUT}" "// Generated asset; do not edit.\n#include \"assets/embedded.hpp\"\nnamespace sowa::assets {\nnamespace {\nalignas(4) const unsigned char data[] = {\n${bytes}\n};\n}\nstd::span<const unsigned char> ${SYMBOL}() { return data; }\n}\n")
