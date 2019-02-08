
#include "../include/ptr_with_data_structs_impl.h"

namespace nodecpp::platform::ptrwithdatastructsdefs { 

///////  ptr_with_zombie_property

void optimized_ptr_with_zombie_property_::throwNullptrOrZombieAccess() const {
	if (((uintptr_t)ptr) == alignof(void*))
		throw nodecpp::error::zombie_pointer_access; 
	else
		throw nodecpp::error::zero_pointer_access; 
}

void optimized_ptr_with_zombie_property_::throwZombieAccess() const {
	throw nodecpp::error::zombie_pointer_access; 
}

void generic_ptr_with_zombie_property_::throwNullptrOrZombieAccess() const {
	if (((uintptr_t)ptr) == alignof(void*))
		throw nodecpp::error::zombie_pointer_access; 
	else
		throw nodecpp::error::zero_pointer_access; 
}

void generic_ptr_with_zombie_property_::throwZombieAccess() const {
	throw nodecpp::error::zombie_pointer_access; 
}

} // nodecpp::platform::ptrwithdatastructsdefs
