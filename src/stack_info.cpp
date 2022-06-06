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

#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )

#include <process.h>
#include <iostream>
#include <Windows.h>
#include "dbghelp.h"
#pragma comment(lib, "dbghelp.lib")

#elif defined(NODECPP_LINUX) || defined(NODECPP_MAC)

#include <execinfo.h>
#include <unistd.h>
#include <dlfcn.h>

#elif defined(NODECPP_ANDROID)

#include <unwind.h>
#include <dlfcn.h>

#else
#error not (yet) supported
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(NODECPP_LINUX) || defined(NODECPP_MAC)

static std::string sh(std::string cmd) {
	static constexpr size_t buffSz = 128;
	char buffer[buffSz+1];
	std::string result;
	std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
	if (!pipe) return result; // with no data
	while (!feof(pipe.get())) {
		if (fgets(buffer, buffSz, pipe.get()) != nullptr) {
			buffer[buffSz] = 0;
			result += buffer;
		}
	}
    return result;
}

#endif

// #if defined(NODECPP_LINUX) && defined(NODECPP_CLANG)

// static void parseBtSymbol( std::string symbol, nodecpp::stack_info_impl::StackFrameInfo& info ) {
// 	if ( symbol.size() == 0 )
// 		return;
// 	size_t pos = symbol.find_last_of( '[' );
// 	if ( pos != std::string::npos )
// 		info.offset = strtoll( symbol.c_str() + pos + 1, NULL, 0);
// 	else
// 	{
// 		info.offset = 0;
// 		return;
// 	}
// 	pos = symbol.find_last_of( '(' );
// 	if ( pos != std::string::npos )
// 		info.modulePath = symbol.substr( 0, pos );
// 	else
// 	{
// 		info.modulePath = "";
// 		return;
// 	}
// }

// #endif // defined(NODECPP_LINUX) && defined(NODECPP_CLANG)

#if defined(NODECPP_LINUX) || defined(NODECPP_ANDROID) || defined(NODECPP_MAC)

static bool addrToModuleAndOffset( void* fnAddr, nodecpp::stack_info_impl::ModuleAndOffset& mao ) {
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

#endif // defined(NODECPP_LINUX) || defined(NODECPP_ANDROID) || defined(NODECPP_MAC)

#if defined(NODECPP_LINUX)

static void parseAddr2LineOutput( const std::string& str, nodecpp::stack_info_impl::StackFrameInfo& info ) {
	if ( str.size() == 0 )
		return;
	size_t pos = 0;
	pos = str.find( '\n', pos );
	if ( pos != std::string::npos )
		info.functionName = str.substr( 0, pos );
	else
	{
		info.functionName = str;
		return;
	}
	
	if(info.functionName == "??")
		info.functionName.clear();

	size_t start = pos + 1;
	pos = str.find( ':', start );
	if ( pos != std::string::npos )
		info.srcPath = str.substr( start, pos - start );
	else // something went wrong
		return;

	if(info.srcPath == "??")
		info.srcPath.clear();

	info.line = atol( str.c_str() + pos + 1 );
}


static std::string callAddr2Line( nodecpp::stack_info_impl::stdstring modulePath, uintptr_t offset ) {
	char offsetstr[32];
	sprintf( offsetstr, "0x%zx", offset );
	std::string cmd = "addr2line -e ";
	cmd += modulePath;
	cmd += " -f -C ";
	cmd += offsetstr;
	auto r = sh(cmd);
	return r;
}

static void useAddr2Line( nodecpp::stack_info_impl::StackFrameInfo& info ) {
	// mb: clang on linux has special case
	auto r = callAddr2Line(info.modulePath, info.offset);
	if(r == "??\n??:0\n" && info.offset2 != 0)
	{
		r = callAddr2Line(info.modulePath, info.offset2);
		std::swap(info.offset, info.offset2);
	}

	parseAddr2LineOutput( r, info );
}

#endif // defined(NODECPP_LINUX)

#if defined(NODECPP_MAC)
static void parseAtosOutput( const std::string& str, nodecpp::stack_info_impl::StackFrameInfo& info ) {
	if ( str.size() == 0 )
		return;
	size_t pos = 0;
	size_t pos1 = str.find(' ');

	if ( pos1 != std::string::npos && pos2 != std::string::npos && pos1 < pos2)
		info.functionName = str.substr(0, pos1);
	else
	{
		info.functionName = str;
		return;
	}

	size_t pos3 = str.find_last_of('(');
	size_t pos4 = str.find_last_of(':');
	size_t pos5 = str.find_last_of(')');

	if ( pos3 != std::string::npos && pos4 != std::string::npos  && pos5 != std::string::npos && pos3 < pos4 && pos4 < pos5)
	{
		info.srcPath = str.substr( pos3 + 1, pos4 - (pos3 + 1) );
		std::string lineStr = str.substr( pos4 + 1, pos5 - (pos4 + 1));
		info.line = atol( lineStr.c_str() );
	}
}

void useAtos( nodecpp::stack_info_impl::StackFrameInfo& info ) {
	char offsetstr[32];
	sprintf( offsetstr, "0x%zx", info.offset );
	std::string cmd = "atos -o ";
	cmd += info.modulePath;
	cmd += " --offset ";
	cmd += offsetstr;
	auto r = sh(cmd);
	parseAtosOutput( r, info );
}

#endif // defined(NODECPP_MAC)

#if defined(NODECPP_ANDROID)
struct BacktraceState
{
	void** current;
	void** end;
};

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg)
{
    BacktraceState* state = static_cast<BacktraceState*>(arg);
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = reinterpret_cast<void*>(pc);
        }
    }
    return _URC_NO_REASON;
}

#endif // defined(NODECPP_ANDROID)

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nodecpp::stack_info_impl {
	bool stackPointerToInfo( void* ptr, nodecpp::stack_info_impl::StackFrameInfo& info )
	{
	#if defined(NODECPP_WINDOWS)

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

	#elif defined(NODECPP_LINUX) && defined(NODECPP_GCC)

		nodecpp::stack_info_impl::ModuleAndOffset mao;
		if ( addrToModuleAndOffset( ptr, mao ) )
		{
			info.offset = mao.offsetInModule;
			info.modulePath = mao.modulePath;
			useAddr2Line( info );
			return true;
		}
		else
			return false;

	#elif defined(NODECPP_LINUX) && defined(NODECPP_CLANG)

		nodecpp::stack_info_impl::ModuleAndOffset mao;
		if ( addrToModuleAndOffset( ptr, mao ) )
		{
			// mb: I see strange behaviour on clang on linux.
			// when we are the main executable, ptr returned by
			// backtrace is already an offset. But is an absolute
			// address when we are a dynamic load library
			info.offset = reinterpret_cast<uintptr_t>(ptr);
			info.offset2 = mao.offsetInModule;
			info.modulePath = mao.modulePath;
			useAddr2Line( info );
			return true;
		}
		else
			return false;

	#elif defined(NODECPP_MAC)

		nodecpp::stack_info_impl::ModuleAndOffset mao;
		if ( addrToModuleAndOffset( ptr, mao ) )
		{
			info.offset = mao.offsetInModule;
			info.modulePath = mao.modulePath;
			// mb: addr2line is not found on mac, use atos
			useAtos( info );
			return true;
		}
		else
			return false;

	#elif defined(NODECPP_ANDROID)

		nodecpp::stack_info_impl::ModuleAndOffset mao;
		if ( addrToModuleAndOffset( ptr, mao ) )
		{
			// mb: clang on android behaves as gcc on linux,
			// and not as clang on linux
			info.offset = mao.offsetInModule;
			info.modulePath = mao.modulePath;

			// mb: addr2line is not present on the device, is on the host where cross compile took place, i.e.
			// %ANDROID_HOME%\ndk\24.0.8215888\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-addr2line.exe
			// useAddr2Line( info );
			return true;
		}
		else
			return false;

	#else
	#error platform not (yet) supported, use NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
	#endif // platform/compiler
	}
} // nodecpp::stack_info_impl

void stackPointerToInfoToString( const nodecpp::stack_info_impl::StackFrameInfo& info, std::string& out )
{
	if ( info.modulePath.size() )
		out += fmt::format( "\tat {} [+0x{:x}]", info.modulePath, info.offset );
	else
		out += "\tat";
	if ( info.functionName.size() && info.srcPath.size() )
		out += fmt::format( ", {} in {}, line {}\n", info.functionName, info.srcPath, info.line );
	else if ( info.srcPath.size() )
		out += fmt::format( ",  {}, line {}\n", info.srcPath, info.line );
	else if ( info.functionName.size() )
		out += fmt::format( ", {}\n", info.functionName );
	else
		out += fmt::format( " <...>\n" );
}


///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nodecpp {

	void StackInfo::preinit()
	{
		static constexpr size_t TRACE_MAX_STACK_FRAMES = 1024;
#if defined(NODECPP_WINDOWS)

		void *stack[TRACE_MAX_STACK_FRAMES];
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		WORD numberOfFrames = CaptureStackBackTrace(1, TRACE_MAX_STACK_FRAMES, stack, NULL); // excluding current call itself
		stackPointers.init( stack, numberOfFrames );
		
#elif defined(NODECPP_LINUX) || defined(NODECPP_MAC)

		void *stack[TRACE_MAX_STACK_FRAMES];
		int numberOfFrames = backtrace( stack, TRACE_MAX_STACK_FRAMES );
		stackPointers.init( stack, numberOfFrames );

#elif defined(NODECPP_ANDROID)

		void *stack[TRACE_MAX_STACK_FRAMES];
		BacktraceState state = {stack, stack + TRACE_MAX_STACK_FRAMES};
		_Unwind_Backtrace(unwindCallback, &state);
		int numberOfFrames = state.current - stack;
		stackPointers.init( stack, numberOfFrames );

#else
#error not (yet) supported
#endif // platform/compiler
		timeStamp = ::nodecpp::logging_impl::getCurrentTimeStamp();
	}

	void StackInfo::postinit() const
	{
		void** stack = stackPointers.get();
		size_t numberOfFrames = stackPointers.size();
		std::string out;
		for (size_t i = 0; i < numberOfFrames; i++)
		{
			nodecpp::stack_info_impl::StackFrameInfo info;
			nodecpp::stack_info_impl::StackPointerInfoCache::getRegister().resolveData( stack[i], info );
			stackPointerToInfoToString( info, out );
		}
		if ( !stripPoint.empty() )
			strip( out, stripPoint.c_str() );
		*const_cast<error::string_ref*>(&whereTaken) = out.c_str();
	}


	namespace impl
	{
		extern const error::string_ref& whereTakenStackInfo( const StackInfo& info ) { 
			if ( info.whereTaken.empty() )
				info.postinit();
			return info.whereTaken;
		}
		extern ::nodecpp::logging_impl::LoggingTimeStamp whenTakenStackInfo( const StackInfo& info ) { return info.timeStamp; }
		extern bool isDataStackInfo( const StackInfo& info ) {
			return !(info.whereTaken.empty() && info.timeStamp.t == 0 && info.stackPointers.size() == 0);
		}
	} // namespace impl

} //namespace nodecpp

#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
