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

#ifndef NODECPP_SAFE_MEMORY_ERROR_H
#define NODECPP_SAFE_MEMORY_ERROR_H 

#include "platform_base.h"
#include "error.h"
#include "std_error.h" // for error conversion purposes

#include <cerrno>  // for error constants

namespace nodecpp::error {

	enum class merrc : int
	{
		success = 0,
		unknown = -1,

		zero_pointer_access = 1,
		memory_access_violation = 2,
		out_of_range = 3,
		lately_detected_zombie_pointer_access = 4,
		early_detected_zombie_pointer_access = 5,
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
			msgs[(int)(merrc::lately_detected_zombie_pointer_access)] = "Lately detected zombie pointer access";
			msgs[(int)(merrc::early_detected_zombie_pointer_access)] = "Early detected zombie pointer access";
		}
	};

#ifndef NODECPP_NO_STACK_INFO_IN_EXCEPTIONS // Note: in this case the Value is a bit more than just an error code
	class memory_error_value : public error_value
	{
		friend class memory_error_domain;
		merrc errorCode;
	public:
		memory_error_value( merrc code ) : errorCode( code ) {}
		memory_error_value( const memory_error_value& other ) = default;
		memory_error_value& operator = ( const memory_error_value& other ) = default;
		memory_error_value& operator = ( memory_error_value&& other ) = default;
		memory_error_value( memory_error_value&& other ) = default;
		virtual ~memory_error_value() {}
	};
#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS



	class memory_error_domain : public error_domain
	{
	protected:
#ifdef NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
		virtual uintptr_t _nodecpp_get_error_code(const error_value* value) const { return (uintptr_t)(value); } // for inter-domain comparison purposes only
#else
		virtual uintptr_t _nodecpp_get_error_code(const error_value* value) const { return (uintptr_t)(((memory_error_value*)(value))->errorCode); } // for inter-domain comparison purposes only
#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS

	public:
		memory_error_domain() {}
		virtual string_ref name() const { return string_ref( string_ref::literal_tag_t(), "memory domain" ); }
#ifdef NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
		using Valuetype = merrc;
		virtual string_ref value_to_message(error_value* value) const { 
			constexpr memory_code_messages msgs;
			return string_ref(msgs[(int)(uintptr_t)(value)]);
		}
		virtual void log(error_value* value, log::LogLevel l ) const { log::default_log::log( l, "{}", value_to_message( value ).c_str() ); }
		virtual void log(error_value* value, log::Log& targetLog, log::LogLevel l ) const { targetLog.log( l, "{}", value_to_message( value ).c_str() ); }
		error_value* create_value( Valuetype code ) const {
			return reinterpret_cast<error_value*>(code);
		}
		virtual bool is_same_error_code(const error_value* value1, const error_value* value2) const { 
			return reinterpret_cast<uintptr_t>(value1) == reinterpret_cast<uintptr_t>(value2);
		}
		virtual error_value* clone_value(error_value* value) const {
			return value;
		}
		virtual void destroy_value(error_value* value) const {}
#else
		using Valuetype = memory_error_value;
		virtual string_ref value_to_message(error_value* value) const { 
			constexpr memory_code_messages msgs;
			return string_ref(msgs[(int)((memory_error_value*)(value))->errorCode]);
		}
		virtual void log(error_value* value, log::LogLevel l ) const { 
			if ( ::nodecpp::impl::isDataStackInfo( value->stackInfo ) )
			{
				log::default_log::log( l, "{} happened at", value_to_message( value ).c_str() );
				value->stackInfo.log( l );
			}
			else
				log::default_log::log( l, "{}", value_to_message( value ).c_str() );
		}
		virtual void log(error_value* value, log::Log& targetLog, log::LogLevel l ) const { 
			if ( ::nodecpp::impl::isDataStackInfo( value->stackInfo ) )
			{
				targetLog.log( l, "{} happened at", value_to_message( value ).c_str() );
				value->stackInfo.log( targetLog, l );
			}
			else
				targetLog.log( l, "{}", value_to_message( value ).c_str() );
		}
		error_value* create_value( Valuetype code ) const {
			return new memory_error_value(code);
		}
		virtual bool is_same_error_code(const error_value* value1, const error_value* value2) const { 
			return (static_cast<const memory_error_value*>(value1))->errorCode == (static_cast<const memory_error_value*>(value2))->errorCode;
		}
		virtual error_value* clone_value(error_value* value) const {
			return new memory_error_value(*static_cast<const memory_error_value*>(value));
		}
		virtual void destroy_value(error_value* value) const {
			if ( value ) {
				memory_error_value* myData = static_cast<memory_error_value*>(value);
				delete myData;
			}
		}
#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS

		virtual bool is_equivalent( const error& src, const error_value* my_value ) const {
			if ( src.domain() == this )
				return is_same_error_code( my_value, src.value() );
			else if ( src.domain() == &std_error_domain_obj )
			{
				switch ( reinterpret_cast<uintptr_t>(my_value) )
				{
					case (int)(merrc::zero_pointer_access): return src.domain()->_nodecpp_get_error_code(src.value()) == (int)(errc::bad_address); break;
					case (int)(merrc::memory_access_violation): return src.domain()->_nodecpp_get_error_code(src.value()) == (int)(errc::bad_address); break;
					case (int)(merrc::lately_detected_zombie_pointer_access): return src.domain()->_nodecpp_get_error_code(src.value()) == (int)(errc::bad_address); break;
					case (int)(merrc::early_detected_zombie_pointer_access): return src.domain()->_nodecpp_get_error_code(src.value()) == (int)(errc::bad_address); break;
					default: return false; break;
				}
			}
			else
				return false;
		}
	};
	extern const memory_error_domain memory_error_domain_obj;

	class memory_error : public error
	{
	public:
		using DomainTypeT = memory_error_domain;
		memory_error(merrc code) : error( &memory_error_domain_obj, memory_error_domain_obj.create_value(code) ) {}
	};

	extern const nodecpp::error::memory_error memory_access_violation;
	extern const nodecpp::error::memory_error out_of_range;
	extern const nodecpp::error::memory_error zero_pointer_access;
	extern const nodecpp::error::memory_error lately_detected_zombie_pointer_access;
	extern const nodecpp::error::memory_error early_detected_zombie_pointer_access;

} // namespace nodecpp::error


#endif // NODECPP_SAFE_MEMORY_ERROR_H
