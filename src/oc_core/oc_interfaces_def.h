#pragma once

/// OC Interfaces
namespace OCI
{
#define DEFINE_OC()

#define OC_INTERFACE_INDEX 0
#include "oc_interfaces/Mock.h"
#undef OC_INTERFACE_INDEX

// add new interface above this line (define OC_INTERFACE_INDEX, include the header file and undef OC_INTERFACE_INDEX)

#define DEFINE_OC_INTERFACE(Interface) {sizeof(Interface::OcRequest)}

	constexpr struct {
		unsigned long long requestSize;
	} ocInterfaces[] = {
		DEFINE_OC_INTERFACE(Mock),
		// add new interface above this line (with DEFINE_OC_INTERFACE; the order must match the interface indices)
	};

	static constexpr uint32_t ocInterfacesCount = sizeof(ocInterfaces) / sizeof(ocInterfaces[0]);

#undef DEFINE_OC_INTERFACE

	typedef sint64(*__GetInvocationFeeFunc)(const void* request);

	static __GetInvocationFeeFunc getOcInvocationFeeFunc[ocInterfacesCount];

#define REGISTER_OC_INTERFACE(Interface) { \
		getOcInvocationFeeFunc[Interface::ocInterfaceIndex] = (__GetInvocationFeeFunc)Interface::getInvocationFee; \
		if (ocInterfaces[Interface::ocInterfaceIndex].requestSize != sizeof(Interface::OcRequest)) \
			return false; \
	}


	static bool initOcInterfaces()
	{
		for (uint32_t idx = 0; idx < ocInterfacesCount; ++idx)
		{
			getOcInvocationFeeFunc[idx] = nullptr;
		}

		REGISTER_OC_INTERFACE(Mock);
		// add new interface above this line (with REGISTER_OC_INTERFACE)

		for (uint32_t idx = 0; idx < ocInterfacesCount; ++idx)
		{
			if (!getOcInvocationFeeFunc[idx])
				return false;
		}
		return true;
	}

}
