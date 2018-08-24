//
//  kern_keyvalue.cpp
//  VirtualSMC
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_util.hpp>
#include <VirtualSMCSDK/kern_keyvalue.hpp>

bool VirtualSMCKeyValue::serializable(bool confidential) const {
	return value->serializable(confidential);
}

size_t VirtualSMCKeyValue::serializedSize() const {
	return sizeof(key) + sizeof(value->size) + value->size;
}

void VirtualSMCKeyValue::serialize(uint8_t *&dst) const {
	lilu_os_memcpy(dst, &key, sizeof(key));
	dst += sizeof(key);
	lilu_os_memcpy(dst, &value->size, sizeof(value->size));
	dst += sizeof(value->size);
	lilu_os_memcpy(dst, value->data, value->size);
	dst += value->size;
}

bool VirtualSMCKeyValue::deserialize(const uint8_t *&src, uint32_t &size, SMC_KEY &name, SMC_DATA *out, SMC_DATA_SIZE &outsz) {
	if (size < sizeof(SMC_KEY) + sizeof(SMC_DATA_SIZE))
		return false;

	lilu_os_memcpy(&name, src, sizeof(SMC_KEY));
	src += sizeof(SMC_KEY);
	lilu_os_memcpy(&outsz, src, sizeof(SMC_DATA_SIZE));
	src += sizeof(SMC_DATA_SIZE);
	size -= sizeof(SMC_KEY) + sizeof(SMC_DATA_SIZE);

	if (size >= outsz && size < SMC_MAX_DATA_SIZE) {
		lilu_os_memcpy(out, src, outsz);
		src += outsz;
		size -= outsz;
		return true;
	}

	return false;
}
