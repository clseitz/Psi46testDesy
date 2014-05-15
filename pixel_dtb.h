/*---------------------------------------------------------------------
 *
 *  filename:    pixel_dtb.h
 *
 *  description: PSI46 testboard API for DTB
 *	author:      Beat Meier
 *	date:        15.7.2013
 *	rev:          2. 5.2014 for FW 2.14 by Daniel Pitzl
 *                   (compare to pxar/core/rpc/rpc_calls.h)
 *---------------------------------------------------------------------
 */

#pragma once

#include "profiler.h"

#include "rpc.h"
#include "config.h"

#ifdef _WIN32
#include "pipe.h"
#endif

#include "usb.h"

// size of ROC pixel array
#define ROC_NUMROWS  80  // # rows
#define ROC_NUMCOLS  52  // # columns
#define ROC_NUMDCOLS 26  // # double columns (= columns/2)

#define PIXMASK  0x80

// DAC addresses for roc_SetDAC

#define	Vdig         1
#define Vana         2
#define	Vsh          3
#define	Vcomp        4


#define	VwllPr       7

#define	VwllSh       9
#define	VhldDel     10
#define	Vtrim       11
#define	VthrComp    12
#define	VIBias_Bus  13
#define	Vbias_sf    14
#define	VoffsetOp   15

#define	VoffsetRO   17
#define	VIon        18
#define	Vcomp_ADC   19
#define	VIref_ADC   20

#define	VIColOr     22


#define	Vcal        25
#define	CalDel      26

#define VD          31  // DP: supply voltage
#define VA          32

#define	CtrlReg    253
#define	WBC        254
#define	RBreg      255 // DP: read back reg


#define print_missing() fprintf(stderr, "WARNING: Missing implementation of '%s' defined in %s:%d \n" , __PRETTY_FUNCTION__, __FILE__, __LINE__);


class CTestboard
{
  RPC_DEFS;
  RPC_THREAD;

#ifdef _WIN32
  CPipeClient pipe;
#endif
  CUSB usb;

 public:
  CRpcIo& GetIo() { return *rpc_io; }

  CTestboard() { RPC_INIT rpc_io = &usb;
    // Set defaults for deser160 variables:
    delayAdjust = 4;
    deserAdjust = 4;
    invertAddress = 0;
  }
  ~CTestboard() { RPC_EXIT }

  //FIXME not nice but better than global variables:
  int delayAdjust;
  int deserAdjust;
  bool invertAddress;

  // === RPC ==============================================================

  RPC_EXPORT uint16_t GetRpcVersion();
  RPC_EXPORT int32_t  GetRpcCallId(string &callName);

  RPC_EXPORT void GetRpcTimestamp(stringR &ts);

  RPC_EXPORT int32_t GetRpcCallCount();
  RPC_EXPORT bool    GetRpcCallName(int32_t id, stringR &callName);

  RPC_EXPORT uint32_t GetRpcCallHash();

  // === DTB connection ====================================================

  bool EnumFirst(unsigned int &nDevices) { return usb.EnumFirst(nDevices); };
  bool EnumNext(string &name);
  bool Enum(unsigned int pos, string &name);

  bool FindDTB(string &usbId);
  bool Open(string &name, bool init=true); // opens a connection
  void Close();				// closes the connection to the testboard

#ifdef _WIN32
  bool OpenPipe(const char *name) { return pipe.Open(name); }
  void ClosePipe() { pipe.Close(); }
#endif

  //DP void SetTimeout(unsigned int timeout) { usb.SetTimeout(timeout); }

  bool IsConnected() { return usb.Connected(); }
  const char * ConnectionError()
  { return usb.GetErrorMsg(usb.GetLastError()); }

  void Flush() { rpc_io->Flush(); }
  void Clear() { rpc_io->Clear(); }

  // === DTB identification ================================================

  RPC_EXPORT void GetInfo(stringR &info);
  RPC_EXPORT uint16_t GetBoardId();
  RPC_EXPORT void GetHWVersion(stringR &version);
  RPC_EXPORT uint16_t GetFWVersion();
  RPC_EXPORT uint16_t GetSWVersion();
  RPC_EXPORT uint16_t GetUser1Version();

  // === DTB service ======================================================

  // --- upgrade
  RPC_EXPORT uint16_t UpgradeGetVersion();
  RPC_EXPORT uint8_t  UpgradeStart(uint16_t version);
  RPC_EXPORT uint8_t  UpgradeData(string &record);
  RPC_EXPORT uint8_t  UpgradeError();
  RPC_EXPORT void     UpgradeErrorMsg(stringR &msg);
  RPC_EXPORT void     UpgradeExec(uint16_t recordCount);

  // === DTB functions ====================================================

  RPC_EXPORT void Init();

  RPC_EXPORT void Welcome();
  RPC_EXPORT void SetLed(uint8_t x);

  // --- Clock, Timing ----------------------------------------------------
  RPC_EXPORT void cDelay(uint16_t clocks);
  RPC_EXPORT void uDelay(uint16_t us);
  void mDelay(uint16_t ms);

  // select ROC/Module clock source
#define CLK_SRC_INT  0
#define CLK_SRC_EXT  1
  RPC_EXPORT void SetClockSource(uint8_t source);

  // --- check if external clock is present
  RPC_EXPORT bool IsClockPresent();

  // --- set clock clock frequency (clock divider)
#define MHZ_1_25   5
#define MHZ_2_5    4
#define MHZ_5      3
#define MHZ_10     2
#define MHZ_20     1
#define MHZ_40     0  // default
  RPC_EXPORT void SetClock(uint8_t MHz);

  // --- set clock stretch
  // width = 0 -> disable stretch
#define STRETCH_AFTER_TRG  0
#define STRETCH_AFTER_CAL  1
#define STRETCH_AFTER_RES  2
#define STRETCH_AFTER_SYNC 3
  RPC_EXPORT void SetClockStretch(uint8_t src,
				  uint16_t delay, uint16_t width);

  // --- Signal Delay -----------------------------------------------------
#define SIG_CLK 0
#define SIG_CTR 1
#define SIG_SDA 2
#define SIG_TIN 3

#define SIG_MODE_NORMAL  0
#define SIG_MODE_LO      1
#define SIG_MODE_HI      2

  RPC_EXPORT void Sig_SetMode(uint8_t signal, uint8_t mode);
  RPC_EXPORT void Sig_SetPRBS(uint8_t signal, uint8_t speed);
  RPC_EXPORT void Sig_SetDelay(uint8_t signal, uint16_t delay, int8_t duty = 0);
  RPC_EXPORT void Sig_SetLevel(uint8_t signal, uint8_t level);
  RPC_EXPORT void Sig_SetOffset(uint8_t offset);
  RPC_EXPORT void Sig_SetLVDS();
  RPC_EXPORT void Sig_SetLCDS();

  // --- digital signal probe ---------------------------------------------
#define PROBE_OFF          0
#define PROBE_CLK          1
#define PROBE_SDA          2
#define PROBE_SDA_SEND     3
#define PROBE_PG_TOK       4
#define PROBE_PG_TRG       5
#define PROBE_PG_CAL       6
#define PROBE_PG_RES_ROC   7
#define PROBE_PG_RES_TBM   8
#define PROBE_PG_SYNC      9
#define PROBE_CTR         10
#define PROBE_TIN         11
#define PROBE_TOUT        12
#define PROBE_CLK_PRESEN  13
#define PROBE_CLK_GOOD    14
#define PROBE_DAQ0_WR     15
#define PROBE_CRC         16
#define PROBE_ADC_RUNNING 19
#define PROBE_ADC_RUN     20
#define PROBE_ADC_PGATE   21
#define PROBE_ADC_START   22
#define PROBE_ADC_SGATE   23
#define PROBE_ADC_S       24

  RPC_EXPORT void SignalProbeD1(uint8_t signal);
  RPC_EXPORT void SignalProbeD2(uint8_t signal);

  // --- analog signal probe ----------------------------------------------

#define PROBEA_TIN     0
#define PROBEA_SDATA1  1  // analog ROC
#define PROBEA_SDATA2  2
#define PROBEA_CTR     3
#define PROBEA_CLK     4
#define PROBEA_SDA     5
#define PROBEA_TOUT    6
#define PROBEA_OFF     7

#define GAIN_1   0
#define GAIN_2   1
#define GAIN_3   2
#define GAIN_4   3

  RPC_EXPORT void SignalProbeA1(uint8_t signal);
  RPC_EXPORT void SignalProbeA2(uint8_t signal);
  RPC_EXPORT void SignalProbeADC(uint8_t signal, uint8_t gain = 0); // SDATA1

  // --- ROC/Module power VD/VA -------------------------------------------

  RPC_EXPORT void Pon();	// switch ROC power on
  RPC_EXPORT void Poff();	// switch ROC power off

  RPC_EXPORT void _SetVD(uint16_t mV);
  RPC_EXPORT void _SetVA(uint16_t mV);
  RPC_EXPORT void _SetID(uint16_t uA100);
  RPC_EXPORT void _SetIA(uint16_t uA100);

  RPC_EXPORT uint16_t _GetVD();
  RPC_EXPORT uint16_t _GetVA();
  RPC_EXPORT uint16_t _GetID();
  RPC_EXPORT uint16_t _GetIA();

  void SetVA(double V) { _SetVA(uint16_t(V*1000)); }  // set VA voltage
  void SetVD(double V) { _SetVD(uint16_t(V*1000)); }  // set VD voltage
  void SetIA(double A) { _SetIA(uint16_t(A*10000)); }  // set VA current limit
  void SetID(double A) { _SetID(uint16_t(A*10000)); }  // set VD current limit

  double GetVA() { return _GetVA()/1000.0; }   // get VA voltage in V
  double GetVD() { return _GetVD()/1000.0; }	 // get VD voltage in V
  double GetIA() { return _GetIA()/10000.0; }  // get VA current in A
  double GetID() { return _GetID()/10000.0; }  // get VD current in A

  RPC_EXPORT void HVon();
  RPC_EXPORT void HVoff();

  RPC_EXPORT void ResetOn();
  RPC_EXPORT void ResetOff();

  RPC_EXPORT uint8_t GetStatus();
  RPC_EXPORT void SetRocAddress(uint8_t addr);

  RPC_EXPORT bool GetPixelAddressInverted();

  RPC_EXPORT void SetPixelAddressInverted(bool status);

  // --- pulse pattern generator ------------------------------------------

#define PG_TOK   0x0100
#define PG_TRG   0x0200
#define PG_CAL   0x0400
#define PG_RESR  0x0800
#define PG_REST  0x1000
#define PG_SYNC  0x2000

  RPC_EXPORT void Pg_SetCmd(uint16_t addr, uint16_t cmd);
  RPC_EXPORT void Pg_SetCmdAll(vector<uint16_t> &cmd);
  RPC_EXPORT void Pg_Stop();
  RPC_EXPORT void Pg_Single();
  RPC_EXPORT void Pg_Trigger();
  RPC_EXPORT void Pg_Loop(uint16_t period);

  // --- data aquisition --------------------------------------------------

  RPC_EXPORT uint32_t Daq_Open(uint32_t buffersize = 10000000, uint8_t channel = 0);
  RPC_EXPORT void Daq_Close( uint8_t channel = 0); // tbm A and B
  RPC_EXPORT void Daq_Start( uint8_t channel = 0); // tbm A and B
  RPC_EXPORT void Daq_Stop( uint8_t channel = 0);
  RPC_EXPORT uint32_t Daq_GetSize(uint8_t channel = 0);
  RPC_EXPORT uint8_t Daq_FillLevel(uint8_t channel);
  RPC_EXPORT uint8_t Daq_FillLevel();

  RPC_EXPORT uint8_t Daq_Read( HWvectorR<uint16_t> &data,
			       uint32_t blocksize = 65536, // FW 2.0
			       uint8_t channel = 0 ); // tbm A and B

  RPC_EXPORT uint8_t Daq_Read( HWvectorR<uint16_t> &data,
			       uint32_t blocksize, uint32_t &availsize,
			       uint8_t channel = 0 );

  // FW 1.12  if( blocksize > 32768) blocksize = 32768;

  RPC_EXPORT void Daq_Select_ADC( uint16_t blocksize, uint8_t source,
				  uint8_t start, uint8_t stop = 0 ); // FPGA ADC
  // start token in, stop token out

  RPC_EXPORT void Daq_Select_Deser160( uint8_t shift );

  RPC_EXPORT void Daq_Select_Deser400();
  RPC_EXPORT void Daq_Deser400_Reset( uint8_t reset = 3 ); // deser reset bits

  RPC_EXPORT void Daq_Select_Datagenerator(uint16_t startvalue);

  RPC_EXPORT void Daq_DeselectAll();

  // --- ROC/module Communication -----------------------------------------

  // -- set the i2c address for the following commands
  RPC_EXPORT void roc_I2cAddr(uint8_t id);

  // -- sends "ClrCal" command to ROC
  RPC_EXPORT void roc_ClrCal();

  // -- sets a single (DAC) register
  void SetDAC( uint8_t reg, uint16_t value ); // DP
  RPC_EXPORT void roc_SetDAC( uint8_t reg, uint8_t value );

  // -- set pixel bits (count <= 60)
  //    M - - - 8 4 2 1
  RPC_EXPORT void roc_Pix(uint8_t col, uint8_t row, uint8_t value);

  // -- trim a single pixel (count < =60)
  RPC_EXPORT void roc_Pix_Trim(uint8_t col, uint8_t row, uint8_t value);

  // -- mask a single pixel (count <= 60)
  RPC_EXPORT void roc_Pix_Mask(uint8_t col, uint8_t row);

  // -- set calibrate at specific column and row
  RPC_EXPORT void roc_Pix_Cal(uint8_t col, uint8_t row, bool sensor_cal = false);

  // -- enable/disable a double column
  RPC_EXPORT void roc_Col_Enable(uint8_t col, bool on);

  // -- mask all pixels of a column and the coresponding double column
  RPC_EXPORT void roc_Col_Mask(uint8_t col);

  // -- mask all pixels and columns of the chip
  RPC_EXPORT void roc_Chip_Mask();

  // == TBM functions =====================================================

  RPC_EXPORT bool TBM_Present(); 

  RPC_EXPORT void tbm_Enable(bool on);

  RPC_EXPORT void tbm_Addr(uint8_t hub, uint8_t port);

  RPC_EXPORT void mod_Addr(uint8_t hub);

  RPC_EXPORT void tbm_Set(uint8_t reg, uint8_t value);

  RPC_EXPORT bool tbm_Get(uint8_t reg, uint8_t &value);

  RPC_EXPORT bool tbm_GetRaw(uint8_t reg, uint32_t &value);

  RPC_EXPORT int32_t CountReadouts( int32_t nTriggers);
  RPC_EXPORT int32_t CountReadouts( int32_t nTriggers, int32_t chipId);
  RPC_EXPORT int32_t CountReadouts( int32_t nTriggers, int32_t dacReg, int32_t dacValue);
  RPC_EXPORT int32_t PH( int32_t col, int32_t row, int32_t trim, int16_t nTriggers );
  RPC_EXPORT int32_t PixelThreshold( int32_t col, int32_t row, int32_t start, int32_t step, int32_t thrLevel, int32_t nTrig, int32_t dacReg, bool xtalk, bool cals );
  RPC_EXPORT bool test_pixel_address(int32_t col, int32_t row);
  RPC_EXPORT int32_t ChipEfficiency_dtb(int16_t nTriggers, vectorR<uint8_t> &res);

  // ETH functions:

  RPC_EXPORT int8_t CalibratePixel( int16_t nTriggers, int16_t col, int16_t row, int16_t &nReadouts, int32_t &PHsum);

  RPC_EXPORT int8_t CalibrateDacScan(int16_t nTriggers, int16_t col, int16_t row, int16_t dacReg1, int16_t dacLower1, int16_t dacUpper1, vectorR<int16_t> &nReadouts, vectorR<int32_t> &PHsum);

  RPC_EXPORT int8_t CalibrateDacDacScan(int16_t nTriggers, int16_t col, int16_t row, int16_t dacReg1, int16_t dacLower1, int16_t dacUpper1, int16_t dacReg2, int16_t dacLower2, int16_t dacUpper2, vectorR<int16_t> &nReadouts, vectorR<int32_t> &PHsum);
  RPC_EXPORT void ParallelCalibrateDacDacScan(vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint8_t col, uint8_t row, uint8_t dacReg1, uint8_t dacLower1, uint8_t dacUpper1, uint8_t dacReg2, uint8_t dacLower2, uint8_t dacUpper2, uint16_t flags);

  RPC_EXPORT int16_t CalibrateMap( int16_t nTriggers, vectorR<int16_t> &nReadouts, vectorR<int32_t> &PHsum, vectorR<uint32_t> &addres ); // empty
  RPC_EXPORT int16_t CalibrateModule( vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags );

  RPC_EXPORT int16_t TrimChip( vector<int16_t> &trim );
  RPC_EXPORT int16_t TriggerRow( int16_t nTriggers, int16_t col, vector<int16_t> &rocs, int16_t delay=4);

  // --- Wafer test functions

  RPC_EXPORT bool testColPixel( uint8_t col, uint8_t trimbit, vectorR<uint8_t> &res);
  //old RPC_EXPORT bool TestColPixel( uint8_t col, uint8_t trimbit, vectorR<uint8_t> &res);
  RPC_EXPORT bool TestColPixel( uint8_t col, uint8_t trimbit, bool sensor_cal,
				vectorR<uint8_t> &res);

  // Ethernet test functions

  RPC_EXPORT void Ethernet_Send(string &message);
  RPC_EXPORT uint32_t Ethernet_RecvPackets();

  // == Trigger Loop functions for Host-side DAQ ROC/Module testing ==============

  RPC_EXPORT bool SetI2CAddresses(vector<uint8_t> &roc_i2c);
  RPC_EXPORT bool SetTrimValues( uint8_t roc_i2c, vector<uint8_t> &trimvalues ); // uint8 in 2.15

  RPC_EXPORT void SetLoopTriggerDelay( uint16_t delay ); // [BC] (since 2.14)
  RPC_EXPORT void LoopInterruptReset();

  // Maps:

  RPC_EXPORT bool LoopMultiRocAllPixelsCalibrate( vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags);
  RPC_EXPORT bool LoopMultiRocOnePixelCalibrate( vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags);
  RPC_EXPORT bool LoopSingleRocAllPixelsCalibrate( uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags);
  RPC_EXPORT bool LoopSingleRocOnePixelCalibrate( uint8_t roc_i2c, uint8_t column, uint8_t row,
						  uint16_t nTriggers, uint16_t flags);
	  
  // 1D DacScans:

  RPC_EXPORT bool LoopMultiRocAllPixelsDacScan( vector<uint8_t> &roc_i2c,
						uint16_t nTriggers, uint16_t flags,
						uint8_t dac1register, uint8_t dac1low, uint8_t dac1high );
  RPC_EXPORT bool LoopMultiRocOnePixelDacScan( vector<uint8_t> &roc_i2c,
					       uint8_t column, uint8_t row,
					       uint16_t nTriggers, uint16_t flags,
					       uint8_t dac1register, uint8_t dac1low, uint8_t dac1high );
  RPC_EXPORT bool LoopSingleRocAllPixelsDacScan( uint8_t roc_i2c,
						 uint16_t nTriggers, uint16_t flags,
						 uint8_t dac1register, uint8_t dac1low, uint8_t dac1high );
  RPC_EXPORT bool LoopSingleRocOnePixelDacScan( uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high);

  // 2D DacDacScans:

  RPC_EXPORT bool LoopMultiRocAllPixelsDacDacScan(vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
  RPC_EXPORT bool LoopMultiRocOnePixelDacDacScan(vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
  RPC_EXPORT bool LoopSingleRocAllPixelsDacDacScan(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
  RPC_EXPORT bool LoopSingleRocOnePixelDacDacScan( uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);

  RPC_EXPORT void VectorTest( vector<uint16_t> &in, vectorR<uint16_t> &out );

  /*
#define PROBE_ADC_GATE 12
#define TRIGGER_OFF       0
#define TRIGGER_FIXED     1
#define TRIGGER_ROC       2
#define TRIGGER_MODULE1   4
#define TRIGGER_MODULE2   8
#define RES  0x0800
#define CAL  0x0400
#define TRG  0x0200
#define TOK  0x0100
  */

  //TODO: Everything below needs to be implemented !!!

  bool GetVersion(char *s, uint32_t n){ print_missing(); return false;}

  void SetEmptyReadoutLength(int32_t emptyReadoutLength){ print_missing(); return; }

  void Single(unsigned char mask){ print_missing(); return; }

  bool SingleWait(unsigned char mask, uint16_t timeout){print_missing(); return false;}

  // -- enables the internal event generator
  //    mask: same as tb_Single
  void Intern(unsigned char mask){ Single(mask); }

  // -- enables the external event input
  //    mask: same as tb_Single
  void Extern(unsigned char mask){ Single(mask);}

  // -- gets the readout counter
  unsigned char GetRoCnt(){ print_missing(); return 1; }  

  bool SendRoCnt(){ print_missing(); return false;}
  unsigned char RecvRoCnt(){ print_missing(); return 1; }

  uint16_t GetRoCntEx(){ print_missing(); return 1; }
  bool SendRoCntEx(){ print_missing(); return false; }
  uint16_t RecvRoCntEx(){ print_missing(); return 1; }

  void SetTriggerMode(uint16_t mode){ print_missing(); return;}
  void DataCtrl(char channel, bool clear, bool trigger, bool cont){ print_missing(); return;}
  void DataEnable(bool on){print_missing(); return ;}
  uint16_t DataState(){ print_missing(); return 1;}
  void DataTriggerLevel(char channel, int16_t level){ print_missing(); return;}
  void DataBlockSize(uint16_t size){print_missing(); return ;}
  bool DataRead(char channel, int16_t buffer[], uint16_t buffersize,
		uint16_t &wordsread){ print_missing(); return false;}
  bool DataReadRaw(char channel, int16_t buffer[], uint16_t buffersize,
		   uint16_t &wordsread){ print_missing(); return false; }
  uint16_t GetModRoCnt(uint16_t index){ print_missing(); return 1;}
  void GetModRoCntAll(uint16_t *counts){ print_missing(); return; }

  uint32_t Daq_Init(uint32_t size){ print_missing(); return 1; }
  void Daq_Enable(){ print_missing(); return; }
  void Daq_Disable(){ print_missing(); return; }
  bool Daq_Ready(){ print_missing(); return true; }
  uint16_t Daq_GetPointer(){ print_missing(); return 1; }
  void Daq_Done(){ print_missing(); return; }

  void ProbeSelect(unsigned char port, unsigned char signal){ print_missing(); return;}
  void SetTriggerMask(unsigned char mask){ print_missing(); return;}

  void TBMEmulatorOn(){ print_missing(); return; }
  void TBMEmulatorOff(){ print_missing(); return; }
  void TbmWrite(int32_t hubAddr, int32_t addr, int32_t value){ print_missing(); return; }
  void Tbm1Write(int32_t hubAddr, int32_t addr, int32_t value){ print_missing(); return; }
  void Tbm2Write(int32_t hubAddr, int32_t addr, int32_t value){ print_missing(); return; }
  void SetDelay(unsigned char signal, uint16_t ns){ print_missing(); return; }
  void AdjustDelay(uint16_t k) { SetDelay(255, k); }

  // === module test functions ======================================
    
  // --- implemented funtions: ---
  void InitDAC();
  void Init_Reset();
  void prep_dig_test();
  void SetMHz(int MHz);
  void I2cAddr(unsigned char id){ roc_I2cAddr(id); }
  //analog void Set(unsigned char reg, unsigned char value){ roc_SetDAC(reg, value); }
  //analog void SetReg(unsigned char addr, uint16_t value){ roc_SetDAC(addr, value); }
  void ArmPixel(int col, int row);
  void ArmPixel(int col, int row, int trim);    
  void DisarmPixel(int col, int row);
  void DisableAllPixels();
  void EnableColumn(int col);
  void EnableAllPixels(int32_t trim[]);
  void SetChip(int iChip);    
  int32_t MaskTest(int16_t nTriggers, int16_t res[]);
  int32_t ChipEfficiency(int16_t nTriggers, int32_t trim[], double res[]); 
  void DacDac( int32_t dac1, int32_t dacRange1, int32_t dac2, int32_t dacRange2, int32_t nTrig, int32_t result[] );
  void AddressLevels(int32_t position, int32_t result[]){ print_missing(); return;}
  int32_t ChipThreshold(int32_t start, int32_t step, int32_t thrLevel, int32_t nTrig, int32_t dacReg, int32_t xtalk, int32_t cals, int32_t trim[], int32_t res[]);
  void ChipThresholdIntern(int32_t start[], int32_t step, int32_t thrLevel, int32_t nTrig, int32_t dacReg, int32_t xtalk, int32_t cals, int32_t trim[], int32_t res[]);
  int32_t SCurve(int32_t nTrig, int32_t dacReg, int32_t threshold, int32_t res[]);
  int32_t SCurve(int32_t nTrig, int32_t dacReg, int32_t thr[], int32_t chipId[], int32_t sCurve[]);
  int32_t SCurveColumn(int32_t column, int32_t nTrig, int32_t dacReg, int32_t thr[], int32_t trims[], int32_t chipId[], int32_t res[]);
  void SetNRocs(int32_t value);
  void SetHubID(int32_t value);

  bool GetPixel(int32_t x){ print_missing(); return false; }
  int32_t FindLevel(){ print_missing(); return 1; }
  unsigned char test_PUC(unsigned char col, unsigned char row, unsigned char trim){ print_missing(); return 1; }
  //void testColPixel(int32_t col, int32_t trimbit, unsigned char *res){ print_missing(); return; }
  bool GetLastDac(unsigned char count, int32_t &ldac){ print_missing(); return false; }
  bool ScanDac(unsigned char dac, unsigned char count,
	       unsigned char min, unsigned char max, int16_t *ldac){ print_missing(); return false; }

  int32_t AoutLevel(int16_t position, int16_t nTriggers){ print_missing(); return 1; }
  int32_t AoutLevelChip(int16_t position, int16_t nTriggers, int32_t trims[],  int32_t res[]){ print_missing(); return 1; }
  int32_t AoutLevelPartOfChip(int16_t position, int16_t nTriggers, int32_t trims[], int32_t res[], bool pxlFlags[]){ print_missing(); return 1; }
  void DoubleColumnADCData(int32_t column, int16_t data[], int32_t readoutStop[]){ print_missing(); return; }
  void ADCRead( int16_t buffer[], uint16_t &wordsread, int16_t nTrig){ print_missing(); return; }
  void PHDac(int32_t dac, int32_t dacRange, int32_t nTrig, int32_t position, int16_t result[]){ print_missing(); return; }
  void TBMAddressLevels(int32_t result[]){ print_missing(); return; }
  void TrimAboveNoise(int16_t nTrigs, int16_t thr, int16_t mode, int16_t result[]){ print_missing(); return; }

  void ReadData(int32_t position, int32_t size, int32_t result[]){ print_missing(); return; }
  void ReadFPGAData(int32_t size, int32_t result[]){ print_missing(); return; }

  void SetEmptyReadoutLengthADC(int32_t emptyReadoutLengthADC){ print_missing(); return; }
  void SetTbmChannel(int32_t tbmChannel){ print_missing(); return; }
  void SetEnableAll(int value){ print_missing(); return; }
  void SetDTL(int32_t value){ print_missing(); return; }
  void SetAoutChipPosition(int32_t value){ print_missing(); return; }
  void MemRead(uint32_t addr, uint16_t size,
	       unsigned char * s){ print_missing(); return; }

  // == PSI46 testboard methods ===========================================

  void ForceSignal(unsigned char pattern){ print_missing(); return; }

  bool ShowUSB() { print_missing(); return false; }; 

  bool Open(char name[], bool init = true){ print_missing(); return true;}

  // Compat. fix for test written for ATB:
  unsigned short GetReg41(){return 1;}

  // =======================================================================

  int32_t demo(int16_t x){ print_missing(); return 1; }

  void GetColPulseHeight(unsigned char col, unsigned char count,
			 int16_t data[]){ print_missing(); return; }

  void Scan1D(unsigned char vx,
	      unsigned char xmin, unsigned char xmax,	char xstep,
	      unsigned char rep, uint32_t usDelay, unsigned char res[]){ print_missing(); return; }

  void BumpTestColPixel(unsigned char col, unsigned char res[]);
  void BumpTestColRef(unsigned char col, unsigned char res[]);
  void DacDac(int16_t dac1, int16_t dacRange1, int16_t dac2, int16_t dacRange2,
	      int16_t nTrig, int16_t res[], int16_t rocpos);

  // ===================================================================

  void ScanAdac(uint16_t chip, unsigned char dac,
		unsigned char min, unsigned char max, char step,
		unsigned char rep, uint32_t usDelay, unsigned char res[]){ print_missing(); return; }
  void CdVc(uint16_t chip, unsigned char wbcmin, unsigned char wbcmax, unsigned char vcalstep,
	    unsigned char cdinit, uint16_t &lres, uint16_t res[]){ print_missing(); return; }

  // === xray test =====================================================
  char CountAllReadouts(int32_t nTrig, int32_t counts[], int32_t amplitudes[]){ print_missing(); return 1; }


 private:
  int hubId;
  int nRocs;

};
