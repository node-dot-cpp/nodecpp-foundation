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

#ifdef NODECPP_TWO_PHASE_STACK_DATA_RESOLVING
#include <map>
class StackPointerInfoCache
{
public:
	struct StackFrameInfo
	{
		std::string modulePath;
		std::string functionName;
		std::string srcPath;
		int line = 0;
		int column = 0;
	};
	using StackStringCacheT = std::map<std::string, int>;
	struct ModuleAndOffset
	{
		const char* modulePath;
		uintptr_t offsetInModule;
	};
	struct StackFrameInfoInternal
	{
		std::string modulePath;
		std::string functionName;
		std::string srcPath;
		int line = 0;
		int column = 0;
	};
	using BaseMapT = std::map<uintptr_t, StackFrameInfo>;
	BaseMapT resolvedData;

private:
		std::condition_variable waitRequester;
		std::mutex mx;

private:
    StackPointerInfoCache() {}

public:
    StackPointerInfoCache( const StackPointerInfoCache& ) = delete;
    StackPointerInfoCache( StackPointerInfoCache&& ) = delete;
    StackPointerInfoCache& operator = ( const StackPointerInfoCache& ) = delete;
    StackPointerInfoCache& operator = ( StackPointerInfoCache&& ) = delete;
    static StackPointerInfoCache& getRegister()
    {
        static StackPointerInfoCache r;
        return r;
    }

	bool getResolvedStackPtrData( void* stackPtr, StackFrameInfo& info )
	{
		std::unique_lock<std::mutex> lock(mx);
		BaseMapT resolvedData_;
		auto ret_ = resolvedData_.find( (uintptr_t)(stackPtr) );
		auto ret = resolvedData.find( (uintptr_t)(stackPtr) );
		if ( ret != resolvedData.end() )
		{
			info = ret->second;
			lock.unlock();
			waitRequester.notify_one();
			return true;
		}
		lock.unlock();
		waitRequester.notify_one();
		return false;
	}

	void addResolvedStackPtrData( void* stackPtr, const StackFrameInfo& info )
	{
		std::unique_lock<std::mutex> lock(mx);
		auto ret = resolvedData.insert( std::make_pair( (uintptr_t)(stackPtr), info ) );
		if ( ret.second )
		{
			lock.unlock();
			waitRequester.notify_one();
			return;
		}
		// note: attempt to assert here results in throwing an exception, which itself may envoce this potentially-broken machinery; in any case it is only a supplementary tool
		lock.unlock();
		waitRequester.notify_one();
		nodecpp::log::default_log::log( nodecpp::log::ModuleID(nodecpp::foundation_module_id), nodecpp::log::LogLevel::fatal, "!!! Assumptions at {}, line {} failed...", __FILE__, __LINE__ );
	}

};
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

#include "llvm/DebugInfo/Symbolize/Symbolize.h"
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string>

#ifndef _GNU_SOURCE
#error _GNU_SOURCE must be defined to proceed
#endif

#include <dlfcn.h>

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

static void addFileLineInfo( const char* ModuleName, uintptr_t Offset, std::string& out, bool addFnName, StackFrameInfo& sfi ) {
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

		if ( addFnName )
			out += fmt::format( "at {} in {}, line {}:{}\n", li.FunctionName.c_str(), li.FileName.c_str(), li.Line, li.Column );
		else
			out += fmt::format( " in {}, line {}:{}\n", li.FileName.c_str(), li.Line, li.Column );
	}
}

#endif // NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE

#else
#error not (yet) supported
#endif

#define TRACE_MAX_STACK_FRAMES 1024
#define TRACE_MAX_FUNCTION_NAME_LENGTH 1024

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
		HANDLE process = GetCurrentProcess();
		SYMBOL_INFO *symbol = (SYMBOL_INFO *)malloc(sizeof(SYMBOL_INFO) + (TRACE_MAX_FUNCTION_NAME_LENGTH - 1) * sizeof(TCHAR));
		symbol->MaxNameLen = TRACE_MAX_FUNCTION_NAME_LENGTH;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		DWORD displacement = 0;
//		IMAGEHLP_LINE64 *line = (IMAGEHLP_LINE64 *)malloc(sizeof(IMAGEHLP_LINE64));
		IMAGEHLP_LINE64 line;
		memset(&line, 0, sizeof(IMAGEHLP_LINE64));
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		std::string out;
		for (int i = 0; i < numberOfFrames; i++)
		{
			StackPointerInfoCache::StackFrameInfo info;
			if ( StackPointerInfoCache::getRegister().getResolvedStackPtrData( stack[i], info ) )
			{
				if ( info.functionName.size() && info.srcPath.size() )
					out += fmt::format( "\tat {} in {}, line {}\n", info.functionName, info.srcPath, info.line );
				else if ( info.srcPath.size() )
					out += fmt::format( "\tat {}, line {}\n", info.srcPath, info.line );
				else if ( info.functionName.size() )
					out += fmt::format( "\tat {}\n", info.functionName );
				else
					out += fmt::format( "\tat <...>\n" );
			}
			else
			{
				DWORD64 address = (DWORD64)(stack[i]);
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
				StackPointerInfoCache::getRegister().addResolvedStackPtrData( stack[i], info );

				if ( addrOK && fileLineOK )
					out += fmt::format( "\tat {} in {}, line {}\n", symbol->Name, line.FileName, line.LineNumber );
				else if ( fileLineOK )
					out += fmt::format( "\tat {}, line {}\n", line.FileName, line.LineNumber );
				else if ( addrOK )
					out += fmt::format( "\tat {}\n", symbol->Name );
				else
					out += fmt::format( "\tat <...>\n" );
			}
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
				StackPointerInfoCache::StackFrameInfo info;
				if ( StackPointerInfoCache::getRegister().getResolvedStackPtrData( stack[i], info ) )
				{
					if ( info.functionName.size() && info.srcPath.size() )
						out += fmt::format( "\tat {} in {}, line {}\n", info.functionName, info.srcPath, info.line );
					else if ( info.srcPath.size() )
						out += fmt::format( "\tat {}, line {}\n", info.srcPath, info.line );
					else if ( info.functionName.size() )
						out += fmt::format( "\tat {}\n", info.functionName );
					else
						out += fmt::format( "\tat <...>\n" );
				}
				else
				{
#ifdef NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE
					info.functionName = btsymbols[i];

					ModuleAndOffset mao;
					if ( StackPointerInfoCache::getRegister().addrToModuleAndOffset( stack[i], mao ) )
					{
						out += fmt::format( "\tat {}", btsymbols[i] );
						addFileLineInfo( mao.modulePath, mao.offsetInModule, out, true, info );
						addResolvedStackPtrData( stack[i], info );
					}
					else
						out += fmt::format( "\tat {}\n", btsymbols[i] );
#else
					out += fmt::format( "\tat {}\n", btsymbols[i] );
#endif // NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE
				}
			}
			free( btsymbols );
			if ( !stripPoint.empty() )
				strip( out, stripPoint.c_str() );
			*const_cast<error::string_ref*>(&whereTaken) = out.c_str();
		}
		else
			*const_cast<error::string_ref*>(&whereTaken) = error::string_ref	( error::string_ref::literal_tag_t(), "" );
#endif // NODECPP_LINUX_NO_LIBUNWIND

#else
#error not (yet) supported
#endif // platform/compiler
	}

	void StackInfo::init_()
	{
#if (defined NODECPP_MSVC) || (defined NODECPP_WINDOWS && defined NODECPP_CLANG )

		/*void *stack[TRACE_MAX_STACK_FRAMES];
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		WORD numberOfFrames = CaptureStackBackTrace(1, TRACE_MAX_STACK_FRAMES, stack, NULL); // excluding current call itself
		stackPointers.init( stack, numberOfFrames );
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
		if ( !stripPoint.empty() )
			strip( out, stripPoint.c_str() );
		whereTaken = out.c_str();*/
		preinit();
		
#elif defined NODECPP_CLANG || defined NODECPP_GCC

#ifdef NODECPP_LINUX_NO_LIBUNWIND
		/*void *stack[TRACE_MAX_STACK_FRAMES];
		int numberOfFrames = backtrace( stack, TRACE_MAX_STACK_FRAMES );
		stackPointers.init( stack, numberOfFrames );
		char ** btsymbols = backtrace_symbols( stack, numberOfFrames );
		if ( btsymbols != nullptr )
		{
			std::string out;
			for (int i = 0; i < numberOfFrames; i++)
			{
#ifdef NODECPP_STACKINFO_USE_LLVM_SYMBOLIZE
				ModuleAndOffset mao;
				if ( addrToModuleAndOffset( stack[i], mao ) )
				{
					out += fmt::format( "\tat {}", btsymbols[i] );
					addFileLineInfo( mao.modulePath, mao.offsetInModule, out, true );
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
			whereTaken = error::string_ref	( error::string_ref::literal_tag_t(), "" );*/
		preinit();
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
				ModuleAndOffset mao;
				if ( addrToModuleAndOffset( pc + offset, mao ) )
				{
					out += fmt::format( " ({}+0x{:x})", nameptr, offset );
					addFileLineInfo( mao.modulePath, mao.offsetInModule, out, true );
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
