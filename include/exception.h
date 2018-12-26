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

	enum class FILE_EXCEPTION { dne, access_denied };
	enum class MEMORY_EXCEPTION { memory_access_violation, null_pointer_access, out_of_bound };

	class exception
	{
	public:
		virtual bool operator == ( FILE_EXCEPTION fe ) { return false; }
		virtual bool operator == ( MEMORY_EXCEPTION me ) { return false; }
		virtual const char* description() { return "unknown excption"; }
	};

	class file_exception : public exception
	{
		FILE_EXCEPTION fe;
	protected:
		file_exception( FILE_EXCEPTION fe_ ) : fe( fe_ ) {}
	};

	class file_exception_dne : public file_exception
	{
	public:
		file_exception_dne() : file_exception( FILE_EXCEPTION::dne ) {}
		virtual bool operator == ( FILE_EXCEPTION fe ) { return fe == FILE_EXCEPTION::dne; }
		virtual const char* description() { return "file does not exist"; }
	};

	class file_exception_access_denied : public file_exception
	{
	public:
		file_exception_access_denied() : file_exception( FILE_EXCEPTION::access_denied ) {}
		virtual bool operator == ( FILE_EXCEPTION fe ) { return fe == FILE_EXCEPTION::access_denied; }
		virtual const char* description() { return "access denied"; }
	};

	class memory_exception : public exception
	{
		MEMORY_EXCEPTION me;
	protected:
		memory_exception( MEMORY_EXCEPTION me_ ) : me( me_ ) {}
	};

	class memory_exception_memory_access_violation : public memory_exception
	{
	public:
		memory_exception_memory_access_violation() : memory_exception( MEMORY_EXCEPTION::memory_access_violation ) {}
		virtual bool operator == ( MEMORY_EXCEPTION me ) { return me == MEMORY_EXCEPTION::memory_access_violation; }
		virtual const char* description() { return "memory access violation"; }
	};

	class memory_exception_null_pointer_access : public memory_exception
	{
	public:
		memory_exception_null_pointer_access() : memory_exception( MEMORY_EXCEPTION::null_pointer_access ) {}
		virtual bool operator == ( MEMORY_EXCEPTION fe ) { return fe == MEMORY_EXCEPTION::null_pointer_access; }
		virtual const char* description() { return "null pointer access"; }
	};

	class memory_exception_out_of_bound : public memory_exception
	{
	public:
		memory_exception_out_of_bound() : memory_exception( MEMORY_EXCEPTION::out_of_bound ) {}
		virtual bool operator == ( MEMORY_EXCEPTION fe ) { return fe == MEMORY_EXCEPTION::out_of_bound; }
		virtual const char* description() { return "out of bound"; }
	};


	class exception1
	{
		enum type { file_exception, memory_exception };
		type t;
		union E
		{
			FILE_EXCEPTION fe;
			MEMORY_EXCEPTION me;
		};
		E e;

	public:
		constexpr exception1( FILE_EXCEPTION fe ) : t( file_exception ) { e.fe = fe; }
		constexpr exception1( MEMORY_EXCEPTION me ) : t( memory_exception ) { e.me = me; }
		bool operator == ( FILE_EXCEPTION fe ) { return t == file_exception && e.fe == fe; }
		bool operator == ( MEMORY_EXCEPTION me ) { return t == memory_exception && e.me == me; }
		const char* description()
		{
			switch ( t )
			{
				case file_exception:
				{
					switch ( e.fe )
					{
						case FILE_EXCEPTION::dne: return "file does not exist"; break;
						case FILE_EXCEPTION::access_denied: return "access denied"; break;
						default: return "unknown file exception"; break;
					}
					break;
				}
				case memory_exception:
				{
					switch ( e.me )
					{
						case MEMORY_EXCEPTION::memory_access_violation: return "memory access violation"; break;
						case MEMORY_EXCEPTION::null_pointer_access: return "null pointer access"; break;
						case MEMORY_EXCEPTION::out_of_bound: return "out of bound"; break;
						default: return "unknown memory exception"; break;
					}
					break;
				}
				default: return "unknown exception"; break;
			}
		}
	};

	class exception2
	{
		enum type { file_exception, memory_exception };
		type t;
		union E
		{
			FILE_EXCEPTION fe;
			MEMORY_EXCEPTION me;
		};
		E e;

	public:
		exception2( file_exception_dne fe ) { e.fe = FILE_EXCEPTION::dne; t = file_exception; }
		exception2( file_exception_access_denied fe ) { e.fe = FILE_EXCEPTION::access_denied; t = file_exception; }
		exception2( MEMORY_EXCEPTION me ) { e.me = me; t = memory_exception; }
		bool operator == ( FILE_EXCEPTION fe ) const { return t == file_exception && e.fe == fe; }
		bool operator == ( MEMORY_EXCEPTION me ) const { return t == memory_exception && e.me == me; }
		virtual const char* description() const
		{
			switch ( t )
			{
				case file_exception:
				{
					switch ( e.fe )
					{
						case FILE_EXCEPTION::dne: return "file does not exist"; break;
						case FILE_EXCEPTION::access_denied: return "access denied"; break;
						default: return "unknown file exception"; break;
					}
					break;
				}
				case memory_exception:
				{
					switch ( e.me )
					{
						case MEMORY_EXCEPTION::memory_access_violation: return "memory access violation"; break;
						case MEMORY_EXCEPTION::null_pointer_access: return "null pointer access"; break;
						case MEMORY_EXCEPTION::out_of_bound: return "out of bound"; break;
						default: return "unknown memory exception"; break;
					}
					break;
				}
				default: return "unknown exception"; break;
			}
		}
	};

	static constexpr exception1 file_exception_dne_{ FILE_EXCEPTION::dne };
	static constexpr exception1 file_exception_access_denied_{ FILE_EXCEPTION::access_denied };
	static constexpr exception1 memory_exception_memory_access_violation_{ MEMORY_EXCEPTION::memory_access_violation };
	static constexpr exception1 memory_exception_null_pointer_access_{ MEMORY_EXCEPTION::null_pointer_access };
	static constexpr exception1 memory_exception_out_of_bound_{ MEMORY_EXCEPTION::out_of_bound };

	class exception3
	{
		enum type { file_exception, memory_exception };
		type t;
		union E
		{
			FILE_EXCEPTION fe;
			MEMORY_EXCEPTION me;
		};
		E e;
		const char* what = "unknow exception";

	public:
		constexpr exception3( FILE_EXCEPTION fe, const char* what_ ) : t( file_exception ), what( what_ ) { e.fe = fe; }
		constexpr exception3( MEMORY_EXCEPTION me, const char* what_ ) : t( memory_exception ), what( what_ ) { e.me = me; }
		bool operator == ( FILE_EXCEPTION fe ) { return t == file_exception && e.fe == fe; }
		bool operator == ( MEMORY_EXCEPTION me ) { return t == memory_exception && e.me == me; }
		const char* description() { return what; }
	};

	static constexpr exception3 file_exception_dne__{ FILE_EXCEPTION::dne, "file does not exist" };
	static constexpr exception3 file_exception_access_denied__{ FILE_EXCEPTION::access_denied, "access denied" };
	static constexpr exception3 memory_exception_memory_access_violation__{ MEMORY_EXCEPTION::memory_access_violation, "memory access violation" };
	static constexpr exception3 memory_exception_null_pointer_access__{ MEMORY_EXCEPTION::null_pointer_access, "null pointer access" };
	static constexpr exception3 memory_exception_out_of_bound__{ MEMORY_EXCEPTION::out_of_bound, "out of bound" };

#if 0
	class exception_domain
	{
	public:
		enum Type { unknown_exception, file_exception, memory_exception };
		exception_domain() {}
		virtual Type type() { return unknown_exception; }
		virtual const char* name() { return "unknown exception"; }
		virtual ~exception_domain() {}
	};

	class file_exception_value;
	class file_exception_domain
	{
	public:
		using Valuetype = file_exception_value;
		virtual exception_domain::Type type() { return exception_domain::file_exception; }
		virtual const char* name() { return "file exception"; }
		virtual ~file_exception_domain() {}
	};

	class exception_value
	{
	public:
		exception_value() {}
		virtual ~exception_value() {}
	};

	class file_exception_value
	{
		std::string fileName;
	public:
		file_exception_value(const char* fileName_) : fileName(fileName_) {}
		virtual ~file_exception_value() {}
	};


	class exception4
	{
		exception_domain* domain;
		exception_value* value;

	public:
		constexpr exception4( FILE_EXCEPTION fe, const char* what_ ) : t( file_exception ), what( what_ ) { e.fe = fe; }
		constexpr exception4( MEMORY_EXCEPTION me, const char* what_ ) : t( memory_exception ), what( what_ ) { e.me = me; }
		bool operator == ( FILE_EXCEPTION fe ) { return t == file_exception && e.fe == fe; }
		bool operator == ( MEMORY_EXCEPTION me ) { return t == memory_exception && e.me == me; }
		const char* description() { return what; }
	};
#endif // 0

#if 0
	class file_exception_value
	{
		std::string fileName;
	public:
		file_exception_value(const char* fileName_) : fileName(fileName_) {}
		virtual ~file_exception_value() {}
	};

	enum DomainType { unknown_exception, file_exception, memory_exception };

	template<DomainType dt>
	class exception_domain
	{
	public:
		static constexpr DomainType myDomainType = dt;
		//const char* name() { return "unknown exception"; }
	};

	template<>
	class exception_domain<DomainType::unknown_exception>
	{
	public:
		const char* name() { return "unknown exception"; }
		using ValueType = size_t;
	};

	template<>
	class exception_domain<DomainType::file_exception>
	{
	public:
		const char* name() { return "file exception"; }
	};

	template<>
	class exception_domain<DomainType::memory_exception>
	{
	public:
		const char* name() { return "memory exception"; }
	};
#endif // 0


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

	//class file_error_value;
	class file_error_value : public error_value
	{
		friend class file_error_domain;
		FILE_EXCEPTION errorCode;
		std::string fileName;
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
				case FILE_EXCEPTION::dne: { std::string s = fmt::format("\'{}\': file does not exist", myData->fileName); return s.c_str(); break; }
				case FILE_EXCEPTION::access_denied: { std::string s = fmt::format("\'{}\': access denied", myData->fileName); return s.c_str(); break; }
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
//		constexpr error( FILE_EXCEPTION fe, const char* what_ ) : t( file_exception ), what( what_ ) { e.fe = fe; }
//		constexpr error( MEMORY_EXCEPTION me, const char* what_ ) : t( memory_exception ), what( what_ ) { e.me = me; }
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

/*	template<class Domain, class ... ValueArgs>
	class Exception7 : public error
	{
	public:
		Exception7(FILE_EXCEPTION code, const char* fileName)
		{
			domain = &file_error_domain_obj;
//			value = new file_error_value(code, fileName);
			value = file_error_domain_obj.create_value(code, fileName);
		}*/


} // namespace nodecpp::exception

#endif // NODECPP_EXCEPTION_H
