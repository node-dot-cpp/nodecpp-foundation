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

#ifndef NODECPP_ASSERT_H
#define NODECPP_ASSERT_H 

#include "platform_base.h"
#include <fmt/format.h>
#include "log.h"
#include "error.h"


namespace nodecpp::assert { // pedantic regular critical

	enum class AssertLevel { critical = 0, regular = 1, pedantic = 2 }; // just a sketch; TODO: revise
	using ModuleIDType = uint64_t; // at least, so far

	enum class ActionType { ignoring, throwing, terminating };

#ifdef NODECPP_CUSTOM_ASSERT_PROCESSING
#include NODECPP_CUSTOM_ASSERT_PROCESSING
#else
#include "default_assert.h"
#endif // NODECPP_CUSTOM_LOG_PROCESSING

	template< ModuleIDType module, AssertLevel level, class Expr, typename... ARGS>
	void nodecpp_assert( const char* file, int line, const Expr& expr, const char* condString, const char* formatStr, const ARGS& ... args ) {
		if constexpr ( ShouldAssert<module, level>::value != ActionType::ignoring )
		{
			if ( expr() )
				return;
			constexpr size_t logBufSz = 1024;
			char logBuf[logBufSz];
			auto res = fmt::format_to_n( logBuf, logBufSz-1, formatStr, args... );
			if ( res.size >= logBufSz )
			{
				logBuf[logBufSz-4] = '.';
				logBuf[logBufSz-3] = '.';
				logBuf[logBufSz-2] = '.';
				logBuf[logBufSz-1] = 0;
			}
			else
				logBuf[res.size] = 0;
			nodecpp::log::log<module, nodecpp::log::LogLevel::error>("File \"{}\", line {}: assertion \'{}\' failed, message: \"{}\"", file, line, condString, logBuf );
			if constexpr ( ShouldAssert<module, level>::value == ActionType::throwing )
				throw std::exception();
			else
				std::abort();
		}
	}

	template< ModuleIDType module, AssertLevel level, class Expr>
	void nodecpp_assert( const char* file, int line, const Expr& expr, const char* condString ) {
		if constexpr ( ShouldAssert<module, level>::value != ActionType::ignoring )
		{
			if ( expr() )
				return;
			nodecpp::log::log<module, nodecpp::log::LogLevel::error>("File \"{}\", line {}: assertion \'{}\' failed", file, line, condString);
			if constexpr ( ShouldAssert<module, level>::value == ActionType::throwing )
				throw std::exception();
			else
				std::abort();
		}
	}

#define NODECPP_ASSERT( module, level, condition, ... ) \
	::nodecpp::assert::nodecpp_assert<module, level>( __FILE__, __LINE__, [&] { return !!(condition); }, #condition, ## __VA_ARGS__ )

} // namespace nodecpp::assert

#endif // NODECPP_ASSERT_H
