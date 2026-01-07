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

#define REGISTER_ORACLE_INTERFACE(Interface) {sizeof(Interface::OracleQuery), sizeof(Interface::OracleReply)}

	constexpr struct {
		unsigned long long querySize;
		unsigned long long replySize;
	} oracleInterfaces[] = {
		REGISTER_ORACLE_INTERFACE(Price),
		REGISTER_ORACLE_INTERFACE(Mock),
	};

	static constexpr uint32_t oracleInterfacesCount = sizeof(oracleInterfaces) / sizeof(oracleInterfaces[0]);

#undef REGISTER_ORACLE_INTERFACE

}
