using namespace QPI;

/**
* OC interface "Mock".
*
* The mock interface accepts an arbitrary 64-bit value as request payload. OC machines
* forward the authorization bundle to a mock interface service that verifies the computor
* signatures and publishes the received invocations.
*/
struct Mock
{
	//-------------------------------------------------------------------------
	// Mandatory OC interface definitions

	/// OC interface index
	static constexpr uint32 ocInterfaceIndex = OC_INTERFACE_INDEX;

	/// OC request data / input to the OC machine
	struct OcRequest
	{
		/// Arbitrary value forwarded to the mock interface service
		uint64 value;
	};

	/// Return invocation fee; constant for the mock
	static sint64 getInvocationFee(const OcRequest& request)
	{
		return 10;
	}
};
