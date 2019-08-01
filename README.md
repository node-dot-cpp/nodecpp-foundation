# nodecpp-foundation
things valuable for a number of node-dot-cpp subprojects and outside
## Getting Started
Clone this repository with modules
```
git clone --recursive https://github.com/node-dot-cpp/nodecpp-foundation.git
```
Enter to the poject directory :
```
cd nodecpp-foundation
```
### Build project

#### Linux

To build library in Linux

**Debug and Release version**
Run in console 
```
cmake .
make
```
or
```
mkdir build
cd build 
cmake ..
```
And you will get both debug and release library in `nodecpp-foundation/build/lib` folder


If you want to build `test.bin` just run in console
```
make test_foundation.bin
```
If you want to build `test_foundation_d.bin` just run in console
```
make test_foundation_d.bin
```
All binaries will be located in `nodecpp-foundation/test/build/bin `

**Run tests**

Run in console
```
make test
```
It must passed 4 tests.



#### Windows Visual Studio
Run in Visual Studio command line
```
cmake -G "Visual Studio 15 2017 Win64" ..
```
This will generate Visual Studio project. 

**Release version**

To build Release version of library run in command prompt line
```
msbuild foundation_test.sln /p:Configuration=Release
```

**Debug version**

To build Debug version of library run in command prompt line
```
msbuild foundation_test.sln /p:Configuration=Debug
```