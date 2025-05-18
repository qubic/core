#pragma once

// Return processor number of processor running this function
unsigned long long getRunningProcessorID();

// Check if running processor is main processor (bootstrap processor in EFI)
bool isMainProcessor();

// TODO: hide this?
extern unsigned long long mainThreadProcessorID;
