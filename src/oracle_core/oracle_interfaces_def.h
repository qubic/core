#pragma once

/// Oracle Interfaces
namespace OI
{
#define DEFINE_ORACLE()

#define ORACLE_INTERFACE_INDEX 0
#include "oracle_interfaces/Price.h"
#undef ORACLE_INTERFACE_INDEX

#define ORACLE_INTERFACE_INDEX 1
#include "oracle_interfaces/Mock.h"
#undef ORACLE_INTERFACE_INDEX

#define DEFINE_ORACLE_INTERFACE(Interface) {sizeof(Interface::OracleQuery), sizeof(Interface::OracleReply)}

	constexpr struct {
		unsigned long long querySize;
		unsigned long long replySize;
	} oracleInterfaces[] = {
		DEFINE_ORACLE_INTERFACE(Price),
		DEFINE_ORACLE_INTERFACE(Mock),
	};

	static constexpr uint32_t oracleInterfacesCount = sizeof(oracleInterfaces) / sizeof(oracleInterfaces[0]);

#undef DEFINE_ORACLE_INTERFACE

	typedef sint64(*__GetQueryFeeFunc)(const void* query);

	static __GetQueryFeeFunc getOracleQueryFeeFunc[oracleInterfacesCount];

#define REGISTER_ORACLE_INTERFACE(Interface) { \
		getOracleQueryFeeFunc[Interface::oracleInterfaceIndex] = (__GetQueryFeeFunc)Interface::getQueryFee; \
		if (oracleInterfaces[Interface::oracleInterfaceIndex].querySize != sizeof(Interface::OracleQuery)) \
			return false; \
		if (oracleInterfaces[Interface::oracleInterfaceIndex].replySize != sizeof(Interface::OracleReply)) \
			return false; \
	}

	
	static bool initOracleInterfaces()
	{
		for (uint32_t idx = 0; idx < oracleInterfacesCount; ++idx)
		{
			getOracleQueryFeeFunc[idx] = nullptr;
		}

		REGISTER_ORACLE_INTERFACE(Price);
		REGISTER_ORACLE_INTERFACE(Mock);

		for (uint32_t idx = 0; idx < oracleInterfacesCount; ++idx)
		{
			if (!getOracleQueryFeeFunc[idx])
				return false;
		}
		return true;
	}


#define ENABLE_ORACLE_STATS_RECORD 1

}
