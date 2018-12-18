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

int main(int argc, char *argv[])
{
	nodecpp::log::log<0, nodecpp::log::LogLevel::Notice>("[1] Hi!" );
	nodecpp::log::log<1, nodecpp::log::LogLevel::Notice>("[2] Hi!" );
	nodecpp::log::log<2, nodecpp::log::LogLevel::Notice>("[3] Hi!" );
	nodecpp::log::log<2, nodecpp::log::LogLevel::Error>("[4] Hi!" );
	printPlatform();
	printf( "\n" );
	testSEH();
    return 0;
}
