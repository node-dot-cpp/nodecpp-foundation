rem script to build with Visual Studio


mkdir build
cd build

cmake -G "Visual Studio 15 2017 Win64" ..
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ctest --output-on-failure -C Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%
