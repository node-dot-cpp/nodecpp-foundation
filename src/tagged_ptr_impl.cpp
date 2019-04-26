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


#include "../include/foundation.h"
#include "../include/tagged_ptr_impl.h"

namespace nodecpp::platform::ptrwithdatastructsdefs { 

///////  ptr_with_zombie_property

void optimized_ptr_with_zombie_property_::throwNullptrOrZombieAccess() const {
	if (((uintptr_t)ptr) == zombie_indicator)
		throw ::nodecpp::error::lately_detected_zombie_pointer_access; 
	else
		throw ::nodecpp::error::zero_pointer_access; 
}

void optimized_ptr_with_zombie_property_::throwZombieAccess() const {
	throw ::nodecpp::error::lately_detected_zombie_pointer_access; 
}

void generic_ptr_with_zombie_property_::throwNullptrOrZombieAccess() const {
	if (isZombie)
		throw ::nodecpp::error::lately_detected_zombie_pointer_access; 
	else
	{
		NODECPP_ASSERT(nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ptr == nullptr );
		throw ::nodecpp::error::zero_pointer_access; 
	}
}

void generic_ptr_with_zombie_property_::throwZombieAccess() const {
	NODECPP_ASSERT(nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, isZombie );
	throw ::nodecpp::error::lately_detected_zombie_pointer_access; 
}

} // nodecpp::platform::ptrwithdatastructsdefs
