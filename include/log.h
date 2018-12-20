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

#ifndef NODECPP_LOG_H
#define NODECPP_LOG_H 

#include "platform_base.h"
#include "../3rdparty/fmt/include/fmt/format.h"


namespace nodecpp::log { 

	enum class LogLevel { error = 0, warn = 1, info = 2, verbose = 3, debug = 4, silly = 5 }; // https://blog.risingstack.com/node-js-logging-tutorial/
	constexpr const char* LogLevelNames[] = { "error", "warn", "info", "verbose", "debug", "silly", "" };
	using ModuleIDType = int; // at least, so far

#ifdef NODECPP_CUSTOM_LOG_PROCESSING
#include NODECPP_CUSTOM_LOG_PROCESSING
#else
#include "default_log.h"
#endif // NODECPP_CUSTOM_LOG_PROCESSING

	extern std::unique_ptr<DefaultSink> logObject;

	std::unique_ptr<DefaultSink> create_sink();

	template< ModuleIDType module, LogLevel level, typename... ARGS>
	void log( const char* formatStr, const ARGS& ... args ) {
		if constexpr ( ShouldLog<module, level>::value )
		{
			if ( logObject == nullptr )
				logObject = create_sink();
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
			logObject->log( level, logBuf );
		}
	}

} // namespace nodecpp::log

#endif // NODECPP_LOG_H
