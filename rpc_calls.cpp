// RPC functions
// This is an auto generated file

#include "pixel_dtb.h"
#include <iostream> // cout

const char CTestboard::rpc_timestamp[] = "";

const unsigned int CTestboard::rpc_cmdListSize = 125;

const char *CTestboard::rpc_cmdName[] =
{
	/*     0 */ "GetRpcVersion$S",
	/*     1 */ "GetRpcCallId$i3c",
	/*     2 */ "GetRpcTimestamp$v4c",
	/*     3 */ "GetRpcCallCount$i",
	/*     4 */ "GetRpcCallName$bi4c",
	/*     5 */ "GetRpcCallHash$I",
	/*     6 */ "GetInfo$v4c",
	/*     7 */ "GetBoardId$S",
	/*     8 */ "GetHWVersion$v4c",
	/*     9 */ "GetFWVersion$S",
	/*    10 */ "GetSWVersion$S",
	/*    11 */ "UpgradeGetVersion$S",
	/*    12 */ "UpgradeStart$CS",
	/*    13 */ "UpgradeData$C3c",
	/*    14 */ "UpgradeError$C",
	/*    15 */ "UpgradeErrorMsg$v4c",
	/*    16 */ "UpgradeExec$vS",
	/*    17 */ "Init$v",
	/*    18 */ "Welcome$v",
	/*    19 */ "SetLed$vC",
	/*    20 */ "cDelay$vS",
	/*    21 */ "uDelay$vS",
	/*    22 */ "SetClockSource$vC",
	/*    23 */ "IsClockPresent$b",
	/*    24 */ "SetClock$vC",
	/*    25 */ "SetClockStretch$vCSS",
	/*    26 */ "Sig_SetMode$vCC",
	/*    27 */ "Sig_SetPRBS$vCC",
	/*    28 */ "Sig_SetDelay$vCSc",
	/*    29 */ "Sig_SetLevel$vCC",
	/*    30 */ "Sig_SetOffset$vC",
	/*    31 */ "Sig_SetLVDS$v",
	/*    32 */ "Sig_SetLCDS$v",
	/*    33 */ "SignalProbeD1$vC",
	/*    34 */ "SignalProbeD2$vC",
	/*    35 */ "SignalProbeA1$vC",
	/*    36 */ "SignalProbeA2$vC",
	/*    37 */ "SignalProbeADC$vCC",
	/*    38 */ "Pon$v",
	/*    39 */ "Poff$v",
	/*    40 */ "_SetVD$vS",
	/*    41 */ "_SetVA$vS",
	/*    42 */ "_SetID$vS",
	/*    43 */ "_SetIA$vS",
	/*    44 */ "_GetVD$S",
	/*    45 */ "_GetVA$S",
	/*    46 */ "_GetID$S",
	/*    47 */ "_GetIA$S",
	/*    48 */ "HVon$v",
	/*    49 */ "HVoff$v",
	/*    50 */ "ResetOn$v",
	/*    51 */ "ResetOff$v",
	/*    52 */ "GetStatus$C",
	/*    53 */ "SetRocAddress$vC",
	/*    54 */ "Pg_SetCmd$vSS",
	/*    55 */ "Pg_Stop$v",
	/*    56 */ "Pg_Single$v",
	/*    57 */ "Pg_Trigger$v",
	/*    58 */ "Pg_Loop$vS",
	/*    59 */ "Daq_Open$IIC",
	/*    60 */ "Daq_Close$vC",
	/*    61 */ "Daq_Start$vC",
	/*    62 */ "Daq_Stop$vC",
	/*    63 */ "Daq_GetSize$IC",
	/*    64 */ "Daq_FillLevel$CC",
	/*    65 */ "Daq_FillLevel$C",
	/*    66 */ "Daq_Read$C5SIC",
	/*    67 */ "Daq_Read$C5SI0IC",
	/*    68 */ "Daq_Select_ADC$vSCCC",
	/*    69 */ "Daq_Select_Deser160$vC",
	/*    70 */ "Daq_Select_Deser400$v",
	/*    71 */ "Daq_Deser400_Reset$vC",
	/*    72 */ "Daq_Select_Datagenerator$vS",
	/*    73 */ "Daq_DeselectAll$v",
	/*    74 */ "roc_I2cAddr$vC",
	/*    75 */ "roc_ClrCal$v",
	/*    76 */ "roc_SetDAC$vCC",
	/*    77 */ "roc_Pix$vCCC",
	/*    78 */ "roc_Pix_Trim$vCCC",
	/*    79 */ "roc_Pix_Mask$vCC",
	/*    80 */ "roc_Pix_Cal$vCCb",
	/*    81 */ "roc_Col_Enable$vCb",
	/*    82 */ "roc_Col_Mask$vC",
	/*    83 */ "roc_Chip_Mask$v",
	/*    84 */ "TBM_Present$b",
	/*    85 */ "tbm_Enable$vb",
	/*    86 */ "tbm_Addr$vCC",
	/*    87 */ "mod_Addr$vC",
	/*    88 */ "tbm_Set$vCC",
	/*    89 */ "tbm_Get$bC0C",
	/*    90 */ "tbm_GetRaw$bC0I",
	/*    91 */ "GetPixelAddressInverted$b",
	/*    92 */ "SetPixelAddressInverted$vb",
	/*    93 */ "CountReadouts$ii",
	/*    94 */ "CountReadouts$iii",
	/*    95 */ "CountReadouts$iiii",
	/*    96 */ "PH$iiiis",
	/*    97 */ "PixelThreshold$iiiiiiiibb",
	/*    98 */ "test_pixel_address$bii",
	/*    99 */ "CalibratePixel$csss0s0i",
	/*   100 */ "CalibrateDacScan$cssssss2s2i",
	/*   101 */ "CalibrateDacDacScan$csssssssss2s2i",
	/*   102 */ "TrimChip$s1s",
	/*   103 */ "CalibrateMap$ss2s2i2I",
	/*   104 */ "TriggerRow$sss1ss",
	/*   105 */ "TestColPixel$bCCb2C",
	/*   106 */ "Ethernet_Send$v3c",
	/*   107 */ "Ethernet_RecvPackets$I",
	/*   108 */ "LoopInterruptReset$v",
	/*   109 */ "SetLoopTriggerDelay$vS",
	/*   110 */ "SetI2CAddresses$b1C",
	/*   111 */ "SetTrimValues$bC1c",
	/*   112 */ "LoopMultiRocAllPixelsCalibrate$b1CSS",
	/*   113 */ "LoopMultiRocOnePixelCalibrate$b1CCCSS",
	/*   114 */ "LoopSingleRocAllPixelsCalibrate$bCSS",
	/*   115 */ "LoopSingleRocOnePixelCalibrate$bCCCSS",
	/*   116 */ "LoopMultiRocAllPixelsDacScan$b1CSSCCC",
	/*   117 */ "LoopMultiRocOnePixelDacScan$b1CCCSSCCC",
	/*   118 */ "LoopSingleRocAllPixelsDacScan$bCSSCCC",
	/*   119 */ "LoopSingleRocOnePixelDacScan$bCCCSSCCC",
	/*   120 */ "LoopMultiRocAllPixelsDacDacScan$b1CSSCCCCCC",
	/*   121 */ "LoopMultiRocOnePixelDacDacScan$b1CCCSSCCCCCC",
	/*   122 */ "LoopSingleRocAllPixelsDacDacScan$bCSSCCCCCC",
	/*   123 */ "LoopSingleRocOnePixelDacDacScan$bCCCSSCCCCCC",
	/*   124 */ "VectorTest$v1S2S"
};

uint16_t CTestboard::GetRpcVersion()
{ //RPC_PROFILING
	uint16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(0);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_UINT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(0); throw; };
	return rpc_par0;
}

int32_t CTestboard::GetRpcCallId(string &rpc_par1)
{ //RPC_PROFILING
	int32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(1);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_INT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(1); throw; };
	return rpc_par0;
}

void CTestboard::GetRpcTimestamp(stringR &rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(2);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,0);
	rpc_Receive(*rpc_io, rpc_par1);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(2); throw; };
}

int32_t CTestboard::GetRpcCallCount()
{ //RPC_PROFILING
	int32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(3);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_INT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(3); throw; };
	return rpc_par0;
}

bool CTestboard::GetRpcCallName(int32_t rpc_par1, stringR &rpc_par2)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(4);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT32(rpc_par1);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	rpc_Receive(*rpc_io, rpc_par2);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(4); throw; };
	return rpc_par0;
}

uint32_t CTestboard::GetRpcCallHash()
{ //RPC_PROFILING
	uint32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(5);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_UINT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(5); throw; };
	return rpc_par0;
}

void CTestboard::GetInfo(stringR &rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(6);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,0);
	rpc_Receive(*rpc_io, rpc_par1);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(6); throw; };
}

uint16_t CTestboard::GetBoardId()
{ //RPC_PROFILING
	uint16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(7);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_UINT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(7); throw; };
	return rpc_par0;
}

void CTestboard::GetHWVersion(stringR &rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(8);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,0);
	rpc_Receive(*rpc_io, rpc_par1);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(8); throw; };
}

uint16_t CTestboard::GetFWVersion()
{ //RPC_PROFILING
	uint16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(9);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_UINT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(9); throw; };
	return rpc_par0;
}

uint16_t CTestboard::GetSWVersion()
{ //RPC_PROFILING
	uint16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(10);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_UINT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(10); throw; };
	return rpc_par0;
}

uint16_t CTestboard::UpgradeGetVersion()
{ //RPC_PROFILING
	uint16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(11);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_UINT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(11); throw; };
	return rpc_par0;
}

uint8_t CTestboard::UpgradeStart(uint16_t rpc_par1)
{ //RPC_PROFILING
	uint8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(12);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_UINT8();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(12); throw; };
	return rpc_par0;
}

uint8_t CTestboard::UpgradeData(string &rpc_par1)
{ //RPC_PROFILING
	uint8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(13);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_UINT8();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(13); throw; };
	return rpc_par0;
}

uint8_t CTestboard::UpgradeError()
{ //RPC_PROFILING
	uint8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(14);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_UINT8();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(14); throw; };
	return rpc_par0;
}

void CTestboard::UpgradeErrorMsg(stringR &rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(15);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,0);
	rpc_Receive(*rpc_io, rpc_par1);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(15); throw; };
}

void CTestboard::UpgradeExec(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(16);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(16); throw; };
}

void CTestboard::Init()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(17);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(17); throw; };
}

void CTestboard::Welcome()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(18);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(18); throw; };
}

void CTestboard::SetLed(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(19);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(19); throw; };
}

void CTestboard::cDelay(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(20);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(20); throw; };
}

void CTestboard::uDelay(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(21);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(21); throw; };
}

void CTestboard::SetClockSource(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(22);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(22); throw; };
}

bool CTestboard::IsClockPresent()
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(23);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(23); throw; };
	return rpc_par0;
}

void CTestboard::SetClock(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(24);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(24); throw; };
}

void CTestboard::SetClockStretch(uint8_t rpc_par1, uint16_t rpc_par2, uint16_t rpc_par3)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(25);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT16(rpc_par2);
	msg.Put_UINT16(rpc_par3);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(25); throw; };
}

void CTestboard::Sig_SetMode(uint8_t rpc_par1, uint8_t rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(26);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(26); throw; };
}

void CTestboard::Sig_SetPRBS(uint8_t rpc_par1, uint8_t rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(27);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(27); throw; };
}

void CTestboard::Sig_SetDelay(uint8_t rpc_par1, uint16_t rpc_par2, int8_t rpc_par3)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(28);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT16(rpc_par2);
	msg.Put_INT8(rpc_par3);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(28); throw; };
}

void CTestboard::Sig_SetLevel(uint8_t rpc_par1, uint8_t rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(29);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(29); throw; };
}

void CTestboard::Sig_SetOffset(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(30);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(30); throw; };
}

void CTestboard::Sig_SetLVDS()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(31);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(31); throw; };
}

void CTestboard::Sig_SetLCDS()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(32);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(32); throw; };
}

void CTestboard::SignalProbeD1(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(33);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(33); throw; };
}

void CTestboard::SignalProbeD2(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(34);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(34); throw; };
}

void CTestboard::SignalProbeA1(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(35);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(35); throw; };
}

void CTestboard::SignalProbeA2(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(36);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(36); throw; };
}

void CTestboard::SignalProbeADC(uint8_t rpc_par1, uint8_t rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(37);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(37); throw; };
}

void CTestboard::Pon()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(38);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(38); throw; };
}

void CTestboard::Poff()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(39);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(39); throw; };
}

void CTestboard::_SetVD(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(40);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(40); throw; };
}

void CTestboard::_SetVA(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(41);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(41); throw; };
}

void CTestboard::_SetID(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(42);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(42); throw; };
}

void CTestboard::_SetIA(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(43);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(43); throw; };
}

uint16_t CTestboard::_GetVD()
{ //RPC_PROFILING
	uint16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(44);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_UINT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(44); throw; };
	return rpc_par0;
}

uint16_t CTestboard::_GetVA()
{ //RPC_PROFILING
	uint16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(45);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_UINT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(45); throw; };
	return rpc_par0;
}

uint16_t CTestboard::_GetID()
{ //RPC_PROFILING
	uint16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(46);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_UINT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(46); throw; };
	return rpc_par0;
}

uint16_t CTestboard::_GetIA()
{ //RPC_PROFILING
	uint16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(47);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_UINT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(47); throw; };
	return rpc_par0;
}

void CTestboard::HVon()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(48);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(48); throw; };
}

void CTestboard::HVoff()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(49);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(49); throw; };
}

void CTestboard::ResetOn()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(50);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(50); throw; };
}

void CTestboard::ResetOff()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(51);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(51); throw; };
}

uint8_t CTestboard::GetStatus()
{ //RPC_PROFILING
	uint8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(52);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_UINT8();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(52); throw; };
	return rpc_par0;
}

void CTestboard::SetRocAddress(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(53);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(53); throw; };
}

void CTestboard::Pg_SetCmd(uint16_t rpc_par1, uint16_t rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(54);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Put_UINT16(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(54); throw; };
}

void CTestboard::Pg_Stop()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(55);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(55); throw; };
}

void CTestboard::Pg_Single()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(56);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(56); throw; };
}

void CTestboard::Pg_Trigger()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(57);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(57); throw; };
}

void CTestboard::Pg_Loop(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(58);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(58); throw; };
}

uint32_t CTestboard::Daq_Open(uint32_t rpc_par1, uint8_t rpc_par2)
{ //RPC_PROFILING
	uint32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(59);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT32(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_UINT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(59); throw; };
	return rpc_par0;
}

void CTestboard::Daq_Close(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(60);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(60); throw; };
}

void CTestboard::Daq_Start(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(61);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(61); throw; };
}

void CTestboard::Daq_Stop(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(62);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(62); throw; };
}

uint32_t CTestboard::Daq_GetSize(uint8_t rpc_par1)
{ //RPC_PROFILING
	uint32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(63);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_UINT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(63); throw; };
	return rpc_par0;
}

uint8_t CTestboard::Daq_FillLevel(uint8_t rpc_par1)
{ //RPC_PROFILING
	uint8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(64);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_UINT8();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(64); throw; };
	return rpc_par0;
}

uint8_t CTestboard::Daq_FillLevel()
{ //RPC_PROFILING
	uint8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(65);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_UINT8();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(65); throw; };
	return rpc_par0;
}

uint8_t CTestboard::Daq_Read(HWvectorR<uint16_t> &rpc_par1, uint32_t rpc_par2, uint8_t rpc_par3)
{ //RPC_PROFILING
	uint8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(66);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT32(rpc_par2);
	msg.Put_UINT8(rpc_par3);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_UINT8();
	rpc_Receive(*rpc_io, rpc_par1);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(66); throw; };
	return rpc_par0;
}

uint8_t CTestboard::Daq_Read(HWvectorR<uint16_t> &rpc_par1, uint32_t rpc_par2, uint32_t &rpc_par3, uint8_t rpc_par4)
{ //RPC_PROFILING
	uint8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(67);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT32(rpc_par2);
	msg.Put_UINT32(rpc_par3);
	msg.Put_UINT8(rpc_par4);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,5);
	rpc_par0 = msg.Get_UINT8();
	rpc_par3 = msg.Get_UINT32();
	rpc_Receive(*rpc_io, rpc_par1);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(67); throw; };
	return rpc_par0;
}

void CTestboard::Daq_Select_ADC(uint16_t rpc_par1, uint8_t rpc_par2, uint8_t rpc_par3, uint8_t rpc_par4)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(68);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Put_UINT8(rpc_par3);
	msg.Put_UINT8(rpc_par4);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(68); throw; };
}

void CTestboard::Daq_Select_Deser160(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(69);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(69); throw; };
}

void CTestboard::Daq_Select_Deser400()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(70);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(70); throw; };
}

void CTestboard::Daq_Deser400_Reset(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(71);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(71); throw; };
}

void CTestboard::Daq_Select_Datagenerator(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(72);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(72); throw; };
}

void CTestboard::Daq_DeselectAll()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(73);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(73); throw; };
}

void CTestboard::roc_I2cAddr(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(74);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(74); throw; };
}

void CTestboard::roc_ClrCal()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(75);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(75); throw; };
}

void CTestboard::roc_SetDAC(uint8_t rpc_par1, uint8_t rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(76);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(76); throw; };
}

void CTestboard::roc_Pix(uint8_t rpc_par1, uint8_t rpc_par2, uint8_t rpc_par3)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(77);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Put_UINT8(rpc_par3);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(77); throw; };
}

void CTestboard::roc_Pix_Trim(uint8_t rpc_par1, uint8_t rpc_par2, uint8_t rpc_par3)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(78);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Put_UINT8(rpc_par3);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(78); throw; };
}

void CTestboard::roc_Pix_Mask(uint8_t rpc_par1, uint8_t rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(79);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(79); throw; };
}

void CTestboard::roc_Pix_Cal(uint8_t rpc_par1, uint8_t rpc_par2, bool rpc_par3)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(80);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Put_BOOL(rpc_par3);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(80); throw; };
}

void CTestboard::roc_Col_Enable(uint8_t rpc_par1, bool rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(81);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_BOOL(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(81); throw; };
}

void CTestboard::roc_Col_Mask(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(82);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(82); throw; };
}

void CTestboard::roc_Chip_Mask()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(83);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(83); throw; };
}

bool CTestboard::TBM_Present()
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(84);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(84); throw; };
	return rpc_par0;
}

void CTestboard::tbm_Enable(bool rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(85);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_BOOL(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(85); throw; };
}

void CTestboard::tbm_Addr(uint8_t rpc_par1, uint8_t rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(86);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(86); throw; };
}

void CTestboard::mod_Addr(uint8_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(87);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(87); throw; };
}

void CTestboard::tbm_Set(uint8_t rpc_par1, uint8_t rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(88);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(88); throw; };
}

bool CTestboard::tbm_Get(uint8_t rpc_par1, uint8_t &rpc_par2)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(89);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_BOOL();
	rpc_par2 = msg.Get_UINT8();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(89); throw; };
	return rpc_par0;
}

bool CTestboard::tbm_GetRaw(uint8_t rpc_par1, uint32_t &rpc_par2)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(90);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT32(rpc_par2);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,5);
	rpc_par0 = msg.Get_BOOL();
	rpc_par2 = msg.Get_UINT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(90); throw; };
	return rpc_par0;
}

bool CTestboard::GetPixelAddressInverted()
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(91);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(91); throw; };
	return rpc_par0;
}

void CTestboard::SetPixelAddressInverted(bool rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(92);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_BOOL(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(92); throw; };
}

int32_t CTestboard::CountReadouts(int32_t rpc_par1)
{ //RPC_PROFILING
	int32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(93);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT32(rpc_par1);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_INT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(93); throw; };
	return rpc_par0;
}

int32_t CTestboard::CountReadouts(int32_t rpc_par1, int32_t rpc_par2)
{ //RPC_PROFILING
	int32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(94);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT32(rpc_par1);
	msg.Put_INT32(rpc_par2);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_INT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(94); throw; };
	return rpc_par0;
}

int32_t CTestboard::CountReadouts(int32_t rpc_par1, int32_t rpc_par2, int32_t rpc_par3)
{ //RPC_PROFILING
	int32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(95);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT32(rpc_par1);
	msg.Put_INT32(rpc_par2);
	msg.Put_INT32(rpc_par3);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_INT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(95); throw; };
	return rpc_par0;
}

int32_t CTestboard::PH(int32_t rpc_par1, int32_t rpc_par2, int32_t rpc_par3, int16_t rpc_par4)
{ //RPC_PROFILING
	int32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(96);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT32(rpc_par1);
	msg.Put_INT32(rpc_par2);
	msg.Put_INT32(rpc_par3);
	msg.Put_INT16(rpc_par4);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_INT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(96); throw; };
	return rpc_par0;
}

int32_t CTestboard::PixelThreshold(int32_t rpc_par1, int32_t rpc_par2, int32_t rpc_par3, int32_t rpc_par4, int32_t rpc_par5, int32_t rpc_par6, int32_t rpc_par7, bool rpc_par8, bool rpc_par9)
{ //RPC_PROFILING
	int32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(97);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT32(rpc_par1);
	msg.Put_INT32(rpc_par2);
	msg.Put_INT32(rpc_par3);
	msg.Put_INT32(rpc_par4);
	msg.Put_INT32(rpc_par5);
	msg.Put_INT32(rpc_par6);
	msg.Put_INT32(rpc_par7);
	msg.Put_BOOL(rpc_par8);
	msg.Put_BOOL(rpc_par9);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_INT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(97); throw; };
	return rpc_par0;
}

bool CTestboard::test_pixel_address(int32_t rpc_par1, int32_t rpc_par2)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(98);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT32(rpc_par1);
	msg.Put_INT32(rpc_par2);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(98); throw; };
	return rpc_par0;
}

int8_t CTestboard::CalibratePixel(int16_t rpc_par1, int16_t rpc_par2, int16_t rpc_par3, int16_t &rpc_par4, int32_t &rpc_par5)
{ //RPC_PROFILING
	int8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(99);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT16(rpc_par1);
	msg.Put_INT16(rpc_par2);
	msg.Put_INT16(rpc_par3);
	msg.Put_INT16(rpc_par4);
	msg.Put_INT32(rpc_par5);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,7);
	rpc_par0 = msg.Get_INT8();
	rpc_par4 = msg.Get_INT16();
	rpc_par5 = msg.Get_INT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(99); throw; };
	return rpc_par0;
}

int8_t CTestboard::CalibrateDacScan(int16_t rpc_par1, int16_t rpc_par2, int16_t rpc_par3, int16_t rpc_par4, int16_t rpc_par5, int16_t rpc_par6, vectorR<int16_t> &rpc_par7, vectorR<int32_t> &rpc_par8)
{ //RPC_PROFILING
	int8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(100);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT16(rpc_par1);
	msg.Put_INT16(rpc_par2);
	msg.Put_INT16(rpc_par3);
	msg.Put_INT16(rpc_par4);
	msg.Put_INT16(rpc_par5);
	msg.Put_INT16(rpc_par6);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_INT8();
	rpc_Receive(*rpc_io, rpc_par7);
	rpc_Receive(*rpc_io, rpc_par8);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(100); throw; };
	return rpc_par0;
}

int8_t CTestboard::CalibrateDacDacScan(int16_t rpc_par1, int16_t rpc_par2, int16_t rpc_par3, int16_t rpc_par4, int16_t rpc_par5, int16_t rpc_par6, int16_t rpc_par7, int16_t rpc_par8, int16_t rpc_par9, vectorR<int16_t> &rpc_par10, vectorR<int32_t> &rpc_par11)
{ //RPC_PROFILING
	int8_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(101);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT16(rpc_par1);
	msg.Put_INT16(rpc_par2);
	msg.Put_INT16(rpc_par3);
	msg.Put_INT16(rpc_par4);
	msg.Put_INT16(rpc_par5);
	msg.Put_INT16(rpc_par6);
	msg.Put_INT16(rpc_par7);
	msg.Put_INT16(rpc_par8);
	msg.Put_INT16(rpc_par9);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_INT8();
	rpc_Receive(*rpc_io, rpc_par10);
	rpc_Receive(*rpc_io, rpc_par11);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(101); throw; };
	return rpc_par0;
}

int16_t CTestboard::TrimChip(vector<int16_t> &rpc_par1)
{ //RPC_PROFILING
	int16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(102);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_INT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(102); throw; };
	return rpc_par0;
}

int16_t CTestboard::CalibrateMap(int16_t rpc_par1, vectorR<int16_t> &rpc_par2, vectorR<int32_t> &rpc_par3, vectorR<uint32_t> &rpc_par4)
{ //RPC_PROFILING
	int16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(103);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT16(rpc_par1);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_INT16();
	rpc_Receive(*rpc_io, rpc_par2);
	rpc_Receive(*rpc_io, rpc_par3);
	rpc_Receive(*rpc_io, rpc_par4);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(103); throw; };
	return rpc_par0;
}

int16_t CTestboard::TriggerRow(int16_t rpc_par1, int16_t rpc_par2, vector<int16_t> &rpc_par3, int16_t rpc_par4)
{ //RPC_PROFILING
	int16_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(104);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_INT16(rpc_par1);
	msg.Put_INT16(rpc_par2);
	msg.Put_INT16(rpc_par4);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par3);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,2);
	rpc_par0 = msg.Get_INT16();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(104); throw; };
	return rpc_par0;
}

bool CTestboard::TestColPixel(uint8_t rpc_par1, uint8_t rpc_par2, bool rpc_par3, vectorR<uint8_t> &rpc_par4)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(105);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Put_BOOL(rpc_par3);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	rpc_Receive(*rpc_io, rpc_par4);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(105); throw; };
	return rpc_par0;
}

void CTestboard::Ethernet_Send(string &rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(106);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(106); throw; };
}

uint32_t CTestboard::Ethernet_RecvPackets()
{ //RPC_PROFILING
	uint32_t rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(107);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,4);
	rpc_par0 = msg.Get_UINT32();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(107); throw; };
	return rpc_par0;
}

void CTestboard::LoopInterruptReset()
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(108);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(108); throw; };
}

void CTestboard::SetLoopTriggerDelay(uint16_t rpc_par1)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(109);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par1);
	msg.Send(*rpc_io);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(109); throw; };
}

bool CTestboard::SetI2CAddresses(vector<uint8_t> &rpc_par1)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(110);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(110); throw; };
	return rpc_par0;
}

bool CTestboard::SetTrimValues(uint8_t rpc_par1, vector<int8_t> &rpc_par2)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(111);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par2);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(111); throw; };
	return rpc_par0;
}

bool CTestboard::LoopMultiRocAllPixelsCalibrate(vector<uint8_t> &rpc_par1, uint16_t rpc_par2, uint16_t rpc_par3)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(112);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par2);
	msg.Put_UINT16(rpc_par3);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(112); throw; };
	return rpc_par0;
}

bool CTestboard::LoopMultiRocOnePixelCalibrate(vector<uint8_t> &rpc_par1, uint8_t rpc_par2, uint8_t rpc_par3, uint16_t rpc_par4, uint16_t rpc_par5)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(113);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par2);
	msg.Put_UINT8(rpc_par3);
	msg.Put_UINT16(rpc_par4);
	msg.Put_UINT16(rpc_par5);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(113); throw; };
	return rpc_par0;
}

bool CTestboard::LoopSingleRocAllPixelsCalibrate(uint8_t rpc_par1, uint16_t rpc_par2, uint16_t rpc_par3)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(114);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT16(rpc_par2);
	msg.Put_UINT16(rpc_par3);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(114); throw; };
	return rpc_par0;
} // OK

/*
bool CTestboard::LoopSingleRocOnePixelCalibrate( uint8_t rpc_par1,
						 uint8_t rpc_par2, uint8_t rpc_par3,
						 uint16_t rpc_par4, uint16_t rpc_par5 )
{
  //RPC_PROFILING
  bool rpc_par0;
  try {
    uint16_t rpc_clientCallId = rpc_GetCallId(115);
    RPC_THREAD_LOCK;
    rpcMessage msg;
    msg.Create(rpc_clientCallId);
    msg.Put_UINT8(rpc_par1);
    msg.Put_UINT8(rpc_par2);
    msg.Put_UINT8(rpc_par3);
    msg.Put_UINT16(rpc_par4);
    msg.Put_UINT16(rpc_par5);
    msg.Send(*rpc_io);
    rpc_io->Flush();
    msg.Receive(*rpc_io);
    msg.Check(rpc_clientCallId,1);
    rpc_par0 = msg.Get_BOOL();
    RPC_THREAD_UNLOCK;
  }
  catch (CRpcError &e) { e.SetFunction(115); throw; };
  return rpc_par0;
}
*/

bool CTestboard::LoopMultiRocAllPixelsDacScan( vector<uint8_t> &rpc_par1,
					       uint16_t rpc_par2, uint16_t rpc_par3,
					       uint8_t rpc_par4, uint8_t rpc_par5, uint8_t rpc_par6 )
{
  //RPC_PROFILING
  bool rpc_par0 = 0;
  cout << "CTestboard::LoopMultiRocAllPixelsDacScan..." << endl << flush;

  try {
    uint16_t rpc_clientCallId = rpc_GetCallId(116);
    RPC_THREAD_LOCK;
    rpcMessage msg;
    msg.Create(rpc_clientCallId);
    msg.Put_UINT16(rpc_par2);
    msg.Put_UINT16(rpc_par3);
    msg.Put_UINT8(rpc_par4);
    msg.Put_UINT8(rpc_par5);
    msg.Put_UINT8(rpc_par6);
    msg.Send(*rpc_io);
    rpc_Send(*rpc_io, rpc_par1);
    rpc_io->Flush();
    msg.Receive(*rpc_io);
    msg.Check(rpc_clientCallId,1);
    rpc_par0 = msg.Get_BOOL();
    RPC_THREAD_UNLOCK;
  }
  catch (CRpcError &e) { e.SetFunction(116); throw; };

  return rpc_par0;
}

bool CTestboard::LoopMultiRocOnePixelDacScan(vector<uint8_t> &rpc_par1, uint8_t rpc_par2, uint8_t rpc_par3, uint16_t rpc_par4, uint16_t rpc_par5, uint8_t rpc_par6, uint8_t rpc_par7, uint8_t rpc_par8)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(117);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par2);
	msg.Put_UINT8(rpc_par3);
	msg.Put_UINT16(rpc_par4);
	msg.Put_UINT16(rpc_par5);
	msg.Put_UINT8(rpc_par6);
	msg.Put_UINT8(rpc_par7);
	msg.Put_UINT8(rpc_par8);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(117); throw; };
	return rpc_par0;
}


bool CTestboard::LoopSingleRocAllPixelsDacScan( uint8_t rpc_par1,
						uint16_t rpc_par2, uint16_t rpc_par3,
						uint8_t rpc_par4, uint8_t rpc_par5, uint8_t rpc_par6 )
{
  //RPC_PROFILING;
  bool rpc_par0;
  cout << "CTestboard::LoopSingleRocAllPixelsDacScan..." << endl << flush;
  try {
    uint16_t rpc_clientCallId = rpc_GetCallId(118);
    RPC_THREAD_LOCK;
    rpcMessage msg;
    msg.Create(rpc_clientCallId);
    msg.Put_UINT8(rpc_par1);
    msg.Put_UINT16(rpc_par2);
    msg.Put_UINT16(rpc_par3);
    msg.Put_UINT8(rpc_par4);
    msg.Put_UINT8(rpc_par5);
    msg.Put_UINT8(rpc_par6);
    msg.Send(*rpc_io);
    rpc_io->Flush();
    msg.Receive(*rpc_io);
    msg.Check(rpc_clientCallId,1);
    rpc_par0 = msg.Get_BOOL();
    RPC_THREAD_UNLOCK;
  }
  catch (CRpcError &e) { e.SetFunction(118); throw; };
  return rpc_par0;
}

bool CTestboard::LoopSingleRocOnePixelDacScan(uint8_t rpc_par1, uint8_t rpc_par2, uint8_t rpc_par3, uint16_t rpc_par4, uint16_t rpc_par5, uint8_t rpc_par6, uint8_t rpc_par7, uint8_t rpc_par8)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(119);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Put_UINT8(rpc_par3);
	msg.Put_UINT16(rpc_par4);
	msg.Put_UINT16(rpc_par5);
	msg.Put_UINT8(rpc_par6);
	msg.Put_UINT8(rpc_par7);
	msg.Put_UINT8(rpc_par8);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(119); throw; };
	return rpc_par0;
}

bool CTestboard::LoopMultiRocAllPixelsDacDacScan(vector<uint8_t> &rpc_par1, uint16_t rpc_par2, uint16_t rpc_par3, uint8_t rpc_par4, uint8_t rpc_par5, uint8_t rpc_par6, uint8_t rpc_par7, uint8_t rpc_par8, uint8_t rpc_par9)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(120);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT16(rpc_par2);
	msg.Put_UINT16(rpc_par3);
	msg.Put_UINT8(rpc_par4);
	msg.Put_UINT8(rpc_par5);
	msg.Put_UINT8(rpc_par6);
	msg.Put_UINT8(rpc_par7);
	msg.Put_UINT8(rpc_par8);
	msg.Put_UINT8(rpc_par9);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(120); throw; };
	return rpc_par0;
}

bool CTestboard::LoopMultiRocOnePixelDacDacScan(vector<uint8_t> &rpc_par1, uint8_t rpc_par2, uint8_t rpc_par3, uint16_t rpc_par4, uint16_t rpc_par5, uint8_t rpc_par6, uint8_t rpc_par7, uint8_t rpc_par8, uint8_t rpc_par9, uint8_t rpc_par10, uint8_t rpc_par11)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(121);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par2);
	msg.Put_UINT8(rpc_par3);
	msg.Put_UINT16(rpc_par4);
	msg.Put_UINT16(rpc_par5);
	msg.Put_UINT8(rpc_par6);
	msg.Put_UINT8(rpc_par7);
	msg.Put_UINT8(rpc_par8);
	msg.Put_UINT8(rpc_par9);
	msg.Put_UINT8(rpc_par10);
	msg.Put_UINT8(rpc_par11);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(121); throw; };
	return rpc_par0;
}

bool CTestboard::LoopSingleRocAllPixelsDacDacScan(uint8_t rpc_par1, uint16_t rpc_par2, uint16_t rpc_par3, uint8_t rpc_par4, uint8_t rpc_par5, uint8_t rpc_par6, uint8_t rpc_par7, uint8_t rpc_par8, uint8_t rpc_par9)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(122);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT16(rpc_par2);
	msg.Put_UINT16(rpc_par3);
	msg.Put_UINT8(rpc_par4);
	msg.Put_UINT8(rpc_par5);
	msg.Put_UINT8(rpc_par6);
	msg.Put_UINT8(rpc_par7);
	msg.Put_UINT8(rpc_par8);
	msg.Put_UINT8(rpc_par9);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(122); throw; };
	return rpc_par0;
}

bool CTestboard::LoopSingleRocOnePixelDacDacScan(uint8_t rpc_par1, uint8_t rpc_par2, uint8_t rpc_par3, uint16_t rpc_par4, uint16_t rpc_par5, uint8_t rpc_par6, uint8_t rpc_par7, uint8_t rpc_par8, uint8_t rpc_par9, uint8_t rpc_par10, uint8_t rpc_par11)
{ //RPC_PROFILING
	bool rpc_par0;
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(123);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Put_UINT8(rpc_par1);
	msg.Put_UINT8(rpc_par2);
	msg.Put_UINT8(rpc_par3);
	msg.Put_UINT16(rpc_par4);
	msg.Put_UINT16(rpc_par5);
	msg.Put_UINT8(rpc_par6);
	msg.Put_UINT8(rpc_par7);
	msg.Put_UINT8(rpc_par8);
	msg.Put_UINT8(rpc_par9);
	msg.Put_UINT8(rpc_par10);
	msg.Put_UINT8(rpc_par11);
	msg.Send(*rpc_io);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,1);
	rpc_par0 = msg.Get_BOOL();
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(123); throw; };
	return rpc_par0;
}

void CTestboard::VectorTest(vector<uint16_t> &rpc_par1, vectorR<uint16_t> &rpc_par2)
{ //RPC_PROFILING
	try {
	uint16_t rpc_clientCallId = rpc_GetCallId(124);
	RPC_THREAD_LOCK
	rpcMessage msg;
	msg.Create(rpc_clientCallId);
	msg.Send(*rpc_io);
	rpc_Send(*rpc_io, rpc_par1);
	rpc_io->Flush();
	msg.Receive(*rpc_io);
	msg.Check(rpc_clientCallId,0);
	rpc_Receive(*rpc_io, rpc_par2);
	RPC_THREAD_UNLOCK
	} catch (CRpcError &e) { e.SetFunction(124); throw; };
}

