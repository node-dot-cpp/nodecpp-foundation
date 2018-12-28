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

#include <cerrno>  // for error constants

namespace nodecpp::exception {

	enum class errc : int
	{
		success = 0,
		unknown = -1,

		bad_address = EFAULT,
		file_exists = EEXIST,
		no_such_file_or_directory = ENOENT,
		not_enough_memory = ENOMEM,
		permission_denied = EACCES,
	};

	struct generic_code_messages
	{
		const char *msgs[256];
		constexpr size_t size() const { return sizeof(msgs) / sizeof(*msgs); }
		constexpr const char *operator[](int i) const { return (i < 0 || i >= static_cast<int>(size()) || nullptr == msgs[i]) ? "unknown" : msgs[i]; }
		constexpr generic_code_messages()
			: msgs{}
		{
			msgs[0] = "Success";

			msgs[EFAULT] = "Bad address";
			msgs[EEXIST] = "File exists";
			msgs[ENOENT] = "No such file or directory";
			msgs[ENOMEM] = "Cannot allocate memory";
			msgs[EACCES] = "Permission denied";
		}
	};

	enum class merrc : int
	{
		success = 0,
		unknown = -1,

		zero_pointer_access = 1,
		memory_access_violation = 2,
		out_of_range = 3,
	};

	struct memory_code_messages
	{
		const char *msgs[256];
		constexpr size_t size() const { return sizeof(msgs) / sizeof(*msgs); }
		constexpr const char *operator[](int i) const { return (i < 0 || i >= static_cast<int>(size()) || nullptr == msgs[i]) ? "unknown" : msgs[i]; }
		constexpr memory_code_messages()
			: msgs{}
		{
			msgs[0] = "Success";

			msgs[(int)(merrc::zero_pointer_access)] = "Zero pointer access";
			msgs[(int)(merrc::memory_access_violation)] = "Memory access violation";
			msgs[(int)(merrc::out_of_range)] = "Out of range";
		}
	};

	class string_ref
	{
		bool fromLiteral;
		const char* str;
		char* duplicate_str( const char* str_ ) { return _strdup( str_ ); }

	public:
		class literal_tag_t {};

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

	class error;

	class error_value
	{
	public:
		error_value() {}
		virtual ~error_value() {}
	};

	class error_domain
	{
	public:
		constexpr error_domain() {}
		virtual string_ref name() const { return string_ref( string_ref::literal_tag_t(), "unknown domain" ); }
		virtual string_ref value_to_meaasage(error_value* value) const { return string_ref( string_ref::literal_tag_t(), ""); }
		virtual ~error_domain() {}
		virtual error_value* clone_value(error_value* value) const { return value; }
		virtual bool is_same_error_code(const error_value* value1, const error_value* value2) const { return false; }
		virtual int _nodecpp_get_error_code(const error_value* value) const { return -1; } // for inter-domain comparison purposes only
		virtual void destroy_value(error_value* value) const {}
		virtual bool is_equivalent( const error& src, const error_value* my_value ) const { return false; }
	};

	class error
	{
	public:
		using DomainTypeT = error_domain;

	private:
		const error_domain* domain_ = nullptr;
		error_value* value_ = nullptr;

	public:
		constexpr error(const error_domain* domain, error_value* value_) : domain_( domain ), value_( value_ ) {}
		error( const error& other ) {
			domain_ = other.domain_;
			if ( domain_ )
				value_ = domain_->clone_value( other.value_ );
			else
				value_ = nullptr;
		}
		error operator = ( const error& other ) {
			domain_ = other.domain_;
			if ( domain_ )
				value_ = domain_->clone_value( other.value_ );
			else
				value_ = nullptr;
			return *this;
		}
		error( error&& other ) {
			domain_ = other.domain_;
			other.domain_ = nullptr;
			value_ = other.value_;
			other.value_ = nullptr;
		}
		error operator = ( error&& other ) {
			domain_ = other.domain_;
			other.domain_ = nullptr;
			value_ = other.value_;
			other.value_ = nullptr;
			return *this;
		}
		string_ref name() const { return domain_->name(); }
		string_ref description() const { return domain_->value_to_meaasage(value_); }
		~error() {
			if ( domain_ )
				domain_->destroy_value( value_ );
		}

		const error_domain* domain() const { return domain_; }
		error_value* value() const { return value_; }

		template <class ParticularError>
		inline bool
		operator==(const ParticularError &b) const
		{
			if ( domain_ != b.domain_ )
				return domain_->is_equivalent( b, value_ ) || reinterpret_cast<const typename ParticularError::DomainTypeT*>(b.domain_)->ParticularError::DomainTypeT::is_equivalent( *this, b.value_ );
			return domain_->is_same_error_code(value_, b.value_);
		}
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
	protected:
		virtual int _nodecpp_get_error_code(const error_value* value) const { return (int)(reinterpret_cast<const file_error_value*>(value)->errorCode); } // for inter-domain comparison purposes only

	public:
		constexpr file_error_domain() {}
		using Valuetype = file_error_value;
		virtual string_ref name() const { return string_ref( string_ref::literal_tag_t(), "file error" ); }
		virtual string_ref value_to_meaasage(error_value* value) const { 
			file_error_value* myData = reinterpret_cast<file_error_value*>(value);
			switch ( myData->errorCode )
			{
				case FILE_EXCEPTION::dne: { std::string s = fmt::format("\'{}\': file does not exist", myData->fileName.c_str()); return string_ref( s.c_str() ); break; }
				case FILE_EXCEPTION::access_denied: { std::string s = fmt::format("\'{}\': access denied", myData->fileName.c_str()); return string_ref( s.c_str() ); break; }
				default: return "unknown file error"; break;
			}
		}
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

	class system_error_domain : public error_domain
	{
	protected:
		virtual int _nodecpp_get_error_code(const error_value* value) const { return (int)(value); } // for inter-domain comparison purposes only

	public:
		constexpr system_error_domain() {}
		using Valuetype = errc;
		virtual string_ref name() const { return string_ref( string_ref::literal_tag_t(), "sytem domain" ); }
		virtual string_ref value_to_meaasage(error_value* value) const { 
			constexpr generic_code_messages msgs;
			return string_ref(msgs[reinterpret_cast<int>(value)]);
		}
		error_value* create_value( Valuetype code ) const {
			return reinterpret_cast<error_value*>(code);
		}
		virtual bool is_same_error_code(const error_value* value1, const error_value* value2) const { 
			return reinterpret_cast<int>(value1) == reinterpret_cast<int>(value2);
		}
		virtual error_value* clone_value(error_value* value) const {
			return value;
		}
		virtual void destroy_value(error_value* value) const {}
		virtual bool is_equivalent( const error& src, const error_value* my_value ) const {
			if ( src.domain() == this )
				return is_same_error_code( my_value, src.value() );
			else
				return false;
		}
	};
	extern const system_error_domain system_error_domain_obj;

	class memory_error_domain : public error_domain
	{
	protected:
		virtual int _nodecpp_get_error_code(const error_value* value) const { return (int)(value); } // for inter-domain comparison purposes only

	public:
		constexpr memory_error_domain() {}
		using Valuetype = merrc;
		virtual string_ref name() const { return string_ref( string_ref::literal_tag_t(), "sytem domain" ); }
		virtual string_ref value_to_meaasage(error_value* value) const { 
			constexpr memory_code_messages msgs;
			return string_ref(msgs[reinterpret_cast<int>(value)]);
		}
		error_value* create_value( Valuetype code ) const {
			return reinterpret_cast<error_value*>(code);
		}
		virtual bool is_same_error_code(const error_value* value1, const error_value* value2) const { 
			return reinterpret_cast<int>(value1) == reinterpret_cast<int>(value2);
		}
		virtual error_value* clone_value(error_value* value) const {
			return value;
		}
		virtual void destroy_value(error_value* value) const {}
		virtual bool is_equivalent( const error& src, const error_value* my_value ) const {
			if ( src.domain() == this )
				return is_same_error_code( my_value, src.value() );
			else if ( src.domain() == &system_error_domain_obj )
			{
				switch ( reinterpret_cast<int>(my_value) )
				{
					case (int)(merrc::zero_pointer_access): return src.domain()->_nodecpp_get_error_code(src.value()) == (int)(errc::bad_address); break;
					case (int)(merrc::memory_access_violation): return src.domain()->_nodecpp_get_error_code(src.value()) == (int)(errc::bad_address); break;
					default: return false; break;
				}
			}
			else
				return false;
		}
	};
	extern const memory_error_domain memory_error_domain_obj;

	class file_error : public error
	{
	public:
		using DomainTypeT = file_error_domain;
		file_error(FILE_EXCEPTION code, const char* fileName) : error( &file_error_domain_obj, file_error_domain_obj.create_value(code, fileName) ) {}
	};

	class system_error : public error
	{
	public:
		using DomainTypeT = system_error_domain;
		constexpr system_error(errc code) : error( &system_error_domain_obj, system_error_domain_obj.create_value(code) ) {}
	};

	extern const nodecpp::exception::system_error bad_address;
	extern const nodecpp::exception::system_error file_exists;
	extern const nodecpp::exception::system_error no_such_file_or_directory;
	extern const nodecpp::exception::system_error not_enough_memory;
	extern const nodecpp::exception::system_error permission_denied;

	class memory_error : public error
	{
	public:
		using DomainTypeT = memory_error_domain;
		constexpr memory_error(merrc code) : error( &memory_error_domain_obj, memory_error_domain_obj.create_value(code) ) {}
	};

	extern const nodecpp::exception::memory_error memory_access_violation;
	extern const nodecpp::exception::memory_error out_of_range;
	extern const nodecpp::exception::memory_error zero_pointer_access;

} // namespace nodecpp::exception


#endif // NODECPP_EXCEPTION_H
