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

#include "../include/exception.h"

namespace nodecpp::exception {

	const system_error_domain system_error_domain_obj;

	const nodecpp::exception::system_error bad_address( nodecpp::exception::errc::bad_address );
	const nodecpp::exception::system_error file_exists( nodecpp::exception::errc::file_exists );
	const nodecpp::exception::system_error no_such_file_or_directory( nodecpp::exception::errc::no_such_file_or_directory );
	const nodecpp::exception::system_error not_enough_memory( nodecpp::exception::errc::not_enough_memory );
	const nodecpp::exception::system_error permission_denied( nodecpp::exception::errc::permission_denied );


	const memory_error_domain memory_error_domain_obj;

	const nodecpp::exception::memory_error memory_access_violation( nodecpp::exception::merrc::memory_access_violation );
	const nodecpp::exception::memory_error out_of_range( nodecpp::exception::merrc::out_of_range );
	const nodecpp::exception::memory_error zero_pointer_access( nodecpp::exception::merrc::zero_pointer_access );

} // namespace nodecpp::exception
