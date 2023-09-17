Rendering Engine
========================

This Rendering Engine provides simple lighting and PBR.

```
##########################################################
# setup for pushing repo
##########################################################

cd lib
git submodule add https://github.com/microsoft/DirectXTK12
git submodule add https://github.com/assimp/assimp

cd DirectXTK12
git checkout tags/sept2023
cd ..

cd assimp
git checkout tags/v5.2.5
cd ..

##########################################################
# setup after cloning the project
##########################################################

git clone --recurse-submodules https://github.com/knutella/retest
# git submodule init
# git submodule update

# create and enter the directory to do build
mkdir build
cd build

# create a solution for building
cmake ..
```