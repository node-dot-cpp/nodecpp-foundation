cmake_minimum_required(VERSION 3.0)
project(libfoundation)

set(LIB_NAME foundation)
set(LIB_D_NAME foundation_d)
set(TEST_BIN_NAME test_foundation.bin)
set(TEST_D_BIN_NAME test_foundation_d.bin)

add_subdirectory(${PROJECT_SOURCE_DIR}/3rdparty/fmt)

set(SOURCE_LIB 			${PROJECT_SOURCE_DIR}/test/samples/file_error.cpp
						${PROJECT_SOURCE_DIR}/src/log.cpp 
					 	${PROJECT_SOURCE_DIR}/src/std_error.cpp 
					 	${PROJECT_SOURCE_DIR}/src/tagged_ptr_impl.cpp 
					 	${PROJECT_SOURCE_DIR}/src/safe_memory_error.cpp 
					 	${PROJECT_SOURCE_DIR}/src/cpu_exceptions_translator.cpp
						${PROJECT_SOURCE_DIR}/src/nodecpp_assert.cpp
						${PROJECT_SOURCE_DIR}/src/page_allocator.cpp
						${PROJECT_SOURCE_DIR}/src/tagged_ptr_impl.cpp
		)

include_directories(	${PROJECT_SOURCE_DIR}/include
					 	${PROJECT_SOURCE_DIR}/test 
					 	${PROJECT_SOURCE_DIR}/test/samples
					 	)

set(SOURCE_EXE  ${PROJECT_SOURCE_DIR}/test/main.cpp 
				${PROJECT_SOURCE_DIR}/test/test_seh.cpp  
				${PROJECT_SOURCE_DIR}/test/samples/file_error.cpp 
				)

if(MSVC OR MSYS OR MINGW)
	 add_definitions(/W3)
	 add_definitions(/EHa)
	 add_definitions(/MD)
else()
	enable_testing()
	add_definitions( -Wall )
	add_definitions(-fexceptions)
	add_definitions(-fnon-call-exceptions)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -fnon-call-exceptions")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS  " ${CMAKE_CXX_FLAGS}    -DNODECPP_CUSTOM_LOG_PROCESSING=\\\"${PROJECT_SOURCE_DIR}/test/my_logger.h\\\" " )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)

find_package( Threads )

add_library(${LIB_NAME} STATIC ${SOURCE_LIB})
add_library(${LIB_D_NAME} STATIC ${SOURCE_LIB})


add_executable(${TEST_BIN_NAME}  ${SOURCE_EXE})
add_executable(${TEST_D_BIN_NAME}  ${SOURCE_EXE})

set_target_properties ( ${LIB_NAME} PROPERTIES COMPILE_FLAGS  ${CMAKE_CXX_FLAGS_RELEASE})
set_target_properties ( ${LIB_D_NAME} PROPERTIES COMPILE_FLAGS  ${CMAKE_CXX_FLAGS_DEBUG} )

target_link_libraries( ${LIB_NAME} fmt::fmt)
target_link_libraries( ${LIB_D_NAME} fmt::fmt)

set_target_properties ( ${TEST_BIN_NAME} PROPERTIES COMPILE_FLAGS  ${CMAKE_CXX_FLAGS_RELEASE})
set_target_properties ( ${TEST_D_BIN_NAME}  PROPERTIES COMPILE_FLAGS  ${CMAKE_CXX_FLAGS_DEBUG})

target_link_libraries(${TEST_BIN_NAME} ${LIB_NAME}  ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${TEST_D_BIN_NAME} ${LIB_D_NAME}  ${CMAKE_THREAD_LIBS_INIT})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TEST_BIN_NAME})
if(UNIX)
	#add_test( Build_test  make ${TEST_BIN_NAME} )
	#add_test( Build_test_d  make ${TEST_D_BIN_NAME} )
	add_test( Run_test "${CMAKE_BINARY_DIR}/build/bin/${TEST_BIN_NAME}" )
	add_test( Run_test_d "${CMAKE_BINARY_DIR}/build/bin/${TEST_D_BIN_NAME}" )
endif(UNIX)
