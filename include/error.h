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

#ifndef NODECPP_ERROR_H
#define NODECPP_ERROR_H 

#include "platform_base.h"
#include "log.h"
#include "string_ref.h"
#ifndef NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
#include "stack_info.h"
#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS

namespace nodecpp::error {

	class error;

	class error_value
	{
	public:
#ifdef NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
		error_value() {}
#else
		::nodecpp::StackInfo stackInfo;
		error_value() { stackInfo.init(); }
#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
		virtual ~error_value() {
		}
	};

	class error_domain
	{
	public:
		constexpr error_domain() {}
		virtual string_ref name() const { return string_ref( string_ref::literal_tag_t(), "unknown domain" ); }
		virtual string_ref value_to_message(error_value* value) const { return string_ref( string_ref::literal_tag_t(), ""); }
#ifdef NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
		virtual void log(error_value* value, log::LogLevel l ) const { log::default_log::log( l, "Error" ); }
		virtual void log(error_value* value, log::Log& targetLog, log::LogLevel l ) const { targetLog.log( l, "Error" ); }
#else
		virtual void log(error_value* value, log::LogLevel l ) const { 
			if ( ::nodecpp::impl::isDataStackInfo( value->stackInfo ) )
			{
				log::default_log::log( l, "An error happened. Stack info:" ); 
				value->stackInfo.log( l );
			}
			else
				log::default_log::log( l, "Error" ); 
		}
		virtual void log(error_value* value, log::Log& targetLog, log::LogLevel l ) const { 
			if ( ::nodecpp::impl::isDataStackInfo( value->stackInfo ) )
			{
				targetLog.log( l, "An error happened. Stack info:" ); 
				value->stackInfo.log( targetLog, l );
			}
			else
				log::default_log::log( l, "Error" ); 
		}
#endif // NODECPP_NO_STACK_INFO_IN_EXCEPTIONS
		virtual ~error_domain() {}
		virtual error_value* clone_value(error_value* value) const { return value; }
		virtual bool is_same_error_code(const error_value* value1, const error_value* value2) const { return false; }
		virtual uintptr_t _nodecpp_get_error_code(const error_value* value) const { return -1; } // for inter-domain comparison purposes only
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
		string_ref description() const { return domain_->value_to_message( value_ ); }
		void log( log::LogLevel l ) const { domain_->log( value_, l ); }
		void log( log::Log& targetLog, log::LogLevel l ) const { domain_->log( value_, targetLog, l ); }
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

} // namespace nodecpp::error


#endif // NODECPP_ERROR_H
