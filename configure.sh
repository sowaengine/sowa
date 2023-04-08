cmake -S thirdparty/manual/zlib-1.2.13 -B build/thirdparty/zlib -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=bin/zlib

cmake -S . -B build/sowa/ -DBUILD_SHARED_LIBS=OFF