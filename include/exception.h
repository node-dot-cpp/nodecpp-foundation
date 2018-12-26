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

#ifndef NODECPP_EXCEPTION_H
#define NODECPP_EXCEPTION_H 

#include "platform_base.h"
#include "../3rdparty/fmt/include/fmt/format.h"
#include "log.h"

namespace nodecpp::exception {

	class string_ref
	{
		bool fromLiteral;
		const char* str;
		class literal_tag_t {};
		char* duplicate_str( const char* str_ ) { return _strdup( str_ ); }

	public:
		string_ref( const char* str_ ) : fromLiteral( false ) {
			str = duplicate_str( str_ );
		}
		string_ref( literal_tag_t, const char* str_ ) : fromLiteral( true ), str( str_ ) {}
		string_ref( const string_ref& other ) {
			if ( fromLiteral )
				str = other.str;
			else
				str = duplicate_str(other.str);
			fromLiteral = other.fromLiteral;
		}
		string_ref operator = ( const string_ref& other ) {
			if ( fromLiteral )
				str = other.str;
			else
				str = duplicate_str(other.str);
			fromLiteral = other.fromLiteral;
			return *this;
		}
		string_ref( string_ref&& other ) {
			other.str = nullptr;
			str = other.str;
			fromLiteral = other.fromLiteral;
			other.fromLiteral = false;
		}
		string_ref operator = ( string_ref&& other ) {
			other.str = nullptr;
			str = other.str;
			fromLiteral = other.fromLiteral;
			other.fromLiteral = false;
			return *this;
		}
		~string_ref() {
			if ( !fromLiteral && str )
				free( const_cast<char*>(str) );
		}
		const char* c_str() {
			return str ? str : "";
		}
	};

	enum class FILE_EXCEPTION { dne, access_denied };
	enum class MEMORY_EXCEPTION { memory_access_violation, null_pointer_access, out_of_bound };

	class error_value
	{
	public:
		error_value() {}
		virtual ~error_value() {}
	};

	class error_domain
	{
	public:
		enum Type { unknown_error, file_error, memory_error };
		constexpr error_domain() {}
		virtual Type type() const { return unknown_error; }
		virtual const char* name() const { return "unknown error"; }
		virtual std::string value_to_meaasage(error_value* value) const { return std::string(""); }
		//virtual ~error_domain() {}
		virtual error_value* clone_value(error_value* value) const {}
		virtual void destroy_value(error_value* value) const {}
	};

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
	public:
		constexpr file_error_domain() {}
		using Valuetype = file_error_value;
		virtual error_domain::Type type() const { return error_domain::file_error; }
		virtual const char* name() const { return "file error"; }
		virtual std::string value_to_meaasage(error_value* value) const { 
			file_error_value* myData = reinterpret_cast<file_error_value*>(value);
			switch ( myData->errorCode )
			{
				case FILE_EXCEPTION::dne: { std::string s = fmt::format("\'{}\': file does not exist", myData->fileName.c_str()); return s.c_str(); break; }
				case FILE_EXCEPTION::access_denied: { std::string s = fmt::format("\'{}\': access denied", myData->fileName.c_str()); return s.c_str(); break; }
				default: return "unknown file error"; break;
			}
		}
		error_value* create_value( FILE_EXCEPTION code, const char* fileName ) const {
			return new file_error_value(code, fileName);
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
		//virtual ~file_error_domain() {}
	};
	static constexpr file_error_domain file_error_domain_obj;

	class error
	{
		const error_domain* domain = nullptr;
		error_value* value = nullptr;

	public:
//		bool operator == ( FILE_EXCEPTION fe ) { return t == file_exception && e.fe == fe; }
//		bool operator == ( MEMORY_EXCEPTION me ) { return t == memory_exception && e.me == me; }
		error(const error_domain* domain_,	error_value* value_) : domain( domain_ ), value( value_ ) {}
		error( const error& other ) {
			domain = other.domain;
			if ( domain )
				value = domain->clone_value( other.value );
			else
				value = nullptr;
		}
		error operator = ( const error& other ) {
			domain = other.domain;
			if ( domain )
				value = domain->clone_value( other.value );
			else
				value = nullptr;
			return *this;
		}
		error( error&& other ) {
			domain = other.domain;
			other.domain = nullptr;
			value = other.value;
			other.value = nullptr;
		}
		error operator = ( error&& other ) {
			domain = other.domain;
			other.domain = nullptr;
			value = other.value;
			other.value = nullptr;
			return *this;
		}
		const char* name() { return domain->name(); }
		std::string description() { return domain->value_to_meaasage(value); }
		~error() {
			if ( domain )
				domain->destroy_value( value );
		}
	};

	class file_error : public error
	{
	public:
		file_error(FILE_EXCEPTION code, const char* fileName) : error( &file_error_domain_obj, file_error_domain_obj.create_value(code, fileName) ) {}
	};

} // namespace nodecpp::exception

#endif // NODECPP_EXCEPTION_H
