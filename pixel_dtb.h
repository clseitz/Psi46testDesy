
// header for rpc_calls.h
// Beat Meier, PSI, 15.7.2013
// Daniel Pitzl, DESY, 21.7.2014 for FW 3.1

#pragma once

#include "version.h"
#include "profiler.h"
#include "rpc.h"

#ifdef _WIN32
#include "pipe.h"
#endif

#include "usb.h"

// size of ROC pixel array
#define ROC_NUMROWS  80 // # rows
#define ROC_NUMCOLS  52 // # columns
#define ROC_NUMDCOLS 26 // # double columns (= columns/2)

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

#define VD          31 // DP: supply voltage
#define VA          32

#define	CtrlReg    253
#define	WBC        254
#define	RBreg      255 // DP: read back reg


#define print_missing() fprintf( stderr, "WARNING: Missing implementation of '%s' defined in %s:%d \n" , __PRETTY_FUNCTION__, __FILE__, __LINE__ );


class CTestboard
{
  RPC_DEFS;
  RPC_THREAD;

#ifdef _WIN32
  CPipeClient pipe;
#endif
  CUSB usb;

 public:
  CRpcIo & GetIo(  )
    {
      return *rpc_io;
    }

  CTestboard(  )
    {
      RPC_INIT rpc_io = &usb;
      // Set defaults for deser160 variables:
      //delayAdjust = 4;
      //deserAdjust = 4;
      invertAddress = 0;
      cout << "instantiated a CTestboard" << endl;
    }

  ~CTestboard(  ) {
    RPC_EXIT}

  //FIXME not nice but better than global variables:
  //int delayAdjust;
  //int deserAdjust;
  bool invertAddress;

  // Host = PC

  int32_t GetHostRpcCallCount(  )
  {
    return rpc_cmdListSize;
  }

  bool GetHostRpcCallName( int32_t id, stringR & callName )
  {
    callName = rpc_cmdName[id];
    return true;
  }

  std::vector < std::string > GetHostRpcCallNames(  ) {

    std::vector < std::string > rpc_cmdList;
    for( size_t i = 0; i < rpc_cmdListSize; i++ )
      rpc_cmdList.push_back( rpc_cmdName[i] );

    return rpc_cmdList;
  }

  // === RPC ==============================================================

  RPC_EXPORT uint16_t GetRpcVersion(  );
  RPC_EXPORT int32_t GetRpcCallId( string & callName );

  RPC_EXPORT void GetRpcTimestamp( stringR & ts );

  RPC_EXPORT int32_t GetRpcCallCount(  );
  RPC_EXPORT bool GetRpcCallName( int32_t id, stringR & callName );

  RPC_EXPORT uint32_t GetRpcCallHash(  );

  bool RpcLink(  )
  { // from pxar/core/rpc/rpc_calls.h

    bool error = false;
    for( unsigned short i = 2; i < rpc_cmdListSize; i++ ) {
      try {
        rpc_GetCallId( i );
      }
      catch( CRpcError & e ) {
        e.SetFunction( 0 );
        if( !error )
          cout << "Missing DTB functions: " << endl; // first line
        std::string fname( rpc_cmdName[i] );
        std::string fname_pretty;
        rpc_TranslateCallName( fname, fname_pretty );
        cout << fname_pretty.c_str(  ) << endl;
        error = true;
      }
    }
    return !error;
  }

  // === DTB connection ====================================================

  bool EnumFirst( unsigned int &nDevices )
  {
    return usb.EnumFirst( nDevices );
  };
  bool EnumNext( string & name );
  bool Enum( unsigned int pos, string & name );

  bool FindDTB( string & usbId );
  bool Open( string & name, bool init = true ); // opens a connection
  void Close(  );               // closes the connection to the testboard

#ifdef _WIN32
  bool OpenPipe( const char *name )
  {
    return pipe.Open( name );
  }
  void ClosePipe(  )
  {
    pipe.Close(  );
  }
#endif

  void SetTimeout( int ms )
  {
    usb.SetTimeout( ms );
  }

  bool IsConnected(  )
  {
    return usb.Connected(  );
  }
  const char *ConnectionError(  )
  {
    return usb.GetErrorMsg( usb.GetLastError(  ) );
  }

  void Flush(  )
  {
    rpc_io->Flush(  );
  }
  void Clear(  )
  {
    rpc_io->Clear(  );
  }

  // === DTB identification ================================================

  RPC_EXPORT void GetInfo( stringR & info );
  RPC_EXPORT uint16_t GetBoardId(  );
  RPC_EXPORT void GetHWVersion( stringR & version );
  RPC_EXPORT uint16_t GetFWVersion(  );
  RPC_EXPORT uint16_t GetSWVersion(  );
  //RPC_EXPORT uint16_t GetUser1Version(  );

  // === DTB service ======================================================

  // --- upgrade
  RPC_EXPORT uint16_t UpgradeGetVersion(  );
  RPC_EXPORT uint8_t UpgradeStart( uint16_t version );
  RPC_EXPORT uint8_t UpgradeData( string & record );
  RPC_EXPORT uint8_t UpgradeError(  );
  RPC_EXPORT void UpgradeErrorMsg( stringR & msg );
  RPC_EXPORT void UpgradeExec( uint16_t recordCount );

  // === DTB functions ====================================================

  RPC_EXPORT void Init(  );

  RPC_EXPORT void Welcome(  );
  RPC_EXPORT void SetLed( uint8_t x );

  // --- Clock, Timing ----------------------------------------------------
  RPC_EXPORT void cDelay( uint16_t clocks );
  RPC_EXPORT void uDelay( uint16_t us );
  void mDelay( uint16_t ms );

  // select ROC/Module clock source
#define CLK_SRC_INT  0
#define CLK_SRC_EXT  1
  RPC_EXPORT void SetClockSource( uint8_t source );

  // --- check if external clock is present
  RPC_EXPORT bool IsClockPresent(  );

  // --- set clock clock frequency (clock divider) 2^n
#define MHZ_1_25   5
#define MHZ_2_5    4
#define MHZ_5      3
#define MHZ_10     2
#define MHZ_20     1
#define MHZ_40     0 // default
  RPC_EXPORT void SetClock( uint8_t MHz );

  // --- set clock stretch
  // width = 0 -> disable stretch
#define STRETCH_AFTER_TRG  0
#define STRETCH_AFTER_CAL  1
#define STRETCH_AFTER_RES  2
#define STRETCH_AFTER_SYNC 3
  RPC_EXPORT void SetClockStretch( uint8_t src,
                                   uint16_t delay, uint16_t width );

  // --- Signal Delay -----------------------------------------------------
#define SIG_CLK 0
#define SIG_CTR 1
#define SIG_SDA 2
#define SIG_TIN 3

#define SIG_MODE_NORMAL  0
#define SIG_MODE_LO      1
#define SIG_MODE_HI      2

  RPC_EXPORT void Sig_SetMode( uint8_t signal, uint8_t mode );
  RPC_EXPORT void Sig_SetPRBS( uint8_t signal, uint8_t speed );
  RPC_EXPORT void Sig_SetDelay( uint8_t signal, uint16_t delay,
				int8_t duty = 0 );
  RPC_EXPORT void Sig_SetLevel( uint8_t signal, uint8_t level );
  RPC_EXPORT void Sig_SetOffset( uint8_t offset );
  RPC_EXPORT void Sig_SetLVDS(  );
  RPC_EXPORT void Sig_SetLCDS(  );
  RPC_EXPORT void Sig_SetRdaToutDelay( uint8_t delay ); // 3.0

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

  RPC_EXPORT void SignalProbeD1( uint8_t signal );
  RPC_EXPORT void SignalProbeD2( uint8_t signal );

  // --- analog signal probe ----------------------------------------------

#define PROBEA_TIN     0
#define PROBEA_SDATA1  1 // analog ROC
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

  RPC_EXPORT void SignalProbeA1( uint8_t signal );
  RPC_EXPORT void SignalProbeA2( uint8_t signal );
  RPC_EXPORT void SignalProbeADC( uint8_t signal, uint8_t gain = 0 ); // SDATA1

  // --- ROC/Module power VD/VA -------------------------------------------

  RPC_EXPORT void Pon(  );      // switch ROC power on
  RPC_EXPORT void Poff(  );     // switch ROC power off

  RPC_EXPORT void _SetVD( uint16_t mV );
  RPC_EXPORT void _SetVA( uint16_t mV );
  RPC_EXPORT void _SetID( uint16_t uA100 );
  RPC_EXPORT void _SetIA( uint16_t uA100 );

  RPC_EXPORT uint16_t _GetVD(  );
  RPC_EXPORT uint16_t _GetVA(  );
  RPC_EXPORT uint16_t _GetID(  );
  RPC_EXPORT uint16_t _GetIA(  );

  void SetVA( double V )
  {
    _SetVA( uint16_t( V * 1000 ) );
  } // set VA voltage
  void SetVD( double V )
  {
    _SetVD( uint16_t( V * 1000 ) );
  } // set VD voltage

  void SetIA( double A )
  {
    _SetIA( uint16_t( A * 10000 ) );
  } // set VA current limit
  void SetID( double A )
  {
    _SetID( uint16_t( A * 10000 ) );
  } // set VD current limit

  double GetVA(  )
  {
    return _GetVA(  ) / 1000.0;
  } // get VA voltage in V
  double GetVD(  )
  {
    return _GetVD(  ) / 1000.0;
  } // get VD voltage in V

  double GetIA(  )
  {
    return _GetIA(  ) / 10000.0;
  } // get VA current in A
  double GetID(  )
  {
    return _GetID(  ) / 10000.0;
  } // get VD current in A

  RPC_EXPORT void HVon(  );
  RPC_EXPORT void HVoff(  );

  RPC_EXPORT void ResetOn(  );
  RPC_EXPORT void ResetOff(  );

  RPC_EXPORT uint8_t GetStatus(  );
  RPC_EXPORT void SetRocAddress( uint8_t addr );

  RPC_EXPORT bool GetPixelAddressInverted(  );

  RPC_EXPORT void SetPixelAddressInverted( bool status );

  // --- pulse pattern generator ------------------------------------------

#define PG_TOK   0x0100
#define PG_TRG   0x0200
#define PG_CAL   0x0400
#define PG_RESR  0x0800
#define PG_REST  0x1000
#define PG_SYNC  0x2000

  RPC_EXPORT void Pg_SetCmd( uint16_t addr, uint16_t cmd );
  RPC_EXPORT void Pg_SetCmdAll( vector < uint16_t > &cmd );
  RPC_EXPORT void Pg_SetSum( uint16_t delays );
  RPC_EXPORT void Pg_Stop(  );
  RPC_EXPORT void Pg_Single(  );
  RPC_EXPORT void Pg_Trigger(  );
  RPC_EXPORT void Pg_Triggers( uint32_t triggers, uint16_t period );
  RPC_EXPORT void Pg_Loop( uint16_t period );

  // --- data aquisition --------------------------------------------------

  RPC_EXPORT uint32_t Daq_Open( uint32_t buffersize = 10000000,
				uint8_t channel = 0 );
  RPC_EXPORT void Daq_Close( uint8_t channel = 0 ); // tbm A and B
  RPC_EXPORT void Daq_Start( uint8_t channel = 0 ); // tbm A and B
  RPC_EXPORT void Daq_Stop( uint8_t channel = 0 );
  RPC_EXPORT uint32_t Daq_GetSize( uint8_t channel = 0 );
  RPC_EXPORT uint8_t Daq_FillLevel( uint8_t channel );
  RPC_EXPORT uint8_t Daq_FillLevel(  );

  RPC_EXPORT uint8_t Daq_Read( HWvectorR < uint16_t > &data, uint32_t blocksize = 65536, // FW 2.0
                               uint8_t channel = 0 ); // tbm A and B

  RPC_EXPORT uint8_t Daq_Read( HWvectorR < uint16_t > &data,
                               uint32_t blocksize, uint32_t & availsize,
                               uint8_t channel = 0 );

  // FW 1.12  if( blocksize > 32768) blocksize = 32768;

  RPC_EXPORT void Daq_Select_ADC( uint16_t blocksize, uint8_t source, uint8_t start, uint8_t stop = 0 ); // FPGA ADC
  // start token in, stop token out

  RPC_EXPORT void Daq_Select_Deser160( uint8_t shift );

  RPC_EXPORT void Daq_Select_Deser400(  );
  RPC_EXPORT void Daq_Deser400_Reset( uint8_t reset = 3 ); // deser reset bits
  RPC_EXPORT void Daq_Deser400_OldFormat( bool old );

  RPC_EXPORT void Daq_Select_Datagenerator( uint16_t startvalue );

  RPC_EXPORT void Daq_DeselectAll(  );

  // --- ROC/module Communication -----------------------------------------

  // -- set the i2c address for the following commands
  RPC_EXPORT void roc_I2cAddr( uint8_t id );

  // -- sends "ClrCal" command to ROC
  RPC_EXPORT void roc_ClrCal(  );

  // -- sets a single (DAC) register
  void SetDAC( uint8_t reg, uint16_t value ); // DP
  RPC_EXPORT void roc_SetDAC( uint8_t reg, uint8_t value );

  // -- set pixel bits (count <= 60)
  //    M - - - 8 4 2 1
  RPC_EXPORT void roc_Pix( uint8_t col, uint8_t row, uint8_t value );

  // -- trim a single pixel (count < =60)
  RPC_EXPORT void roc_Pix_Trim( uint8_t col, uint8_t row, uint8_t value );

  // -- mask a single pixel (count <= 60)
  RPC_EXPORT void roc_Pix_Mask( uint8_t col, uint8_t row );

  // -- set calibrate at specific column and row
  RPC_EXPORT void roc_Pix_Cal( uint8_t col, uint8_t row,
			       bool sensor_cal = false );

  // -- enable/disable a double column
  RPC_EXPORT void roc_Col_Enable( uint8_t col, bool on );

  // -- mask all pixels of a column and the coresponding double column
  RPC_EXPORT void roc_Col_Mask( uint8_t col );

  // -- mask all pixels and columns of the chip
  RPC_EXPORT void roc_Chip_Mask(  );

  // == TBM functions =====================================================

  RPC_EXPORT bool TBM_Present(  );

  RPC_EXPORT void tbm_Enable( bool on );

  RPC_EXPORT void tbm_Addr( uint8_t hub, uint8_t port );

  RPC_EXPORT void mod_Addr( uint8_t hub );

  RPC_EXPORT void tbm_Set( uint8_t reg, uint8_t value );

  RPC_EXPORT bool tbm_Get( uint8_t reg, uint8_t & value );

  RPC_EXPORT bool tbm_GetRaw( uint8_t reg, uint32_t & value );

  RPC_EXPORT int32_t CountReadouts( int32_t nTriggers );
  RPC_EXPORT int32_t CountReadouts( int32_t nTriggers, int32_t chipId );
  RPC_EXPORT int32_t CountReadouts( int32_t nTriggers, int32_t dacReg,
                                    int32_t dacValue );
  RPC_EXPORT int32_t PH( int32_t col, int32_t row, int32_t trim,
                         int16_t nTriggers );
  RPC_EXPORT int32_t PixelThreshold( int32_t col, int32_t row, int32_t start,
                                     int32_t step, int32_t thrLevel,
                                     int32_t nTrig, int32_t dacReg,
                                     bool xtalk, bool cals );
  RPC_EXPORT bool test_pixel_address( int32_t col, int32_t row );
  RPC_EXPORT int32_t ChipEfficiency_dtb( int16_t nTriggers,
                                         vectorR < uint8_t > &res );

  // ETH functions:

  RPC_EXPORT int8_t CalibratePixel( int16_t nTriggers, int16_t col,
                                    int16_t row, int16_t & nReadouts,
                                    int32_t & PHsum );

  RPC_EXPORT int8_t CalibrateDacScan( int16_t nTriggers, int16_t col,
                                      int16_t row, int16_t dacReg1,
                                      int16_t dacLower1, int16_t dacUpper1,
                                      vectorR < int16_t > &nReadouts,
                                      vectorR < int32_t > &PHsum );

  RPC_EXPORT int8_t CalibrateDacDacScan( int16_t nTriggers, int16_t col,
                                         int16_t row, int16_t dacReg1,
                                         int16_t dacLower1, int16_t dacUpper1,
                                         int16_t dacReg2, int16_t dacLower2,
                                         int16_t dacUpper2,
                                         vectorR < int16_t > &nReadouts,
                                         vectorR < int32_t > &PHsum );
  RPC_EXPORT void ParallelCalibrateDacDacScan( vector < uint8_t > &roc_i2c,
                                               uint16_t nTriggers,
                                               uint8_t col, uint8_t row,
                                               uint8_t dacReg1,
                                               uint8_t dacLower1,
                                               uint8_t dacUpper1,
                                               uint8_t dacReg2,
                                               uint8_t dacLower2,
                                               uint8_t dacUpper2,
                                               uint16_t flags );

  RPC_EXPORT int16_t CalibrateMap( int16_t nTriggers, vectorR < int16_t > &nReadouts, vectorR < int32_t > &PHsum, vectorR < uint32_t > &addres ); // empty
  RPC_EXPORT int16_t CalibrateModule( vector < uint8_t > &roc_i2c,
                                      uint16_t nTriggers, uint16_t flags );

  RPC_EXPORT int16_t TrimChip( vector < int16_t > &trim );
  RPC_EXPORT int16_t TriggerRow( int16_t nTriggers, int16_t col,
                                 vector < int16_t > &rocs, int16_t delay =
                                 4 );

  // --- Wafer test functions

  RPC_EXPORT bool testColPixel( uint8_t col, uint8_t trimbit,
                                vectorR < uint8_t > &res );
  //old RPC_EXPORT bool TestColPixel( uint8_t col, uint8_t trimbit, vectorR<uint8_t> &res);
  RPC_EXPORT bool TestColPixel( uint8_t col, uint8_t trimbit, bool sensor_cal,
                                vectorR < uint8_t > &res );

  // Ethernet test functions

  RPC_EXPORT void Ethernet_Send( string & message );
  RPC_EXPORT uint32_t Ethernet_RecvPackets(  );

  // == Trigger Loop functions for Host-side DAQ ROC/Module testing

  RPC_EXPORT bool SetI2CAddresses( vector < uint8_t > &roc_i2c );
  RPC_EXPORT bool SetTrimValues( uint8_t roc_i2c, vector < uint8_t > &trimvalues ); // uint8 in 2.15

  RPC_EXPORT void SetLoopTriggerDelay( uint16_t delay ); // [BC] (since 2.14)
  RPC_EXPORT void LoopInterruptReset(  );

  // Maps:

  RPC_EXPORT bool LoopMultiRocAllPixelsCalibrate( vector < uint8_t > &roc_i2c,
                                                  uint16_t nTriggers,
                                                  uint16_t flags );
  RPC_EXPORT bool LoopMultiRocOnePixelCalibrate( vector < uint8_t > &roc_i2c,
                                                 uint8_t column, uint8_t row,
                                                 uint16_t nTriggers,
                                                 uint16_t flags );
  RPC_EXPORT bool LoopSingleRocAllPixelsCalibrate( uint8_t roc_i2c,
                                                   uint16_t nTriggers,
                                                   uint16_t flags );
  RPC_EXPORT bool LoopSingleRocOnePixelCalibrate( uint8_t roc_i2c,
                                                  uint8_t column, uint8_t row,
                                                  uint16_t nTriggers,
                                                  uint16_t flags );

  // 1D DacScans:

  RPC_EXPORT bool LoopMultiRocAllPixelsDacScan( vector < uint8_t > &roc_i2c,
                                                uint16_t nTriggers,
                                                uint16_t flags,
                                                uint8_t dac1register,
                                                uint8_t dac1low,
                                                uint8_t dac1high );

  RPC_EXPORT bool LoopMultiRocAllPixelsDacScan( vector < uint8_t > &roc_i2c,
                                                uint16_t nTriggers,
                                                uint16_t flags,
                                                uint8_t dac1register,
                                                uint8_t dac1step,
                                                uint8_t dac1low,
                                                uint8_t dac1high );

  RPC_EXPORT bool LoopMultiRocOnePixelDacScan( vector < uint8_t > &roc_i2c,
                                               uint8_t column, uint8_t row,
                                               uint16_t nTriggers,
                                               uint16_t flags,
                                               uint8_t dac1register,
                                               uint8_t dac1low,
                                               uint8_t dac1high );

  RPC_EXPORT bool LoopMultiRocOnePixelDacScan( vector < uint8_t > &roc_i2c,
                                               uint8_t column, uint8_t row,
                                               uint16_t nTriggers,
                                               uint16_t flags,
                                               uint8_t dac1register,
                                               uint8_t dac1step,
                                               uint8_t dac1low,
                                               uint8_t dac1high );

  RPC_EXPORT bool LoopSingleRocAllPixelsDacScan( uint8_t roc_i2c,
                                                 uint16_t nTriggers,
                                                 uint16_t flags,
                                                 uint8_t dac1register,
                                                 uint8_t dac1low,
                                                 uint8_t dac1high );

  RPC_EXPORT bool LoopSingleRocAllPixelsDacScan( uint8_t roc_i2c,
                                                 uint16_t nTriggers,
                                                 uint16_t flags,
                                                 uint8_t dac1register,
                                                 uint8_t dac1step,
                                                 uint8_t dac1low,
                                                 uint8_t dac1high );

  RPC_EXPORT bool LoopSingleRocOnePixelDacScan( uint8_t roc_i2c,
                                                uint8_t column, uint8_t row,
                                                uint16_t nTriggers,
                                                uint16_t flags,
                                                uint8_t dac1register,
                                                uint8_t dac1low,
                                                uint8_t dac1high );

  RPC_EXPORT bool LoopSingleRocOnePixelDacScan( uint8_t roc_i2c,
                                                uint8_t column, uint8_t row,
                                                uint16_t nTriggers,
                                                uint16_t flags,
                                                uint8_t dac1register,
                                                uint8_t dac1step,
                                                uint8_t dac1low,
                                                uint8_t dac1high );

  // 2D DacDacScans:

  RPC_EXPORT bool LoopMultiRocAllPixelsDacDacScan( vector < uint8_t >
                                                   &roc_i2c,
                                                   uint16_t nTriggers,
                                                   uint16_t flags,
                                                   uint8_t dac1register,
                                                   uint8_t dac1low,
                                                   uint8_t dac1high,
                                                   uint8_t dac2register,
                                                   uint8_t dac2low,
                                                   uint8_t dac2high );

  RPC_EXPORT bool LoopMultiRocAllPixelsDacDacScan( vector < uint8_t >
                                                   &roc_i2c,
                                                   uint16_t nTriggers,
                                                   uint16_t flags,
                                                   uint8_t dac1register,
                                                   uint8_t dac1step,
                                                   uint8_t dac1low,
                                                   uint8_t dac1high,
                                                   uint8_t dac2register,
                                                   uint8_t dac2step,
                                                   uint8_t dac2low,
                                                   uint8_t dac2high );

  RPC_EXPORT bool LoopMultiRocOnePixelDacDacScan( vector < uint8_t > &roc_i2c,
                                                  uint8_t column, uint8_t row,
                                                  uint16_t nTriggers,
                                                  uint16_t flags,
                                                  uint8_t dac1register,
                                                  uint8_t dac1low,
                                                  uint8_t dac1high,
                                                  uint8_t dac2register,
                                                  uint8_t dac2low,
                                                  uint8_t dac2high );

  RPC_EXPORT bool LoopMultiRocOnePixelDacDacScan( vector < uint8_t > &roc_i2c,
                                                  uint8_t column, uint8_t row,
                                                  uint16_t nTriggers,
                                                  uint16_t flags,
                                                  uint8_t dac1register,
                                                  uint8_t dac1step,
                                                  uint8_t dac1low,
                                                  uint8_t dac1high,
                                                  uint8_t dac2register,
                                                  uint8_t dac2step,
                                                  uint8_t dac2low,
                                                  uint8_t dac2high );

  RPC_EXPORT bool LoopSingleRocAllPixelsDacDacScan( uint8_t roc_i2c,
                                                    uint16_t nTriggers,
                                                    uint16_t flags,
                                                    uint8_t dac1register,
                                                    uint8_t dac1low,
                                                    uint8_t dac1high,
                                                    uint8_t dac2register,
                                                    uint8_t dac2low,
                                                    uint8_t dac2high );

  RPC_EXPORT bool LoopSingleRocAllPixelsDacDacScan( uint8_t roc_i2c,
                                                    uint16_t nTriggers,
                                                    uint16_t flags,
                                                    uint8_t dac1register,
                                                    uint8_t dac1step,
                                                    uint8_t dac1low,
                                                    uint8_t dac1high,
                                                    uint8_t dac2register,
                                                    uint8_t dac2step,
                                                    uint8_t dac2low,
                                                    uint8_t dac2high );

  RPC_EXPORT bool LoopSingleRocOnePixelDacDacScan( uint8_t roc_i2c,
                                                   uint8_t column,
                                                   uint8_t row,
                                                   uint16_t nTriggers,
                                                   uint16_t flags,
                                                   uint8_t dac1register,
                                                   uint8_t dac1low,
                                                   uint8_t dac1high,
                                                   uint8_t dac2register,
                                                   uint8_t dac2low,
                                                   uint8_t dac2high );

  RPC_EXPORT bool LoopSingleRocOnePixelDacDacScan( uint8_t roc_i2c,
                                                   uint8_t column,
                                                   uint8_t row,
                                                   uint16_t nTriggers,
                                                   uint16_t flags,
                                                   uint8_t dac1register,
                                                   uint8_t dac1step,
                                                   uint8_t dac1low,
                                                   uint8_t dac1high,
                                                   uint8_t dac2register,
                                                   uint8_t dac2step,
                                                   uint8_t dac2low,
                                                   uint8_t dac2high );

  RPC_EXPORT void VectorTest( vector < uint16_t > &in,
                              vectorR < uint16_t > &out );

  // --- Temperature Measurement ------------------------------------------
  RPC_EXPORT uint16_t GetADC( uint8_t addr );

 private:
  int hubId;
  int nRocs;

};
