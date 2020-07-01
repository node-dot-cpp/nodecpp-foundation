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
#include "stack_info_impl.h"
#include "log.h"

#ifdef NODECPP_TWO_PHASE_STACK_DATA_RESOLVING
#include <map>
#endif // NODECPP_TWO_PHASE_STACK_DATA_RESOLVING

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

#ifndef _GNU_SOURCE
#error _GNU_SOURCE must be defined to proceed
#endif

#include "llvm/DebugInfo/Symbolize/Symbolize.h"
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <dlfcn.h>

#endif // NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE

#else
#error not (yet) supported
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined NODECPP_LINUX && ( defined NODECPP_CLANG || defined NODECPP_GCC ) && NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE

static bool addrToModuleAndOffset( void* fnAddr, ModuleAndOffset& mao ) {
	Dl_info info;
	int ret = dladdr(fnAddr, &info); // see https://linux.die.net/man/3/dlopen for details
	if ( ret == 0 )
	{
		mao.modulePath = nullptr;
		mao.offsetInModule = 0;
		return false;
	}
	mao.modulePath = info.dli_fname;
	mao.offsetInModule = (uintptr_t)fnAddr - (uintptr_t)(info.dli_fbase);
	return true;
}

using namespace llvm;
using namespace symbolize;

template<typename T>
static bool error(Expected<T> &ResOrErr) {
	if (ResOrErr)
		return false;
	consumeError( ResOrErr.takeError() );
	return true;
}

static void addFileLineInfo( const char* ModuleName, uintptr_t Offset, bool addFnName, StackFrameInfo& sfi ) {
	LLVMSymbolizer Symbolizer;

	auto ResOrErr = Symbolizer.symbolizeInlinedCode( ModuleName, {Offset, object::SectionedAddress::UndefSection});

	if ( !error(ResOrErr) )
	{
		auto& info = ResOrErr.get();
		size_t numOfFrames = info.getNumberOfFrames();
		if ( numOfFrames == 0 )
			return;
		const DILineInfo & li = info.getFrame(0);

		sfi.functionName = li.FunctionName;
		sfi.srcPath = li.FileName;
		sfi.line = li.Line;
		sfi.column = li.Column;
	}
}

#endif // defined NODECPP_LINUX && ( defined NODECPP_CLANG || defined NODECPP_GCC ) && NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE

///////////////////////////////////////////////////////////////////////////////////////////////////

bool stackPointerToInfo( void* ptr, StackFrameInfo& info )
{
#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )

	static constexpr size_t TRACE_MAX_FUNCTION_NAME_LENGTH = 1024;
	HANDLE process = GetCurrentProcess();
	uint8_t symbolBuff[ sizeof(SYMBOL_INFO) + (TRACE_MAX_FUNCTION_NAME_LENGTH - 1) * sizeof(TCHAR) ];
	SYMBOL_INFO *symbol = (SYMBOL_INFO *)symbolBuff;
	symbol->MaxNameLen = TRACE_MAX_FUNCTION_NAME_LENGTH;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	DWORD displacement = 0;
	IMAGEHLP_LINE64 line;
	DWORD64 address = (DWORD64)(ptr);
	bool addrOK = false;
#ifndef _DEBUG
	addrOK = SymFromAddr(process, address, NULL, symbol);
#endif
	bool fileLineOK = SymGetLineFromAddr64(process, address, &displacement, &line);

	if ( fileLineOK )
	{
		info.srcPath = line.FileName;
		info.line = line.LineNumber;
	}
	if ( addrOK )
		info.functionName = symbol->Name;
	return true;
		
#elif defined NODECPP_CLANG || defined NODECPP_GCC

#ifdef NODECPP_LINUX_NO_LIBUNWIND

#ifdef NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE
	ModuleAndOffset mao;
	if ( addrToModuleAndOffset( ptr, mao ) )
	{
		addFileLineInfo( mao.modulePath, mao.offsetInModule/*, out*/, true, info );
		return true;
	}
	else
		return false;
#else
#error not applicable
#endif // NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE
#else
#error not applicable
#endif // NODECPP_LINUX_NO_LIBUNWIND

#else
#error not (yet) supported
#endif // platform/compiler
}

///////////////////////////////////////////////////////////////////////////////////////////////////



#define TRACE_MAX_STACK_FRAMES 1024

namespace nodecpp {

	void StackInfo::preinit()
	{
#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )

		void *stack[TRACE_MAX_STACK_FRAMES];
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		WORD numberOfFrames = CaptureStackBackTrace(1, TRACE_MAX_STACK_FRAMES, stack, NULL); // excluding current call itself
		stackPointers.init( stack, numberOfFrames );
		
#elif defined NODECPP_CLANG || defined NODECPP_GCC

#ifdef NODECPP_LINUX_NO_LIBUNWIND

		void *stack[TRACE_MAX_STACK_FRAMES];
		int numberOfFrames = backtrace( stack, TRACE_MAX_STACK_FRAMES );
		stackPointers.init( stack, numberOfFrames );

#else // NODECPP_LINUX_NO_LIBUNWIND // we do it in a single shot (at least, as for now)

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
				out += fmt::format( " ({}+0x{:x})\n", nameptr, offset );
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

	void StackInfo::postinit() const
	{
#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )

		void** stack = stackPointers.get();
		size_t numberOfFrames = stackPointers.size();
		std::string out;
		for (int i = 0; i < numberOfFrames; i++)
		{
			StackFrameInfo info;
			StackPointerInfoCache::getRegister().resolveData( stack[i], info );
			// info to string
			if ( info.functionName.size() && info.srcPath.size() )
				out += fmt::format( "\tat {} in {}, line {}\n", info.functionName, info.srcPath, info.line );
			else if ( info.srcPath.size() )
				out += fmt::format( "\tat {}, line {}\n", info.srcPath, info.line );
			else if ( info.functionName.size() )
				out += fmt::format( "\tat {}\n", info.functionName );
			else
				out += fmt::format( "\tat <...>\n" );
		}
		if ( !stripPoint.empty() )
			strip( out, stripPoint.c_str() );
		*const_cast<error::string_ref*>(&whereTaken) = out.c_str();
		
#elif defined NODECPP_CLANG || defined NODECPP_GCC

#ifdef NODECPP_LINUX_NO_LIBUNWIND
		void** stack = stackPointers.get();
		size_t numberOfFrames = stackPointers.size();
		char ** btsymbols = backtrace_symbols( stack, numberOfFrames );
		if ( btsymbols != nullptr )
		{
			std::string out;
			for (int i = 0; i < numberOfFrames; i++)
			{
				StackFrameInfo info;
				StackPointerInfoCache::getRegister().resolveData( stack[i], info );
				// info to string
				if ( info.functionName.size() && info.srcPath.size() )
					out += fmt::format( "\tat {} in {}, line {}\n", info.functionName, info.srcPath, info.line );
				else if ( info.srcPath.size() )
					out += fmt::format( "\tat {}, line {}\n", info.srcPath, info.line );
				else if ( info.functionName.size() )
					out += fmt::format( "\tat {}\n", info.functionName );
				else
					out += fmt::format( "\tat <...>\n" );
			}
			free( btsymbols );
			if ( !stripPoint.empty() )
				strip( out, stripPoint.c_str() );
			*const_cast<error::string_ref*>(&whereTaken) = out.c_str();
		}
		else
			*const_cast<error::string_ref*>(&whereTaken) = error::string_ref	( error::string_ref::literal_tag_t(), "" );
#else
// everything has already been done in preinit()
#endif // NODECPP_LINUX_NO_LIBUNWIND

#else
#error not (yet) supported
#endif // platform/compiler
	}


	namespace impl
	{
		extern const error::string_ref& whereTakenStackInfo( const StackInfo& info ) { 
#ifdef NODECPP_TWO_PHASE_STACK_DATA_RESOLVING
			if ( info.whereTaken.empty() )
				info.postinit();
#endif
			return info.whereTaken;
		}
		extern ::nodecpp::logging_impl::LoggingTimeStamp whenTakenStackInfo( const StackInfo& info ) { return info.timeStamp; }
		extern bool isDataStackInfo( const StackInfo& info ) {
#ifdef NODECPP_TWO_PHASE_STACK_DATA_RESOLVING
			return !(info.whereTaken.empty() && info.timeStamp.t == 0 && info.stackPointers.size() == 0);
#else
			return !(info.whereTaken.empty() && info.timeStamp.t == 0);
#endif
		}
	} // namespace impl

} //namespace nodecpp

#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
