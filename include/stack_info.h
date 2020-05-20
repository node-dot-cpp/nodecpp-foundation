/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
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

#ifndef NODECPP_STACK_INFO_H
#define NODECPP_STACK_INFO_H

#include "platform_base.h"
#include "foundation.h"
#include "error.h"
#include "log.h"
#include <fmt/format.h>

#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )
#include <process.h>
#include <iostream>
#include <Windows.h>
#include "dbghelp.h"
#pragma comment(lib, "dbghelp.lib")
#define TRACE_MAX_STACK_FRAMES 1024
#define TRACE_MAX_FUNCTION_NAME_LENGTH 1024
#elif defined NODECPP_CLANG || defined NODECPP_GCC
#else
#error not (yet) supported
#endif

namespace nodecpp {

	class StackInfo
	{
#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )
		uint64_t timeStamp = 0;
		error::string_ref whereTaken;
		void init_()
		{
			void *stack[TRACE_MAX_STACK_FRAMES];
			HANDLE process = GetCurrentProcess();
			SymInitialize(process, NULL, TRUE);
			WORD numberOfFrames = CaptureStackBackTrace(0, TRACE_MAX_STACK_FRAMES, stack, NULL);
			SYMBOL_INFO *symbol = (SYMBOL_INFO *)malloc(sizeof(SYMBOL_INFO) + (TRACE_MAX_FUNCTION_NAME_LENGTH - 1) * sizeof(TCHAR));
			symbol->MaxNameLen = TRACE_MAX_FUNCTION_NAME_LENGTH;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			DWORD displacement = 0;
			IMAGEHLP_LINE64 *line = (IMAGEHLP_LINE64 *)malloc(sizeof(IMAGEHLP_LINE64));
			memset(line, 0, sizeof(IMAGEHLP_LINE64));
			line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			std::string out;
			for (int i = 0; i < numberOfFrames; i++)
			{
				DWORD64 address = (DWORD64)(stack[i]);
				bool addrOK = false;
#ifndef DEBUG_
				addrOK = SymFromAddr(process, address, NULL, symbol);
#endif
				bool fileLineOK = SymGetLineFromAddr64(process, address, &displacement, line);
				if ( addrOK && fileLineOK )
					out += fmt::format( "\tat {} in {}, line {}\n", symbol->Name, line->FileName, line->LineNumber );
				else if ( fileLineOK )
					out += fmt::format( "\tat {}, line {}\n", symbol->Name, line->FileName, line->LineNumber );
				else if ( addrOK && fileLineOK )
					out += fmt::format( "\tat {}\n", symbol->Name );
				else
					out += fmt::format( "\tat <...>\n" );
			}
			whereTaken = out.c_str();
#elif defined NODECPP_CLANG || defined NODECPP_GCC
#else
#error not (yet) supported
#endif
		}
		const char* c_str() { return whereTaken.c_str(); } // for internal purposes only: this value is non-deterministic and cannot be used except as for (future) logging

	public:
		StackInfo() : whereTaken( "" ) {}
		StackInfo( bool doInit ) : whereTaken( error::string_ref::literal_tag_t(), "" ) { if ( doInit ) init_(); }
		StackInfo( const StackInfo& other ) = default;
		StackInfo& operator = ( const StackInfo& other ) = default;
		StackInfo( StackInfo&& other ) = default;
		StackInfo& operator = ( StackInfo&& other ) = default;
		virtual ~StackInfo() {}
		void init() { init_(); }
		void log( log::LogLevel l) { log::default_log::log( l, "{}", whereTaken.c_str() ); }
		void log( log::Log targetLog, log::LogLevel l) { targetLog.log( l, "{}", whereTaken.c_str() ); }
	};

} //namespace nodecpp

#endif // NODECPP_STACK_INFO_H
