rm ./test.bin
g++ ../main.cpp ../test_seh.cpp ../../src/log.cpp ../../3rdparty/fmt/src/format.cc ../../src/cpu_exceptions_translator.cpp -I../../3rdparty/fmt/include -DNODECPP_CUSTOM_LOG_PROCESSING="\"../test/my_logger.h\"" -std=c++17 -Wall -fexceptions -fnon-call-exceptions -o test.bin
