/* -------------------------------------------------------------------------------
* Copyright (c) 2018, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#include <stdio.h>
#include <utility> // TODO: move it to a proper place
#include <assert.h> // TODO: replace by ouw own assertion system

#include "../include/foundation.h"
#include "../include/assert.h"
#include "test.h"

void printPlatform()
{
#if defined NODECPP_CLANG
	printf( "Compiler: clang\n" );
#elif defined NODECPP_GCC
	printf( "Compiler: gcc\n" );
#elif defined NODECPP_MSVC
	printf( "Compiler: msvcv\n" );
#else
	printf( "Compiler: unknown\n" );
#endif

#if defined NODECPP_X64
	printf( "64 bit\n" );
#elif defined NODECPP_X86
	printf( "32 bit\n" );
#else
	printf( "unknown platform\n" );
#endif

#if defined NODECPP_LINUX
	printf( "OS: Linux\n" );
#elif (defined NODECPP_WINDOWS )
	printf( "OS: Windows\n" );
#else
	printf( "OS: unknown\n" );
#endif
	printf( "Minimum CPU page size: %d bytes\n", NODECPP_MINIMUM_CPU_PAGE_SIZE );
	printf( "Minimum Zero Guard page size: %d bytes\n", NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE );
}

void fnWithAssertion(int i)
{
	NODECPP_ASSERT( 0, nodecpp::assert::AssertLevel::critical, i>0, "i = {}", i );
}

#include "../include/exception.h"

int fnThatThrows( int n )
{
	if ( n > 0 )
		return n;
	switch ( n )
	{
		case - 1: throw nodecpp::exception::file_error(nodecpp::exception::FILE_EXCEPTION::dne, "some_file" ); break;
		/*case -2: throw nodecpp::exception::file_exception_access_denied__; break;
		case -3: throw nodecpp::exception::memory_exception_memory_access_violation__; break;
		case -4: throw nodecpp::exception::memory_exception_null_pointer_access__; break;
		case -5: throw nodecpp::exception::memory_exception_out_of_bound__; break;*/
	}
	return 1;
}

void fnThatCatches()
{
	int ret = fnThatThrows(1);
	nodecpp::log::log<0, nodecpp::log::LogLevel::info>("fnThatThrows(1) = {}", ret);
	ret = 0;
	try
	{
		ret = fnThatThrows(-1);
		nodecpp::log::log<0, nodecpp::log::LogLevel::info>("fnThatThrows(-1): OK, ret = {}", ret);
	}
	catch (nodecpp::exception::error e)
	{
		nodecpp::log::log<0, nodecpp::log::LogLevel::info>("exception caught; e.name = {}, e.description = {}", e.name(), e.description().c_str() );
//		if ( e.
		ret = 0;
	}
	nodecpp::log::log<0, nodecpp::log::LogLevel::info>("fnThatCatches(), ret = {}", ret);
}

int main(int argc, char *argv[])
{
	fnThatCatches(); return 0;
	fnWithAssertion(1);
	try
	{
		fnWithAssertion(0);
	}
	catch (...)
	{
		nodecpp::log::log<0, nodecpp::log::LogLevel::info>("error cought!");
	}


	const char* testMsg = "some long message";
	int fake = 17;
	nodecpp::log::log<0, nodecpp::log::LogLevel::info>("[1] Hi! msg = \'{}\', fake = {} <end>", testMsg, fake );
	nodecpp::log::log<1, nodecpp::log::LogLevel::info>("[2] Hi! msg = \'{}\', fake = {} <end>", testMsg, fake );
	nodecpp::log::log<2, nodecpp::log::LogLevel::verbose>("[3] Hi! msg = \'{}\', fake = {} <end>", testMsg, fake );
	nodecpp::log::log<2, nodecpp::log::LogLevel::error>("[4] Hi! msg = \'{}\', fake = {} <end>", testMsg, fake );
	printPlatform();
	printf( "\n" );
	testSEH();
    return 0;
}
