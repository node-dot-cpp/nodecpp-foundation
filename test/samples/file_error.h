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

#ifndef NODECPP_FILE_ERROR_H
#define NODECPP_FILE_ERROR_H 

#include <platform_base.h>
#include <error.h>
#include <fmt/format.h>

#include <cerrno>  // for error constants

using namespace nodecpp::error;

namespace nodecpp::error {

	enum class FILE_EXCEPTION { dne, access_denied };
	enum class MEMORY_EXCEPTION { memory_access_violation, null_pointer_access, out_of_bound };

	class file_error_value : public error_value
	{
		friend class file_error_domain;
		FILE_EXCEPTION errorCode;
		string_ref fileName;
	public:
		file_error_value( FILE_EXCEPTION code, const char* fileName_ ) : errorCode( code ), fileName( fileName_ ) {}
		file_error_value( const file_error_value& other ) = default;
		file_error_value operator = ( const file_error_value& other ) {
			errorCode = other.errorCode;
			fileName = other.fileName;
			return *this;
		}
		file_error_value operator = ( file_error_value&& other ) {
			errorCode = other.errorCode;
			fileName = std::move( other.fileName );
			return *this;
		}
		file_error_value( file_error_value&& other ) = default;
		virtual ~file_error_value() {}
	};

	class file_error_domain : public error_domain
	{
	protected:
		virtual uintptr_t _nodecpp_get_error_code(const error_value* value) const { return (int)(reinterpret_cast<const file_error_value*>(value)->errorCode); } // for inter-domain comparison purposes only

	public:
		constexpr file_error_domain() {}
		using Valuetype = file_error_value;
		virtual string_ref name() const { return string_ref( string_ref::literal_tag_t(), "file error" ); }
		virtual string_ref value_to_message(error_value* value) const { 
			file_error_value* myData = reinterpret_cast<file_error_value*>(value);
			switch ( myData->errorCode )
			{
				case FILE_EXCEPTION::dne: { std::string s = fmt::format("\'{}\': file does not exist", myData->fileName.c_str()); return string_ref( s.c_str() ); break; }
				case FILE_EXCEPTION::access_denied: { std::string s = fmt::format("\'{}\': access denied", myData->fileName.c_str()); return string_ref( s.c_str() ); break; }
				default: return "unknown file error"; break;
			}
		}
		virtual void log(error_value* value, log::LogLevel l ) const { log::default_log::log( l, "{}", value_to_message( value ).c_str() ); }
		virtual void log(error_value* value, log::Log& targetLog, log::LogLevel l ) const { targetLog.log( l, "{}", value_to_message( value ).c_str() ); }
		error_value* create_value( FILE_EXCEPTION code, const char* fileName ) const {
			return new file_error_value(code, fileName);
		}
		virtual bool is_same_error_code(const error_value* value1, const error_value* value2) const { 
			return reinterpret_cast<const file_error_value*>(value1)->errorCode == reinterpret_cast<const file_error_value*>(value1)->errorCode;
		}
		virtual error_value* clone_value(error_value* value) const {
			if ( value )
				return new file_error_value( *reinterpret_cast<file_error_value*>(value) );
			else
				return nullptr;
		}
		virtual void destroy_value(error_value* value) const {
			if ( value ) {
				file_error_value* myData = reinterpret_cast<file_error_value*>(value);
				delete myData;
			}
		}
		virtual bool is_equivalent( const error& src, const error_value* my_value ) const {
			if ( src.domain() == this )
				return is_same_error_code( my_value, src.value() );
			else
				return false;
		}
	};
	extern const file_error_domain file_error_domain_obj;

	class file_error : public error
	{
	public:
		using DomainTypeT = file_error_domain;
		file_error(FILE_EXCEPTION code, const char* fileName) : error( &file_error_domain_obj, file_error_domain_obj.create_value(code, fileName) ) {}
	};

} // namespace nodecpp::error


#endif // NODECPP_FILE_ERROR_H
