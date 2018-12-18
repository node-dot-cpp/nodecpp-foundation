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
#include <string>
#include "../3rdparty/fmt/include/fmt/format.h"


namespace nodecpp::log { 

	enum class LogLevel { Emergency = 0, Alert = 1, Critical = 2, Error = 3, Warning = 4, Notice = 5, Informational = 6, Debug = 7 }; // https://en.wikipedia.org/wiki/Syslog#Severity_level
	using ModuleIDType = int; // at least, so far

	template< ModuleIDType module, LogLevel level>
	struct ShouldLog
	{
		static constexpr bool value = true;
	};

#ifdef NODECPP_CUSTOM_LOG_PROCESSING
#include NODECPP_CUSTOM_LOG_PROCESSING
#endif // NODECPP_CUSTOM_LOG_PROCESSING

	class Log
	{
	private:
		FILE* LogFile = NULL;
		FILE* LogConsole = stdout;
	public:
		template< ModuleIDType module, LogLevel level, typename... ARGS>
		void log( const char* formatStr, const ARGS& ... args ) {
			if constexpr ( ShouldLog<module, level>::value )
			{
				std::string s = fmt::format( formatStr, args... );
				if ( LogFile ) fprintf( LogFile, "%s\n", s.c_str() );
				if ( LogConsole ) fprintf( LogConsole, "%s\n", s.c_str() );
			}
		}
	};


	extern std::unique_ptr<Log> logObject;

//	inline
	std::unique_ptr<Log> create_log_object();
/*	{
		return std::make_unique<Log>();
	}*/

	template< ModuleIDType module, LogLevel level, typename... ARGS>
	void log( const char* formatStr, const ARGS& ... args ) {
		if constexpr ( ShouldLog<module, level>::value )
		{
			if ( logObject == nullptr )
				logObject = create_log_object();
			logObject->log<module, level>(formatStr, args... );
		}
	}

} // namespace nodecpp::log

#endif // NODECPP_LOG_H
