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

#ifndef NODECPP_NO_STACK_INFO_IN_EXCEPTIONS

#include "stack_info.h"
#include "log.h"

#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )

#include <process.h>
#include <iostream>
#include <Windows.h>
#include "dbghelp.h"
#pragma comment(lib, "dbghelp.lib")

#elif defined NODECPP_CLANG || defined NODECPP_GCC

#ifdef NODECPP_LINUX_NO_LIBUNWIND
#include <execinfo.h>
#else
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <cxxabi.h>
#endif // NODECPP_LINUX_NO_LIBUNWIND

#ifdef NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE

#include "llvm/DebugInfo/Symbolize/Symbolize.h"
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string>

static char path2thisExe[0x400];
static bool path2thisExeLoaded = false;
static const char* path2me() {
	if ( path2thisExeLoaded )
		return path2thisExe;

    int ret;

    ret = readlink("/proc/self/exe", path2thisExe, sizeof(path2thisExe)-1);
    if(ret ==-1)
		return "";
    path2thisExe[ret] = 0;
	path2thisExeLoaded = true;
printf( "Path of my EXE: %s\n\n", path2thisExe );
    return path2thisExe;
}

/*static uintptr_t baseAddrOfthisExe = 0;
static uintptr_t baseAddr()
{
	if ( baseAddrOfthisExe )
		return baseAddrOfthisExe;

    int ret;

    ret = readlink("/proc/self/exe", path2thisExe, sizeof(path2thisExe)-1);
    if(ret ==-1)
		return "";
    path2thisExe[ret] = 0;
	path2thisExeLoaded = true;
printf( "Path of my EXE: %s\n\n", path2thisExe );
    return path2thisExe;
}*/

#ifndef _GNU_SOURCE
#error _GNU_SOURCE must be defined to proceed
#endif

#include <dlfcn.h>

struct SInfo
{
	const char* modulePath;
	uintptr_t offsetInModule;
};

static bool addrToModuleAndOffset( void* fnAddr, SInfo& si ) {
	Dl_info info;
	int ret = dladdr(fnAddr, &info); // see https://linux.die.net/man/3/dlopen for details
	if ( ret == 0 )
	{
		si.modulePath = nullptr;
		si.offsetInModule = 0;
		return false;
	}
	si.modulePath = info.dli_fname;
	si.offsetInModule = (uintptr_t)fnAddr - (uintptr_t)(info.dli_fbase);
	return true;
}

using namespace llvm;
using namespace symbolize;

template<typename T>
static bool error(Expected<T> &ResOrErr) {
	if (ResOrErr)
		return false;
//	logAllUnhandledErrors(ResOrErr.takeError(), errs(), "LLVMSymbolizer: error reading file: ");
	consumeError( ResOrErr.takeError() );
	return true;
}

static void addFileLineInfo( const char* ModuleName, uintptr_t Offset, std::string& out, bool addFnName ) {
	LLVMSymbolizer Symbolizer;

	auto ResOrErr = Symbolizer.symbolizeInlinedCode( ModuleName, {Offset, object::SectionedAddress::UndefSection});

	if ( !error(ResOrErr) )
	{
		auto& info = ResOrErr.get();
		size_t numOfFrames = info.getNumberOfFrames();
		if ( numOfFrames == 0 )
			return;
		const DILineInfo & li = info.getFrame(0);
		if ( addFnName )
			out += fmt::format( "at {} in {}, line {}:{}\n", li.FunctionName.c_str(), li.FileName.c_str(), li.Line, li.Column );
		else
			out += fmt::format( " in {}, line {}:{}\n", li.FileName.c_str(), li.Line, li.Column );

		/*printf( "num of frames: %d\n", (int)(numOfFrames) );
		for ( size_t i=0; i<numOfFrames; ++i )
		{
			const DILineInfo & li = info.getFrame(i);
			printf( "  [%zd]\n", i );
			printf( "    file: %s\n", li.FileName.c_str() );
			printf( "    func: %s\n", li.FunctionName.c_str() );
			printf( "    line: %d\n", li.Line );
			printf( "    clmn: %d\n", li.Column );
			printf( "    s.l.: %d\n", li.StartLine );
		}*/
	}
}

#endif // NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE

#else
#error not (yet) supported
#endif

#define TRACE_MAX_STACK_FRAMES 1024
#define TRACE_MAX_FUNCTION_NAME_LENGTH 1024

namespace nodecpp {

	void StackInfo::init_( const char* stripPoint )
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
		if ( stripPoint != nullptr )
			strip( out, stripPoint );
		whereTaken = out.c_str();
		
#elif defined NODECPP_CLANG || defined NODECPP_GCC

#ifdef NODECPP_LINUX_NO_LIBUNWIND
		void *stack[TRACE_MAX_STACK_FRAMES];
		int numberOfFrames = backtrace( stack, TRACE_MAX_STACK_FRAMES );
		char ** btsymbols = backtrace_symbols( stack, numberOfFrames );
		if ( btsymbols != nullptr )
		{
			std::string out;
			for (int i = 0; i < numberOfFrames; i++)
			{
#ifdef NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE
				SInfo si;
				if ( addrToModuleAndOffset( stack[i], si ) )
				{
					out += fmt::format( "\tat {}", btsymbols[i] );
					addFileLineInfo( si.modulePath, si.offsetInModule, out, true );
				}
				else
					out += fmt::format( "\tat {}\n", btsymbols[i] );
#else
				out += fmt::format( "\tat {}\n", btsymbols[i] );
#endif // NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE
			}
			free( btsymbols );
			if ( stripPoint != nullptr )
				strip( out, stripPoint );
			whereTaken = out.c_str();
		}
		else
			whereTaken = error::string_ref	( error::string_ref::literal_tag_t(), "" );
#else
		unw_cursor_t cursor;
		unw_context_t context;

		// Initialize cursor to current frame for local unwinding.
		unw_getcontext(&context);
		unw_init_local(&cursor, &context);

		// Unwind frames one by one, going up the frame stack.

		std::string out;

		while (unw_step(&cursor) > 0) {
			unw_word_t offset, pc;
			unw_get_reg(&cursor, UNW_REG_IP, &pc);
			if (pc == 0) {
				break;
			}
			out += fmt::format( "0x{:x}:", pc );

			char sym[256];
			if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
				char* nameptr = sym;
				int status;
				char* demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
				if (status == 0) {
					nameptr = demangled;
				}
#ifdef NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE
				SInfo si;
				if ( addrToModuleAndOffset( pc + offset, si ) )
				{
					out += fmt::format( " ({}+0x{:x})", nameptr, offset );
					addFileLineInfo( si.modulePath, si.offsetInModule, out, true );
				}
				else
					out += fmt::format( " ({}+0x{:x})\n", nameptr, offset );
#else
				out += fmt::format( " ({}+0x{:x})\n", nameptr, offset );
#endif // NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE
				std::free(demangled);
			} else {
				out += "<...>\n";
			}
		}
		if ( stripPoint != nullptr )
			strip( out, stripPoint );
		whereTaken = out.c_str();
#endif // NODECPP_LINUX_NO_LIBUNWIND

#else
#error not (yet) supported
#endif // platform/compiler
		timeStamp = ::nodecpp::logging_impl::getCurrentTimeStamp();
	}

	namespace impl
	{
		extern const error::string_ref& whereTakenStackInfo( const StackInfo& info ) { return info.whereTaken; }
		extern ::nodecpp::logging_impl::LoggingTimeStamp whenTakenStackInfo( const StackInfo& info ) { return info.timeStamp; }
		extern bool isDataStackInfo( const StackInfo& info ) { return !(info.whereTaken.empty() && info.timeStamp.t == 0); }
	} // namespace impl

} //namespace nodecpp

#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
