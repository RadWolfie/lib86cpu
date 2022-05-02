# Lib86cpu

"lib86cpu" is an open source library that emulates an Intel Pentium III processor, and exposes its functionality
via its own API. It uses a custom frontend to translate the guest code to LLVM IR, and then uses LLVM as the
backend to emit native code for the target platform.

## Building

Cmake version 3.4.3 or higher is required.\
Visual Studio 2022.\
Only Windows builds are supported for now.

**On Windows:**

1. `git clone --recurse-submodules https://github.com/ergo720/lib86cpu`  
2. `cmake -S "import\llvm\llvm" -B "llvm\build" -G "Visual Studio 17 2022" -A Win32 -Thost=x64 -DCMAKE_INSTALL_PREFIX="llvm\\\${BUILD_TYPE}" -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_INCLUDE_TOOLS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_BUILD_TOOLS=OFF -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_BUILD_TESTS=OFF -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_ENABLE_BINDINGS=OFF`
3. Build all the projects of the LLVM.sln solution file with Visual Studio
4. Build the INSTALL project (this copies all llvm libraries to llvm\build\${BUILD_TYPE} directory)
5. `cd` to the directory of lib86cpu
6. `mkdir build && cd build`
7. `cmake .. -G "Visual Studio 17 2022" -A Win32`
8. Build the resulting solution file lib86cpu.sln with Visual Studio

**NOTE:** use `cmake .. -G "Visual Studio 17 2022" -A Win32 -DLIB86CPU_BUILD_TEST=ON` if you want to also build the test app
