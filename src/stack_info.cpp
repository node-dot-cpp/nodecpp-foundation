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

#include "stack_info.h"
#include "log.h"

#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )
#include <process.h>
#include <iostream>
#include <Windows.h>
#include "dbghelp.h"
#pragma comment(lib, "dbghelp.lib")
#define TRACE_MAX_STACK_FRAMES 1024
#define TRACE_MAX_FUNCTION_NAME_LENGTH 1024
#elif defined NODECPP_CLANG || defined NODECPP_GCC
#error not yet supported
#else
#error not (yet) supported
#endif

namespace nodecpp {

	void StackInfo::init_()
	{
#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )
		void *stack[TRACE_MAX_STACK_FRAMES];
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		WORD numberOfFrames = CaptureStackBackTrace(1, TRACE_MAX_STACK_FRAMES, stack, NULL); // excluding current call itself
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
#ifndef _DEBUG
			addrOK = SymFromAddr(process, address, NULL, symbol);
#endif
			bool fileLineOK = SymGetLineFromAddr64(process, address, &displacement, line);
			if ( addrOK && fileLineOK )
				out += fmt::format( "\tat {} in {}, line {}\n", symbol->Name, line->FileName, line->LineNumber );
			else if ( fileLineOK )
				out += fmt::format( "\tat {}, line {}\n", line->FileName, line->LineNumber );
			else if ( addrOK )
				out += fmt::format( "\tat {}\n", symbol->Name );
			else
				out += fmt::format( "\tat <...>\n" );
		}
		whereTaken = out.c_str();
#elif defined NODECPP_CLANG || defined NODECPP_GCC
#error not yet supported
#else
#error not (yet) supported
#endif
		timeStamp = ::nodecpp::log::logTime();
	}

	namespace impl
	{
		extern const error::string_ref& whereTakenStackInfo( const StackInfo& info ) { return info.whereTaken; }
		extern uint64_t whenTakenStackInfo( const StackInfo& info ) { return info.timeStamp; }
		extern bool isDataStackInfo( const StackInfo& info ) { return info.timeStamp != 0; }
	} // namespace impl

} //namespace nodecpp
