# Windows clang setup

1. Download and install the following

- [visual studio](https://visualstudio.microsoft.com/)
- [git](https://git-scm.com/)


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