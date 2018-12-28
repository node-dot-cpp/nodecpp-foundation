rm ./test.bin 
g++ ../main.cpp ../test_seh.cpp ../../3rdparty/fmt/src/format.cc ../../src/log.cpp ../../src/cpu_exceptions_translator.cpp ../../src/std_error.cpp ../samples/file_error.cpp ../samples/safe_memory_error.cpp -I../../3rdparty/fmt/include -DNODECPP_CUSTOM_LOG_PROCESSING="\"../test/my_logger.h\"" -std=c++17 -Wall -fexceptions -fnon-call-exceptions -o test.bin 
