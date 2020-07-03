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

#elif defined NODECPP_CLANG || defined NODECPP_GCC

#include <execinfo.h>
#include <unistd.h>
#include <dlfcn.h>

#else
#error not (yet) supported
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////



#if defined NODECPP_LINUX && ( defined NODECPP_CLANG || defined NODECPP_GCC )

static void parseBtSymbol( std::string symbol, StackFrameInfo& info ) {
	if ( symbol.size() == 0 )
		return;
	size_t pos = symbol.find_last_of( '[' );
	if ( pos != std::string::npos )
		info.offset = strtoll( symbol.c_str() + pos + 1, NULL, 0);
	else
	{
		info.offset = 0;
		return;
	}
	pos = symbol.find_last_of( '(' );
	if ( pos != std::string::npos )
		info.modulePath = symbol.substr( 0, pos );
	else
	{
		info.modulePath = "";
		return;
	}
}

static void parseAddr2LineOutput( const std::string& str, StackFrameInfo& info ) {
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
	size_t start = pos + 1;
	pos = str.find( ':', start );
	if ( pos != std::string::npos )
		info.srcPath = str.substr( start, pos - start );
	else // something went wrong
		return;
	info.line = atol( str.c_str() + pos + 1 );
}

std::string sh(std::string cmd) {
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
printf( "sh(%s) = \"%s\"\n", cmd.c_str(), result.c_str() );
    return result;
}

static void useAddr2Line( const char* moduleName, uintptr_t offsetInModule, StackFrameInfo& info ) {
#ifdef NODECPP_CLANG
	if ( info.offset != 0 )
		offsetInModule = info.offset;
#endif // NODECPP_CLANG
	char offsetstr[32];
	sprintf( offsetstr, "0x%zx", offsetInModule );
	std::string cmd = "addr2line -e ";
	cmd += moduleName;
	cmd += " -f -C ";
	cmd += offsetstr;
	auto r = sh(cmd);
	parseAddr2LineOutput( r, info );
}

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

static void addFileLineInfo( const char* ModuleName, uintptr_t Offset, StackFrameInfo& sfi ) {
	useAddr2Line( ModuleName, Offset, sfi );
//	useLlvmSymbolizer( ModuleName, Offset, sfi );
}

#endif // defined NODECPP_LINUX && ( defined NODECPP_CLANG || defined NODECPP_GCC )

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

	ModuleAndOffset mao;
	if ( addrToModuleAndOffset( ptr, mao ) )
	{
		addFileLineInfo( mao.modulePath, mao.offsetInModule, info );
		return true;
	}
	else
		return false;

#else
#error not (yet) supported
#endif // platform/compiler
}

void stackPointerToInfoToString( const StackFrameInfo& info, std::string& out )
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
#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )

		void *stack[TRACE_MAX_STACK_FRAMES];
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		WORD numberOfFrames = CaptureStackBackTrace(1, TRACE_MAX_STACK_FRAMES, stack, NULL); // excluding current call itself
		stackPointers.init( stack, numberOfFrames );
		
#elif defined NODECPP_CLANG || defined NODECPP_GCC

		void *stack[TRACE_MAX_STACK_FRAMES];
		int numberOfFrames = backtrace( stack, TRACE_MAX_STACK_FRAMES );
		stackPointers.init( stack, numberOfFrames );

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
			stackPointerToInfoToString( info, out );
		}
		if ( !stripPoint.empty() )
			strip( out, stripPoint.c_str() );
		*const_cast<error::string_ref*>(&whereTaken) = out.c_str();
		
#elif defined NODECPP_CLANG || defined NODECPP_GCC

		void** stack = stackPointers.get();
		size_t numberOfFrames = stackPointers.size();
		char ** btsymbols = backtrace_symbols( stack, numberOfFrames );
		if ( btsymbols != nullptr )
		{
			std::string out;
			for (size_t i = 0; i < numberOfFrames; i++)
			{
printf( "btsymbols[%zd] = \"%s\"\n", i, btsymbols[i] );
				StackFrameInfo info;
#ifdef NODECPP_CLANG
				parseBtSymbol( btsymbols[i], info );
#endif // NODECPP_CLANG
				StackPointerInfoCache::getRegister().resolveData( stack[i], info );
				stackPointerToInfoToString( info, out );
			}
			free( btsymbols );
			if ( !stripPoint.empty() )
				strip( out, stripPoint.c_str() );
			*const_cast<error::string_ref*>(&whereTaken) = out.c_str();
		}
		else
			*const_cast<error::string_ref*>(&whereTaken) = error::string_ref	( error::string_ref::literal_tag_t(), "" );

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
