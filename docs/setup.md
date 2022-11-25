# Windows clang setup

1. Download and install the following

- [visual studio](https://visualstudio.microsoft.com/)
- [git](https://git-scm.com/)
- [cmake](https://cmake.org/)


2. Clone the llvm source repository

> git clone https://github.com/llvm/llvm-project.git llvm


3. Use CMake to setup a build for llvm

> mkdir build
> cd build
> cmake -DLLVM_ENABLE_PROJECTS=clang -CMAKE_INSTALL_PREFIX=C:\llvm ..\llvm

**NOTE**: Make sure you set the **CMAKE_INSTALL_PREFIX** to something without spaces otherwise bad things will happen


4. build llvm

- select the **Release** configuration when building
- build the **ALL_BUILD** target to build the project
- build the **INSTALL** target to install clang/llvm onto your machine

5. set your path to include the bin directory in the llvm installation folder (e.g. `C:\llvm\bin`)

6. test your installation by opening up a new command line and typing in:

> clang-cl help

# Catch2 setup

Using vcpkg, make sure to install the 64 bit version if necessary

> vcpkg install Catch2:x64-windows

# Cmake build

When generating cmake build with vcpkg packages, make sure to use the correct build args

> cmake .. -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake

When using visual studio, make sure to switch off of the `Debug` configuration otherwise builds might not work

# Useful links

- https://llvm.org/docs/CMake.html#embedding-llvm-in-your-project