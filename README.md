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

**Release version**
Run in console 
```
cmake .
make
```
And you will get library in `nodecpp-foundation/build/release/lib` folder

If you want to build `test.bin` just run in console
```
make test.bin
```
`test.bin` will be located in `nodecpp-foundation/test/build/release `

**Debug version**

Run in console 
```
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```
And you will get library in `nodecpp-foundation/build/debug/lib` folder

If you want to build `test.bin` just run in console
```
make test.bin
```

`test_debug.bin` will be located in `nodecpp-foundation/test/build/debug `

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