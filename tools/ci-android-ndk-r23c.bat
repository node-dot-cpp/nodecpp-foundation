rmdir /S /Q build\android-ndk-r23c
mkdir build\android-ndk-r23c
cd build\android-ndk-r23c

rem env variable ANDROID_HOME should point to the root Sdk folder

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DANDROID_ABI=arm64-v8a -DANDROID_NDK=%ANDROID_HOME%\ndk\23.2.8568313 -DCMAKE_TOOLCHAIN_FILE=%ANDROID_HOME%\ndk\23.2.8568313\build\cmake\android.toolchain.cmake -G Ninja ..\..
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build .
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

adb devices
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

adb push test_foundation /data/local/tmp/
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

adb shell chmod +x /data/local/tmp/test_foundation
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

adb shell /data/local/tmp/test_foundation
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

