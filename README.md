# Sowa Engine

<img src="https://raw.githubusercontent.com/Lexographics/Easengine/main/.gh_resources/icon-1024x.png" width="200" alt="Sowa Engine Logo"><br><br>


**Sowa Engine** is a 2D Game/Rendering Engine


## Notes
   This software is not in usable state yet.
   
## Building
- After cloning the repository, update submodules <br>
```git submodule update --init --recursive --remote```

- Run [nmres](https://github.com/Lexographics/nmResource) in engine root directory   
```nmres --recursive --cwd Ease/ --namespace Ease::Res --suffix .res.hpp --rules res_rules.txt```

- Run following commands to initialize and build cmake project
```
    mkdir build/
    cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Release
    cmake --build build/   # --parallel (count) argument can be passed to run multiple instances of compilers
```