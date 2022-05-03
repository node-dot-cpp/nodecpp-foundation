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

#ifndef NODECPP_NO_STACK_INFO_IN_EXCEPTIONS

// NOTE: on Linux (both clang and gcc) the following is required: -rdynamic -ldl

#include "platform_base.h"
#include "foundation.h"
#include "string_ref.h"
#include "log.h"
#include <fmt/format.h>

#if ! ( ( ( defined NODECPP_LINUX || defined NODECPP_MAC ) && ( defined NODECPP_CLANG || defined NODECPP_GCC ) ) || defined NODECPP_WINDOWS )
#error unsupported
#endif

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

		class StackPointers {
			void** ptrs = nullptr;
			size_t cnt = 0;
		public:
			StackPointers() {};
			StackPointers( const StackPointers& other ) { init( other.ptrs, other.cnt ); }
			StackPointers& operator = ( const StackPointers& other ) { init( other.ptrs, other.cnt ); return *this; }
			StackPointers( StackPointers&& other ) noexcept { ptrs = other.ptrs; cnt = other.cnt; other.ptrs = nullptr; other.cnt = 0; }
			StackPointers& operator = ( StackPointers&& other ) noexcept { ptrs = other.ptrs; cnt = other.cnt; other.ptrs = nullptr; other.cnt = 0; return *this; }
			~StackPointers() { if (ptrs ) free( ptrs ); }
			void init( void** ptrs_, size_t cnt_ ) { 
				// note: due to the purpose of this class we cannot throw here; if something goes wrong, we just indicate no data
				if (ptrs ) 
					free( ptrs ); 
				ptrs = nullptr;
				cnt = cnt_; 
				if ( cnt )
				{
					ptrs = (void**)( malloc( sizeof(void*) * cnt ) ); 
					if ( ptrs != nullptr )
						memcpy( ptrs, ptrs_, sizeof(void*) * cnt );
					else
						cnt = 0;
				}
			}
			size_t size() const { return cnt; }
			void** get() const { return ptrs; }
		};
		StackPointers stackPointers;
		error::string_ref stripPoint;
		::nodecpp::logging_impl::LoggingTimeStamp timeStamp;
		error::string_ref whereTaken;

		void preinit();
		void postinit() const;
		void strip( std::string& s, const char* stripPoint ) const {
			size_t pos = s.find( stripPoint, 0 );
			if ( pos != std::string::npos )
			{
				pos = s.find( "\n", pos );
				if ( pos != std::string::npos )
					s = s.substr( pos + 1 );
			}
		}

	public:
		StackInfo() : stripPoint( error::string_ref::literal_tag_t(), "" ), whereTaken( error::string_ref::literal_tag_t(), "" ) {}
		StackInfo( bool doInit ) : stripPoint( error::string_ref::literal_tag_t(), "" ), whereTaken( error::string_ref::literal_tag_t(), "" ) { if ( doInit ) preinit(); }
		StackInfo( bool doInit, error::string_ref&& stripPoint_ ) : stripPoint( std::move( stripPoint_ ) ), whereTaken( error::string_ref::literal_tag_t(), "" ) { if ( doInit ) preinit(); }
		StackInfo( const StackInfo& other ) = default;
		StackInfo& operator = ( const StackInfo& other ) = default;
		StackInfo( StackInfo&& other ) = default;
		StackInfo& operator = ( StackInfo&& other ) = default;
		virtual ~StackInfo() {}
		void init() { 
			stripPoint = error::string_ref( error::string_ref::literal_tag_t(), "" ); 
			preinit();
		}
		void init( error::string_ref&& stripPoint_ ) { 
			stripPoint = std::move( stripPoint_ ); 
			preinit();
		}
		void clear() { stripPoint = error::string_ref( error::string_ref::literal_tag_t(), "" ); whereTaken = nullptr; }
		void log( log::LogLevel l ) { 
			if ( whereTaken.empty() )
				postinit();
			log::default_log::log( l, "time {}\n{}", timeStamp, whereTaken.c_str() );
		}
		void log( log::Log& targetLog, log::LogLevel l ) {
			if ( whereTaken.empty() )
				postinit();
			targetLog.log( l, "time {}\n{}", timeStamp, whereTaken.c_str() );
		}
	};

} //namespace nodecpp

#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS

#endif // NODECPP_STACK_INFO_H
