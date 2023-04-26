cmake --build build/thirdparty/nmres --parallel 4

./build/thirdparty/nmres/nmres --recursive --cwd Sowa --namespace sowa::Res --suffix .res.hpp --rules res_rules.txt