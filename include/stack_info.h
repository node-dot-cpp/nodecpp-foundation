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

#ifndef NODECPP_STACK_INFO_H
#define NODECPP_STACK_INFO_H

#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO

#include "platform_base.h"
#include "foundation.h"
#include "string_ref.h"
#include "log.h"
#include <fmt/format.h>

namespace nodecpp {

	class StackInfo;

	namespace impl
	{
		extern const error::string_ref& whereTakenStackInfo( const StackInfo& info );
		extern ::nodecpp::logging_impl::LoggingTimeStamp whenTakenStackInfo( const StackInfo& info );
		extern bool isDataStackInfo( const StackInfo& info );
	} // namespace impl

	class StackInfo
	{
		friend const error::string_ref& impl::whereTakenStackInfo( const StackInfo& info );
		friend ::nodecpp::logging_impl::LoggingTimeStamp impl::whenTakenStackInfo( const StackInfo& info );
		friend bool impl::isDataStackInfo( const StackInfo& info );

		::nodecpp::logging_impl::LoggingTimeStamp timeStamp;
		error::string_ref whereTaken;
		void init_( const char* stripPoint = nullptr );
		void strip( std::string& s, const char* stripPoint ){
			size_t pos = s.find( stripPoint, 0 );
			if ( pos != std::string::npos )
			{
				pos = s.find( "\n", pos );
				if ( pos != std::string::npos )
					s = s.substr( pos + 1 );
			}
		}

	public:
		StackInfo() : whereTaken( error::string_ref::literal_tag_t(), "" ) {}
		StackInfo( bool doInit ) : whereTaken( error::string_ref::literal_tag_t(), "" ) { if ( doInit ) init_(); }
		StackInfo( bool doInit, const char* stripPoint ) : whereTaken( error::string_ref::literal_tag_t(), "" ) { if ( doInit ) init_( stripPoint ); }
		StackInfo( const StackInfo& other ) = default;
		StackInfo& operator = ( const StackInfo& other ) = default;
		StackInfo( StackInfo&& other ) = default;
		StackInfo& operator = ( StackInfo&& other ) = default;
		virtual ~StackInfo() {}
		void init( const char* stripPoint = nullptr ) { init_( stripPoint ); }
		void clear() { whereTaken = nullptr; }
		void log( log::LogLevel l ) { log::default_log::log( l, "time {}\n{}", timeStamp, whereTaken.c_str() ); }
		void log( log::Log& targetLog, log::LogLevel l ) { targetLog.log( l, "time {}\n{}", timeStamp, whereTaken.c_str() ); }
	};

} //namespace nodecpp

#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO

#endif // NODECPP_STACK_INFO_H
