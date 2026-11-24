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

#define ORACLE_INTERFACE_INDEX 2
#include "oracle_interfaces/DogeShareValidation.h"
#undef ORACLE_INTERFACE_INDEX

#define ORACLE_INTERFACE_INDEX 3
#include "oracle_interfaces/EvmLogRead.h"
#undef ORACLE_INTERFACE_INDEX

#define ORACLE_INTERFACE_INDEX 4
#include "oracle_interfaces/QubicLogRead.h"
#undef ORACLE_INTERFACE_INDEX

// add new interface above this line (define ORACLE_INTERFACE_INDEX, include the header file and undef ORACLE_INTERFACE_INDEX)

#define DEFINE_ORACLE_INTERFACE(Interface) {sizeof(Interface::OracleQuery), sizeof(Interface::OracleReply)}

	constexpr struct {
		unsigned long long querySize;
		unsigned long long replySize;
	} oracleInterfaces[] = {
		DEFINE_ORACLE_INTERFACE(Price),
		DEFINE_ORACLE_INTERFACE(Mock),
		DEFINE_ORACLE_INTERFACE(DogeShareValidation),
		DEFINE_ORACLE_INTERFACE(EvmLogRead),
		DEFINE_ORACLE_INTERFACE(QubicLogRead),
		// add new interface above this line (with DEFINE_ORACLE_INTERFACE; the order must match the interfaces indices)
	};

	static constexpr uint32_t oracleInterfacesCount = sizeof(oracleInterfaces) / sizeof(oracleInterfaces[0]);

#undef DEFINE_ORACLE_INTERFACE

	typedef sint64(*__GetQueryFeeFunc)(const void* query);

	static __GetQueryFeeFunc getOracleQueryFeeFunc[oracleInterfacesCount];

	typedef bool(*__ReplyIsValidFunc)(const void* query, const void* reply);

	static __ReplyIsValidFunc replyIsValidFunc[oracleInterfacesCount];

	// Every interface must define replyIsValid(reply) or replyIsValid(query, reply).
	template <typename Interface>
	static bool replyIsValidErased(const void* query, const void* reply)
	{
		const auto& q = *(const typename Interface::OracleQuery*)query;
		const auto& r = *(const typename Interface::OracleReply*)reply;
		if constexpr (requires { Interface::replyIsValid(q, r); })
			return Interface::replyIsValid(q, r);
		else
			return Interface::replyIsValid(r);
	}

#define REGISTER_ORACLE_INTERFACE(Interface) { \
		getOracleQueryFeeFunc[Interface::oracleInterfaceIndex] = (__GetQueryFeeFunc)Interface::getQueryFee; \
		replyIsValidFunc[Interface::oracleInterfaceIndex] = replyIsValidErased<Interface>; \
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
			replyIsValidFunc[idx] = nullptr;
		}

		REGISTER_ORACLE_INTERFACE(Price);
		REGISTER_ORACLE_INTERFACE(Mock);
		REGISTER_ORACLE_INTERFACE(DogeShareValidation);
		REGISTER_ORACLE_INTERFACE(EvmLogRead);
		REGISTER_ORACLE_INTERFACE(QubicLogRead);
		// add new interface above this line (with REGISTER_ORACLE_INTERFACE)

		for (uint32_t idx = 0; idx < oracleInterfacesCount; ++idx)
		{
			if (!getOracleQueryFeeFunc[idx] || !replyIsValidFunc[idx])
				return false;
		}
		return true;
	}


#define ENABLE_ORACLE_STATS_RECORD 1

}
