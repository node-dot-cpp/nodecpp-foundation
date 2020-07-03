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

#ifndef STACK_INFO_IMPL_H
#define STACK_INFO_IMPL_H

#ifndef NODECPP_NO_STACK_INFO_IN_EXCEPTIONS

#include <map>
#include <set>

struct StackFrameInfo
{
	std::string modulePath;
	std::string functionName;
	std::string srcPath;
	int line = 0;
	int column = 0;
	uintptr_t offset = 0;
};

struct ModuleAndOffset
{
	const char* modulePath;
	uintptr_t offsetInModule;
};

bool stackPointerToInfo( void* ptr, StackFrameInfo& info );

class StackPointerInfoCache
{
public:
	using StackStringCacheT = std::set<std::string>;
	StackStringCacheT modules;
	StackStringCacheT srcFiles;
	StackStringCacheT functionNames;
	struct StackFrameInfoInternal
	{
		const char* modulePath = nullptr;
		const char* functionName = nullptr;
		const char* srcPath = nullptr;
		int line = 0;
		int column = 0;
	};
	using BaseMapT = std::map<uintptr_t, StackFrameInfoInternal>;
	BaseMapT resolvedData;

private:
	std::mutex mxSearcher;

private:
	StackPointerInfoCache() {}

	bool getResolvedStackPtrData_( void* stackPtr, StackFrameInfo& info )
	{
		auto ret = resolvedData.find( (uintptr_t)(stackPtr) );
		if ( ret != resolvedData.end() )
		{
			info.functionName = ret->second.functionName;
			info.modulePath = ret->second.modulePath;
			info.srcPath = ret->second.srcPath;
			info.line = ret->second.line;
			info.column = ret->second.column;
			return true;
		}
		return false;
	}

	const char* add2stringCache( std::string str, StackStringCacheT& coll )
	{
		auto findRes = coll.find( str );
		if ( findRes != coll.end() )
			return findRes->c_str();
		else
		{
			auto r = coll.insert( str );
			if ( r.second )
				return r.first->c_str();
			else
				return "***"; // note: throwing here is not a good idea as with a high probability we're inside exception handler now. Let's just have this info lost
		}
	}

	void addResolvedStackPtrData_( void* stackPtr, const StackFrameInfo& info )
	{
		StackFrameInfoInternal sfi;
		sfi.modulePath = add2stringCache( info.modulePath, modules );
		sfi.srcPath = add2stringCache( info.srcPath, srcFiles );
		sfi.functionName = add2stringCache( info.functionName, functionNames );
		sfi.line = info.line;
		sfi.column = info.column;

		auto ret = resolvedData.insert( std::make_pair( (uintptr_t)(stackPtr), sfi ) );
		if ( ret.second )
			return;
		// note: attempt to assert here results in throwing an exception, which itself may envoce this potentially-broken machinery; in any case it is only a supplementary tool
		nodecpp::log::default_log::log( nodecpp::log::ModuleID(nodecpp::foundation_module_id), nodecpp::log::LogLevel::fatal, "!!! Assumptions at {}, line {} failed...", __FILE__, __LINE__ );
	}

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

	bool resolveData( void* stackPtr, StackFrameInfo& info )
	{
		std::unique_lock<std::mutex> lock(mxSearcher);
		if ( getResolvedStackPtrData_( stackPtr, info ) )
			return true;
		if ( getResolvedStackPtrData_( stackPtr, info ) )
			return true;
		bool ok = stackPointerToInfo( stackPtr, info );
		addResolvedStackPtrData_( stackPtr, info );
		return true;
	}
};

#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
#endif // STACK_INFO_IMPL_H
