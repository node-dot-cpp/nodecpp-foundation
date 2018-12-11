rm ./test.bin 
clang++ ../main.cpp ../test_seh.cpp ../../src/cpu_exceptions_translator.cpp -Wall -fexceptions -fnon-call-exceptions -std=c++17 -o test.bin
