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
#include <stdexcept>

#include "../include/cpu_exceptions_translator.h"
#include "../include/log.h"

using namespace std;

class TestBase
{
	int n1;
public:
	TestBase( int k ) { n1 = k; nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "TestBase::TestBase({})", k ); }
	virtual ~TestBase() { nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "TestBase::~TestBase()" ); }

};

class Test : public TestBase
{
	int n2;
public:
	Test( int k ) : TestBase(k+1) { n2 = k;  nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "Test::Test({})", k ); }
	virtual ~Test() { nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "Test::~Test()" ); }

};

void badCallInner_Nullptr()
{
	volatile int * ptr = nullptr;
    *ptr = 0;
}

void badCallOuter_Nullptr()
{
	TestBase tb( 1 );
	badCallInner_Nullptr();
	nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "we must be dead here" );
}

void badCallInner_AnyPtr()
{
	volatile int * ptr = (int*) 0x100000;
    *ptr = 0;
}

void badCallOuter_AnyPtr()
{
	TestBase tb( 10 );
	badCallInner_AnyPtr();
	nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "we must be dead here" );
}

void badCallInner_DivByZero()
{
	volatile int a = 0;
	volatile int b = 3 / a;
	nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "{}", b ); // dummy call to avoid optimizing out
}

void badCallOuter_DivByZero()
{
	TestBase tb( 100 );
	badCallInner_DivByZero();
	nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "we must be dead here" );
}

void testSEH()
{
    initTranslator();

    try
    {
		badCallOuter_Nullptr();
    }
    catch (std::exception& e)
    {
        nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "{}", e.what() );
    }

    nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "... and still working [1]" );

    try
    {
		badCallOuter_AnyPtr();
	}
    catch (std::exception& e)
    {
        nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "{}", e.what() );
    }

    nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "... and still working [2]" );
	
    try 
	{
		badCallOuter_DivByZero();
	}
	catch( std::exception& e )
	{
		nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "{}", e.what() );
	}

    nodecpp::log::log<0, nodecpp::log::LogLevel::info>( "... and still working [3]" );
}
