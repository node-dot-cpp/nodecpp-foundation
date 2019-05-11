cmake_minimum_required(VERSION 3.0)
project(libfoundation)

set(LIB_NAME foundation)
set(LIB_NAME foundation)
set(LIB_D_NAME foundation_d)
set(TEST_BIN_NAME test_foundation.bin)
set(TEST_D_BIN_NAME test_foundation_d.bin)

file(GLOB SOURCE_LIB 	"src/log.cpp",
					 	"src/cpu_exceptions_translator.cpp",
					 	"src/std_error.cpp",
					 	"src/safe_memory_error.cpp",
					 	"src/tagged_ptr_impl.cpp"
					 	)

set(SOURCE_LIB ${SOURCE_LIB} 3rdparty/fmt/src/format.cc  test/samples/file_error.cpp )
include_directories(	${PROJECT_SOURCE_DIR}/include
					 	${PROJECT_SOURCE_DIR}/3rdparty/fmt/include 
					 	${PROJECT_SOURCE_DIR}/test 
					 	${PROJECT_SOURCE_DIR}/test/samples
					 	)

set(SOURCE_EXE  ${PROJECT_SOURCE_DIR}/test/main.cpp 
				${PROJECT_SOURCE_DIR}/test/test_seh.cpp  
				${PROJECT_SOURCE_DIR}/test/samples/file_error.cpp 
				)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2  -DNDEBUG")
set(CMAKE_CXX_FLAGS  " ${CMAKE_CXX_FLAGS} -DNODECPP_CUSTOM_LOG_PROCESSING=\\\"${PROJECT_SOURCE_DIR}/test/my_logger.h\\\" " )

if(MSVC OR MSYS OR MINGW)
	 add_definitions(/W3)
	 add_definitions(/EHa)
else()
	enable_testing()
	add_definitions( -Wall )
	add_definitions(-fexceptions)
	add_definitions(-fnon-call-exceptions)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -fnon-call-exceptions")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/bin)
add_library(${LIB_NAME} STATIC ${SOURCE_LIB})
add_library(${LIB_D_NAME} STATIC ${SOURCE_LIB})


add_executable(${TEST_BIN_NAME} EXCLUDE_FROM_ALL ${SOURCE_EXE})
add_executable(${TEST_D_BIN_NAME} EXCLUDE_FROM_ALL ${SOURCE_EXE})

set_target_properties ( ${LIB_NAME} PROPERTIES COMPILE_FLAGS  ${CMAKE_CXX_FLAGS_RELEASE})
set_target_properties ( ${LIB_D_NAME} PROPERTIES COMPILE_FLAGS  ${CMAKE_CXX_FLAGS_DEBUG} )

set_target_properties ( ${TEST_BIN_NAME} PROPERTIES COMPILE_FLAGS  ${CMAKE_CXX_FLAGS_RELEASE})
set_target_properties ( ${TEST_D_BIN_NAME}  PROPERTIES COMPILE_FLAGS  ${CMAKE_CXX_FLAGS_DEBUG})

target_link_libraries(${TEST_BIN_NAME} ${LIB_NAME})
target_link_libraries(${TEST_D_BIN_NAME} ${LIB_D_NAME})

if(UNIX)
	add_test( Build_test  make ${TEST_BIN_NAME} )
	add_test( Builf_test_d  make ${TEST_D_BIN_NAME} )
	add_test( Run_test "${PROJECT_SOURCE_DIR}/build/bin/${TEST_BIN_NAME}" )
	add_test( Rub_test_d "${PROJECT_SOURCE_DIR}/build/bin/${TEST_D_BIN_NAME}" )
endif(UNIX)
