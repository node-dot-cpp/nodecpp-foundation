rm ./test.bin
g++ ../main.cpp ../test_seh.cpp ../../src/cpu_exceptions_translator.cpp -std=c++17 -Wall -fexceptions -fnon-call-exceptions -o test.bin
