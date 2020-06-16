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

#ifndef NODECPP_STRING_REF_H
#define NODECPP_STRING_REF_H 

#include "platform_base.h"

// quick workaround TODO: address properly ASAP!
#if defined NODECPP_WINDOWS
#include <string.h>
#define strdup _strdup 
#else
#include <cstdlib>
#include <string.h>
#endif

namespace nodecpp::error {

	class string_ref
	{
		bool fromLiteral;
		const char* str = nullptr;
		char* duplicate_str( const char* str_ ) { return strdup( str_ ); }
		void release_str() {
			if ( !fromLiteral && str )
			{
				free( const_cast<char*>(str) );
				str = nullptr;
			}
		}

	public:
		class literal_tag_t {};

	public:
		string_ref( const char* str_ ) : fromLiteral( false ) {
			str = duplicate_str( str_ );
		}
		string_ref& operator = ( const char* str_ )  {
			release_str();
			str = duplicate_str( str_ );
			fromLiteral = false;
			return *this;
		}
		string_ref( literal_tag_t, const char* str_ ) : fromLiteral( true ), str( str_ ) {}
		string_ref( const string_ref& other ) {
			if ( other.fromLiteral )
				str = other.str;
			else
				str = duplicate_str(other.str);
			fromLiteral = other.fromLiteral;
		}
		string_ref& operator = ( const string_ref& other ) {
			release_str();
			if ( other.fromLiteral )
				str = other.str;
			else
				str = duplicate_str(other.str);
			fromLiteral = other.fromLiteral;
			return *this;
		}
		string_ref( string_ref&& other ) {
			str = other.str;
			other.str = nullptr;
			fromLiteral = other.fromLiteral;
			other.fromLiteral = false;
		}
		string_ref& operator = ( string_ref&& other ) {
			release_str();
			str = other.str;
			other.str = nullptr;
			fromLiteral = other.fromLiteral;
			other.fromLiteral = false;
			return *this;
		}
		~string_ref() {
			release_str();
		}
		const char* c_str() const {
			return str ? str : "";
		}
		bool empty() const { return str == nullptr || str[0] == 0; }
	};



} // namespace nodecpp::error


#endif // NODECPP_STRING_REF_H
