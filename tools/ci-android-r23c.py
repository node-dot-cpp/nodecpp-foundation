import os
import sys
import shutil
import subprocess

def build(buildType):
    path = "build/android-r23c"
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path)
    os.chdir(path)
    # currCwd = os.getcwd()

    androidHome = os.getenv('ANDROID_HOME')
    commandLine = (
        "cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=" + buildType +
        " -DANDROID_ABI=arm64-v8a -DANDROID_NDK=" + androidHome + "/ndk/23.2.8568313" +
        " -DCMAKE_TOOLCHAIN_FILE=" + androidHome + "/ndk/23.2.8568313/build/cmake/android.toolchain.cmake" +
        " -G Ninja -S ../.."
    )
    subprocess.run(commandLine, shell=True, check=True)
    subprocess.run("cmake --build .", shell=True, check=True)

def test():
    subprocess.run("adb devices", shell=True, check=True)
    subprocess.run("adb push test_foundation /data/local/tmp", shell=True, check=True)
    subprocess.run("adb shell chmod +x /data/local/tmp/test_foundation", shell=True, check=True)
    subprocess.run("adb shell /data/local/tmp/test_foundation", shell=True, check=True)



buildType = 'Release'

if len(sys.argv) <= 1:
    print('Build Type not set. Release configuration used as a default')
    buildType = 'Release'
else:
    for i in range(1, len(sys.argv)):
        if (sys.argv[i] == "Release"):
            print("Release configuration is set")
            buildType = 'Release'
        elif (sys.argv[i] == "Debug"):
            print("Debug configuration is set")
            buildType = 'Debug'
        else:
            print("Unexpected configuration [" + sys.argv[i] +
                  "]. Only [Release, Debug] configurations are allowed. Terminate")
            exit()

build(buildType)
test()

