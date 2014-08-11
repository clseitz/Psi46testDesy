
//  test board and ROC commands

//  Beat Meier, PSI, 31.8.2007 for Chip/Wafer tester

//  Daniel Pitzl, DESY, 18.3.2014

#include <cstdlib> // abs
#include <math.h>
#include <time.h> // clock
#include <sys/time.h> // gettimeofday, timeval
#include <fstream> // gainfile, dacPar
#include <iostream> // cout
#include <iomanip> // setw
#include <sstream> // stringstream
#include <utility>

#include "TCanvas.h"
#include <TStyle.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>

#include "psi46test.h" // includes pixel_dtb.h
#include "analyzer.h" // includes datastream.h

#include "command.h"
#include "rpc.h"

#include "iseg.h" // HV

using namespace std;

#ifndef _WIN32
#define _int64 long long
#endif

#define DO_FLUSH  if( par.isInteractive() ) tb.Flush();

//#define DAQOPENCLOSE

using namespace std;

// globals:

//const uint32_t Blocksize = 8192; // Wolfram, does not help effmap 111
//const uint32_t Blocksize = 32700; // max 32767 in FW 2.0
//const uint32_t Blocksize = 124800; // FW 2.11, 4160*3*10
//const uint32_t Blocksize = 1048576; // 2^20
const uint32_t Blocksize = 4 * 1024 * 1024; // 
//const uint32_t Blocksize = 16*1024*1024; // ERROR
//const uint32_t Blocksize = 64*1024*1024; // ERROR

//             cable length:     5   48  prober 450 cm  bump bonder
//int clockPhase =  4; //  4    0    19    5       16
//int deserPhase =  4; //  4    4     5    6        5
//int deserPhase =  5; //  FEC 30 cm cable

int dacval[16][256];            // DP
string dacName[256];

int Chip = 400;
int Module = 3;

int roclist[16] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int modthr[16][52][80];
int modtrm[16][52][80];

// gain:

bool haveGain = 0;

double p0[52][80];
double p1[52][80];
double p2[52][80];
double p3[52][80];
double p4[52][80];
double p5[52][80];

TCanvas *c1;

TH1D *h10;
TH1D *h11;
TH1D *h12;
TH1D *h13;
TH1D *h14;
TH1D *h15;
TH1D *h16;

TH2D *h21;
TH2D *h22;
TH2D *h23;
TH2D *h24;

Iseg iseg;

//------------------------------------------------------------------------------
class TBstate
{
  bool daqOpen;
  uint32_t daqSize;
  int clockPhase;
  int deserPhase;

public:
  TBstate(  ):daqOpen( 0 ), daqSize( 0 ), clockPhase( 4 ), deserPhase( 4 )
  {
  }
  ~TBstate(  )
  {
  }

  void SetDaqOpen( const bool open )
  {
    daqOpen = open;
  }
  bool GetDaqOpen(  )
  {
    return daqOpen;
  }

  void SetDaqSize( const uint32_t size )
  {
    daqSize = size;
  }
  uint32_t GetDaqSize(  )
  {
    return daqSize;
  }

  void SetClockPhase( const uint32_t phase )
  {
    clockPhase = phase;
  }
  uint32_t GetClockPhase(  )
  {
    return clockPhase;
  }

  void SetDeserPhase( const uint32_t phase160 )
  {
    deserPhase = phase160;
  }
  uint32_t GetDeserPhase(  )
  {
    return deserPhase;
  }
};

TBstate tbState;

//------------------------------------------------------------------------------
CMD_PROC( showtb ) // test board state
{
  Log.section( "TBSTATE", true );

  cout << " 40 MHz clock phase " << tbState.GetClockPhase(  ) << " ns" << endl;
  cout << "160 MHz deser phase " << tbState.GetDeserPhase(  ) << " ns" << endl;
  if( tbState.GetDaqOpen(  ) )
    cout << "DAQ memeory size " << tbState.GetDaqSize(  ) << " words" << endl;
  else
    cout << "no DAQ open" << endl;

  uint8_t status = tb.GetStatus(  );
  printf( "SD card detect: %c\n", ( status & 8 ) ? '1' : '0' );
  printf( "CRC error:      %c\n", ( status & 4 ) ? '1' : '0' );
  printf( "Clock good:     %c\n", ( status & 2 ) ? '1' : '0' );
  printf( "CLock present:  %c\n", ( status & 1 ) ? '1' : '0' );

  Log.printf( "Clock phase %i\n", tbState.GetClockPhase(  ) );
  Log.printf( "Deser phase %i\n", tbState.GetDeserPhase(  ) );
  if( tbState.GetDaqOpen(  ) )
    Log.printf( "DAQ memory size %i\n", tbState.GetDaqSize(  ) );
  else
    Log.printf( "DAQ not open\n" );

  cout << "PixelAddressInverted " << tb.GetPixelAddressInverted(  ) << endl;
  cout << "PixelAddressInverted " << tb.invertAddress << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( showhv ) // iseg HV status
{
  iseg.status(  );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( scan ) // scan for DTB on USB
{
  CTestboard *tb = new CTestboard;
  string name;
  vector < string > devList;
  unsigned int nDev, nr;

  try {
    if( !tb->EnumFirst( nDev ) )
      throw int ( 1 );
    for( nr = 0; nr < nDev; nr++ ) {
      if( !tb->EnumNext( name ) )
        throw int ( 2 );
      if( name.size(  ) < 4 )
        continue;
      if( name.compare( 0, 4, "DTB_" ) == 0 )
        devList.push_back( name );
    }
  }
  catch( int e ) {
    switch ( e ) {
    case 1:
      printf( "Cannot access the USB driver\n" );
      break;
    case 2:
      printf( "Cannot read name of connected device\n" );
      break;
    }
    delete tb;
    return true;
  }

  if( devList.size(  ) == 0 ) {
    printf( "no DTB connected\n" );
    return true;
  }

  for( nr = 0; nr < devList.size(  ); nr++ )
    try {
    printf( "%10s: ", devList[nr].c_str(  ) );
    if( !tb->Open( devList[nr], false ) ) {
      printf( "DTB in use\n" );
      continue;
    }
    unsigned int bid = tb->GetBoardId(  );
    printf( "DTB Id %u\n", bid );
    tb->Close(  );
    }
  catch( ... ) {
    printf( "DTB not identifiable\n" );
    tb->Close(  );
  }

  delete tb;                    // not permanentely opened

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( open )
{
  if( tb.IsConnected(  ) ) {
    printf( "Already connected to DTB.\n" );
    return true;
  }

  string usbId;
  char name[80];
  if( PAR_IS_STRING( name, 79 ) )
    usbId = name;
  else if( !tb.FindDTB( usbId ) )
    return true;

  bool status = tb.Open( usbId, false );

  if( !status ) {
    printf( "USB error: %s\nCould not connect to DTB %s\n",
            tb.ConnectionError(  ), usbId.c_str(  ) );
    return true;
  }

  printf( "DTB %s opened\n", usbId.c_str(  ) );

  string info;
  tb.GetInfo( info );
  printf( "--- DTB info-------------------------------------\n"
          "%s"
          "-------------------------------------------------\n",
          info.c_str(  ) );

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( close )
{
  tb.Close(  );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( welcome )
{
  tb.Welcome(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( setled )
{
  int value;
  PAR_INT( value, 0, 0x3f );
  tb.SetLed( value );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( log ) // put comment into log from script or command line
{
  char s[256];
  PAR_STRINGEOL( s, 255 );
  Log.printf( "%s\n", s );
  return true;
}

//------------------------------------------------------------------------------
bool UpdateDTB( const char *filename )
{
  fstream src;

  if( tb.UpgradeGetVersion(  ) == 0x0100 ) {
   // open file
    src.open( filename );
    if( !src.is_open(  ) ) {
      printf( "ERROR UPGRADE: Could not open \"%s\"!\n", filename );
      return false;
    }

   // check if upgrade is possible
    printf( "Start upgrading DTB.\n" );
    if( tb.UpgradeStart( 0x0100 ) != 0 ) {
      string msg;
      tb.UpgradeErrorMsg( msg );
      printf( "ERROR UPGRADE: %s!\n", msg.data(  ) );
      return false;
    }

   // download data
    printf( "Download running ...\n" );
    string rec;
    uint16_t recordCount = 0;
    while( true ) {
      getline( src, rec );
      if( src.good(  ) ) {
        if( rec.size(  ) == 0 )
          continue;
        recordCount++;
        if( tb.UpgradeData( rec ) != 0 ) {
          string msg;
          tb.UpgradeErrorMsg( msg );
          printf( "ERROR UPGRADE: %s!\n", msg.data(  ) );
          return false;
        }
      }
      else if( src.eof(  ) )
        break;
      else {
        printf( "ERROR UPGRADE: Error reading \"%s\"!\n", filename );
        return false;
      }
    }

    if( tb.UpgradeError(  ) != 0 ) {
      string msg;
      tb.UpgradeErrorMsg( msg );
      printf( "ERROR UPGRADE: %s!\n", msg.data(  ) );
      return false;
    }

   // write EPCS FLASH
    printf( "DTB download complete.\n" );
    tb.mDelay( 200 );
    printf( "FLASH write start (LED 1..4 on)\n"
            "DO NOT INTERUPT DTB POWER !\n"
            "Wait till LEDs goes off\n"
            "  exit from psi46test\n" "  power cycle the DTB\n" );
    tb.UpgradeExec( recordCount );
    tb.Flush(  );
    return true;
  }

  printf( "ERROR UPGRADE: Could not upgrade this DTB version!\n" );
  return false;
}

//------------------------------------------------------------------------------
CMD_PROC( upgrade )
{
  char filename[256];
  PAR_STRING( filename, 255 );
  UpdateDTB( filename );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rpcinfo )
{
  string name, call, ts;

  tb.GetRpcTimestamp( ts );
  int version = tb.GetRpcVersion(  );
  int n = tb.GetRpcCallCount(  );

  printf( "--- DTB RPC info ----------------------------------------\n" );
  printf( "RPC version:     %i.%i\n", version / 256, version & 0xff );
  printf( "RPC timestamp:   %s\n", ts.c_str(  ) );
  printf( "Number of calls: %i\n", n );
  printf( "Function calls:\n" );
  for( int i = 0; i < n; ++i ) {
    tb.GetRpcCallName( i, name );
    rpc_TranslateCallName( name, call );
    printf( "%5i: %s\n", i, call.c_str(  ) );
  }
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( info )
{
  string s;
  tb.GetInfo( s );
  printf( "--- DTB info ------------------------------------\n%s"
          "-------------------------------------------------\n",
          s.c_str(  ) );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( version )
{
  string hw;
  tb.GetHWVersion( hw );
  int fw = tb.GetFWVersion(  );
  int sw = tb.GetSWVersion(  );
  printf( "%s: FW=%i.%02i SW=%i.%02i\n", hw.c_str(  ), fw / 256, fw % 256,
          sw / 256, sw % 256 );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( boardid )
{
  int id = tb.GetBoardId(  );
  printf( "Board Id = %i\n", id );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( init )
{
  tb.Init(  ); // done at power up?
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( flush )
{
  tb.Flush(  ); // send buffer of USB commands to DTB
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clear )
{
  tb.Clear(  ); // rpc_io->Clear()
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( udelay )
{
  int del;
  PAR_INT( del, 0, 65500 );
  if( del )
    tb.uDelay( del );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( mdelay )
{
  int ms;
  PAR_INT( ms, 1, 10000 );
  tb.mDelay( ms );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( d1 )
{
  int sig;
  PAR_INT( sig, 0, 31 );
  tb.SignalProbeD1( sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( d2 )
{
  int sig;
  PAR_INT( sig, 0, 31 );
  tb.SignalProbeD2( sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( a1 )
{
  int sig;
  PAR_INT( sig, 0, 7 );
  tb.SignalProbeA1( sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( a2 )
{
  int sig;
  PAR_INT( sig, 0, 7 );
  tb.SignalProbeA2( sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( probeadc )
{
  int sig;
  PAR_INT( sig, 0, 7 );
  tb.SignalProbeADC( sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clksrc )
{
  int source;
  PAR_INT( source, 0, 1 );
  tb.SetClockSource( source );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clkok )
{
  if( tb.IsClockPresent(  ) )
    printf( "clock ok\n" );
  else
    printf( "clock missing\n" );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( fsel ) // clock frequency selector, 0 = 40 MHz
{
  int div;
  PAR_INT( div, 0, 5 );
  tb.SetClock( div );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
// clock stretch
//pixel_dtb.h: #define STRETCH_AFTER_TRG  0
//pixel_dtb.h: #define STRETCH_AFTER_CAL  1
//pixel_dtb.h: #define STRETCH_AFTER_RES  2
//pixel_dtb.h: #define STRETCH_AFTER_SYNC 3
// width = 0 disable stretch

CMD_PROC( stretch ) // src=1 (after cal)  delay=8  width=999
{
  int src, delay, width;
  PAR_INT( src, 0, 3 );
  PAR_INT( delay, 0, 1023 );
  PAR_INT( width, 0, 0xffff );
  tb.SetClockStretch( src, delay, width );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clk ) // clock phase delay (relative to what?)
{
  int ns, duty;
  PAR_INT( ns, 0, 400 );
  if( !PAR_IS_INT( duty, -8, 8 ) )
    duty = 0;
  tb.Sig_SetDelay( SIG_CLK, ns, duty );
  tb.Sig_SetDelay( SIG_CTR, ns, duty );
  tb.Sig_SetDelay( SIG_SDA, ns + 15, duty );
  tb.Sig_SetDelay( SIG_TIN, ns + 5, duty );
  tbState.SetClockPhase( ns );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( sda ) // I2C data delay
{
  int ns, duty;
  PAR_INT( ns, 0, 400 );
  if( !PAR_IS_INT( duty, -8, 8 ) )
    duty = 0;
  tb.Sig_SetDelay( SIG_SDA, ns, duty );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
/*
  CMD_PROC(rda)
  {
  int ns;
  PAR_INT(ns,0,400);
  tb.SetDelay(DELAYSIG_RDA, ns);
  DO_FLUSH
  return true;
  }
*/

//------------------------------------------------------------------------------
CMD_PROC( ctr ) // cal-trig-reset delay
{
  int ns, duty;
  PAR_INT( ns, 0, 400 );
  if( !PAR_IS_INT( duty, -8, 8 ) )
    duty = 0;
  tb.Sig_SetDelay( SIG_CTR, ns, duty );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tin ) // token in delay
{
  int ns, duty;
  PAR_INT( ns, 0, 400 );
  if( !PAR_IS_INT( duty, -8, 8 ) )
    duty = 0;
  tb.Sig_SetDelay( SIG_TIN, ns, duty );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clklvl )
{
  int lvl;
  PAR_INT( lvl, 0, 15 );
  tb.Sig_SetLevel( SIG_CLK, lvl );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( sdalvl )
{
  int lvl;
  PAR_INT( lvl, 0, 15 );
  tb.Sig_SetLevel( SIG_SDA, lvl );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( ctrlvl )
{
  int lvl;
  PAR_INT( lvl, 0, 15 );
  tb.Sig_SetLevel( SIG_CTR, lvl );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tinlvl )
{
  int lvl;
  PAR_INT( lvl, 0, 15 );
  tb.Sig_SetLevel( SIG_TIN, lvl );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clkmode )
{
  int mode;
  PAR_INT( mode, 0, 3 );
  if( mode == 3 ) {
    int speed;
    PAR_INT( speed, 0, 31 );
    tb.Sig_SetPRBS( SIG_CLK, speed );
  }
  else
    tb.Sig_SetMode( SIG_CLK, mode );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( sdamode )
{
  int mode;
  PAR_INT( mode, 0, 3 );
  if( mode == 3 ) {
    int speed;
    PAR_INT( speed, 0, 31 );
    tb.Sig_SetPRBS( SIG_SDA, speed );
  }
  else
    tb.Sig_SetMode( SIG_SDA, mode );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( ctrmode )
{
  int mode;
  PAR_INT( mode, 0, 3 );
  if( mode == 3 ) {
    int speed;
    PAR_INT( speed, 0, 31 );
    tb.Sig_SetPRBS( SIG_CTR, speed );
  }
  else
    tb.Sig_SetMode( SIG_CTR, mode );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tinmode )
{
  int mode;
  PAR_INT( mode, 0, 3 );
  if( mode == 3 ) {
    int speed;
    PAR_INT( speed, 0, 31 );
    tb.Sig_SetPRBS( SIG_TIN, speed );
  }
  else
    tb.Sig_SetMode( SIG_TIN, mode );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( sigoffset )
{
  int offset;
  PAR_INT( offset, 0, 15 );
  tb.Sig_SetOffset( offset );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( lvds )
{
  tb.Sig_SetLVDS(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( lcds )
{
  tb.Sig_SetLCDS(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
/*
  CMD_PROC(tout)
  {
  int ns;
  PAR_INT(ns,0,450);
  tb.SetDelay(SIGNAL_TOUT, ns);
  DO_FLUSH
  return true;
  }

  CMD_PROC(trigout)
  {
  int ns;
  PAR_INT(ns,0,450);
  tb.SetDelay(SIGNAL_TRGOUT, ns);
  DO_FLUSH
  return true;
  }
*/

//------------------------------------------------------------------------------
CMD_PROC( pon )
{
  tb.Pon(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( poff )
{
  tb.Poff(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( va )
{
  int value;
  PAR_INT( value, 0, 3000 );
  tb._SetVA( value );
  dacval[0][VA] = value;
  Log.printf( "[SETVA] %i\n", value );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vd )
{
  int value;
  PAR_INT( value, 0, 3000 );
  tb._SetVD( value );
  dacval[0][VD] = value;
  Log.printf( "[SETVD] %i\n", value );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( ia ) // analog current limit [mA]
{
  int value;
  PAR_INT( value, 0, 1200 );
  tb._SetIA( value * 10 ); // internal unit is 0.1 mA
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( id ) // digital current limit [mA]
{
  int value;
  PAR_INT( value, 0, 1200 );
  tb._SetID( value * 10 ); // internal unit is 0.1 mA
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getva ) // measure analog supply voltage
{
  double v = tb.GetVA(  );
  printf( "VA %1.3f V\n", v );
  Log.printf( "VA %1.3f V\n", v );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getvd ) // measure digital supply voltage
{
  double v = tb.GetVD(  );
  printf( "VD %1.3f V\n", v );
  Log.printf( "VD %1.3f V\n", v );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getia ) // measure analog supply current
{
  double ia = tb.GetIA(  ) * 1E3; // [mA]
  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      nrocs++;
  printf( "IA %4.1f mA for %i ROCs = %4.1f mA per ROC\n",
          ia, nrocs, ia / nrocs );
  Log.printf( "IA %4.1f mA for %i ROCs = %4.1f mA per ROC\n",
              ia, nrocs, ia / nrocs );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getid ) // measure digital supply current
{
  double id = tb.GetID(  ) * 1E3; // [mA]
  printf( "ID %1.1f mA\n", id );
  Log.printf( "ID %1.1f mA\n", id );
  return true;
}

vector < double >idv;

//------------------------------------------------------------------------------
CMD_PROC( getid2 ) // measure digital supply current
{
  int mode;
  PAR_INT( mode, 0, 15 );

  if( mode == 0 ) {
    idv.clear(  );
  }
  if( mode == 1 ) {
    double id = tb.GetID(  ) * 1E3;
    printf( "ID = %1.1f mA\n", id );
    idv.push_back( id );
  }
  else if( mode == 2 ) {
    for( size_t i = 0; i < idv.size(  ); ++i )
      cout << setw( 2 ) << i << "  " << idv.at( i ) << endl;
    cout << endl;
   // for ROOT:
    cout << "Double_t id[" << idv.size(  ) << "] = {" << endl;
    for( size_t i = 0; i < idv.size(  ); ++i ) {
      if( i > 0 )
        cout << ", ";
      cout << idv.at( i );
    }
    cout << " };" << endl;
  }
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( hvon )
{
  tb.HVon(  ); // close HV relais on DTB
  DO_FLUSH;
  Log.printf( "[HVON]" );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vb )
{
  int value;
  PAR_INT( value, 0, 300 );
  iseg.setVoltage( value );
  Log.printf( "[SETVB] -%i\n", value );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( hvoff )
{
  iseg.setVoltage( 0 );
  tb.HVoff(  ); // open HV relais on DTB
  DO_FLUSH;
  Log.printf( "[HVOFF]" );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( reson )
{
  tb.ResetOn(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( resoff )
{
  tb.ResetOff(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rocaddr )
{
  int addr;
  PAR_INT( addr, 0, 15 );
  tb.SetRocAddress( addr );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rowinvert )
{
  tb.SetPixelAddressInverted( 1 );
  tb.invertAddress = 1;
  cout << "SetPixelAddressInverted" << endl;
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( chip )
{
  int chip;
  PAR_INT( chip, 0, 9999 );

  Chip = chip;

  cout << "Chip " << Chip << endl;

  if( Chip >= 200 && Chip < 299 ) {
    tb.SetPixelAddressInverted( 1 );
    tb.invertAddress = 1;
    cout << "SetPixelAddressInverted" << endl;
  }
  else { // just to be sure
    tb.SetPixelAddressInverted( 0 );
    tb.invertAddress = 0;
    cout << "SetPixelAddressNotInverted" << endl;
  }

  dacName[1] = "Vdig";
  dacName[2] = "Vana";
  if( Chip < 400 )
    dacName[3] = "Vsf";
  else
    dacName[3] = "Vsh"; // digV2.1
  dacName[4] = "Vcomp";

  dacName[5] = "Vleak_comp"; // only analog
  dacName[6] = "VrgPr"; // removed on dig
  dacName[7] = "VwllPr";
  dacName[8] = "VrgSh"; // removed on dig
  dacName[9] = "VwllSh";

  dacName[10] = "VhldDel";
  dacName[11] = "Vtrim";
  dacName[12] = "VthrComp";

  dacName[13] = "VIBias_Bus";
  dacName[14] = "Vbias_sf";

  dacName[15] = "VoffsetOp";
  dacName[16] = "VIbiasOp"; // analog
  if( Chip < 400 )
    dacName[17] = "VoffsetRO"; //
  else
    dacName[17] = "PHOffset"; // digV2.1
  dacName[18] = "VIon";

  dacName[19] = "Vcomp_ADC"; // dig
  if( Chip < 400 )
    dacName[20] = "VIref_ADC"; // dig
  else
    dacName[20] = "PHScale"; // digV2.1
  dacName[21] = "VIbias_roc"; // analog
  dacName[22] = "VIColOr";

  dacName[25] = "VCal";
  dacName[26] = "CalDel";

  dacName[31] = "VD ";
  dacName[32] = "VA ";

  dacName[253] = "CtrlReg";
  dacName[254] = "WBC";
  dacName[255] = "RBReg";

  string gainFileName;

  if( Chip == 401 )
    gainFileName = "phroc-c401-Ia25-trim30.dat";
  if( Chip == 402 )
    gainFileName = "phroc-c402-trim30.dat";
  if( Chip == 405 )
    gainFileName = "phroc-c405-trim30.dat";
 
  if( gainFileName.length(  ) > 0 ) {

    ifstream gainFile( gainFileName.c_str(  ) );

    if( gainFile ) {

      haveGain = 1;
      cout << "gainFile: " << gainFileName << endl;

      char ih[99];
      int icol;
      int irow;

      while( gainFile >> ih ) {
        gainFile >> icol;
        gainFile >> irow;
        gainFile >> p0[icol][irow];
        gainFile >> p1[icol][irow];
        gainFile >> p2[icol][irow];
        gainFile >> p3[icol][irow];
        gainFile >> p4[icol][irow];
        gainFile >> p5[icol][irow];
      }

    } // gainFile open

  } // gainFileName

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgset )
{
  int addr, delay, bits;
  PAR_INT( addr, 0, 255 );
  PAR_INT( bits, 0, 63 );
  PAR_INT( delay, 0, 255 );
  tb.Pg_SetCmd( addr, ( bits << 8 ) + delay );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgstop )
{
  tb.Pg_Stop(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgsingle )
{
  tb.Pg_Single(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgtrig )
{
  tb.Pg_Trigger(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgloop )
{
  int period;
  PAR_INT( period, 0, 65535 );
  tb.Pg_Loop( period );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( trigdel )
{
  int delay;
  PAR_INT( delay, 0, 65535 );
  tb.SetLoopTriggerDelay( delay ); // [BC]
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
// inverse decorrelated Weibull PH -> large Vcal DAC
double PHtoVcal( double ph, uint16_t col, uint16_t row )
{
  if( !haveGain )
    return ph;

  if( col > 51 )
    return ph;
  if( row > 79 )
    return ph;

 // phcal2ps decorrelated: ph = p4 + p3*exp(-t^p2), t = p0 + q/p1
 // phroc2ps decorrelated: ph = p4 - p3*exp(-t^p2), t = p0 + q/p1

  double Ared = ph - p4[col][row]; // p4 is asymptotic maximum

  if( Ared >= 0 ) {
    Ared = -0.1; // avoid overflow
  }

  double a3 = p3[col][row];     // negative

  if( a3 > 0 )
    a3 = -a3; // sign changed

 // large Vcal = ( (-ln((A-p4)/p3))^1/p2 - p0 )*p1
 // phroc: q =  ( (-ln(-(A-p4)/p3))^1/p2 - p0 )*p1 // sign changed

  double vc = p1[col][row] * ( pow( -log( Ared / a3 ), 1 / p2[col][row] ) - p0[col][row] ); // [large Vcal]

  if( vc > 999 )
    cout << "overflow " << vc << " at"
	 << setw( 3 ) << col
	 << setw( 3 ) << row << ", Ared " << Ared << ", a3 " << a3 << endl;

  if( dacval[0][CtrlReg] == 0 )
    return vc * p5[col][row]; // small Vcal

  return vc; // large Vcal
}

//------------------------------------------------------------------------------
CMD_PROC( upd ) // redraw ROOT canvas; only works for global histos
{
  int plot;
  if( !PAR_IS_INT( plot, 10, 29 ) ) {
    gPad->Modified(  );
    gPad->Update(  );
   //c1->Modified();
   //c1->Update();
  }
  else if( plot == 10 ) {
    gStyle->SetOptStat( 111111 );
    h10->Draw(  );
    c1->Update(  );
  }
  else if( plot == 11 ) {
    gStyle->SetOptStat( 111111 );
    h11->Draw(  );
    c1->Update(  );
  }
  else if( plot == 12 ) {
    gStyle->SetOptStat( 111111 );
    h12->Draw(  );
    c1->Update(  );
  }
  else if( plot == 13 ) {
    gStyle->SetOptStat( 111111 );
    h13->Draw(  );
    c1->Update(  );
  }
  else if( plot == 14 ) {
    gStyle->SetOptStat( 111111 );
    h14->Draw(  );
    c1->Update(  );
  }
  else if( plot == 21 ) {
    double statY = gStyle->GetStatY(  );
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    h21->Draw( "colz" );
    c1->Update(  );
    gStyle->SetStatY( statY );
  }
  else if( plot == 22 ) {
    double statY = gStyle->GetStatY(  );
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    h22->Draw( "colz" );
    c1->Update(  );
    gStyle->SetStatY( statY );
  }
  else if( plot == 23 ) {
    double statY = gStyle->GetStatY(  );
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    h23->Draw( "colz" );
    c1->Update(  );
    gStyle->SetStatY( statY );
  }
  else if( plot == 24 ) {
    double statY = gStyle->GetStatY(  );
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    h24->Draw( "colz" );
    c1->Update(  );
    gStyle->SetStatY( statY );
  }

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dopen )
{
  int buffersize;
  PAR_INT( buffersize, 0, 64100200 );
  int channel;
  if( !PAR_IS_INT( channel, 0, 7 ) )
    channel = 0;

  buffersize = tb.Daq_Open( buffersize, channel );
  if( buffersize == 0 )
    cout << "Daq_Open error for channel " << channel << ", size " <<
      buffersize << endl;
  else {
    cout << buffersize << " words allocated for data buffer " << channel <<
      endl;
    tbState.SetDaqOpen( 1 );
    tbState.SetDaqSize( buffersize );
  }

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dsel ) // dsel 160 or 400
{
  int MHz;
  PAR_INT( MHz, 160, 400 );
  if( MHz > 200 )
    tb.Daq_Select_Deser400(  );
  else
    tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dselmod )
{
  tb.Daq_Select_Deser400(  );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dselroca ) // activate ADC for analog ROC ?
{
  int datasize;
  PAR_INT( datasize, 1, 2047 );
  tb.Daq_Select_ADC( datasize, 1, 4, 6 );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dselsim )
{
  int startvalue;
  if( !PAR_IS_INT( startvalue, 0, 16383 ) )
    startvalue = 0;
  tb.Daq_Select_Datagenerator( startvalue );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dseloff )
{
  tb.Daq_DeselectAll(  );
  DO_FLUSH return true;
}

//------------------------------------------------------------------------------
CMD_PROC( deser )
{
  int shift;
  PAR_INT( shift, 0, 7 );
  tb.Daq_Select_Deser160( shift );
  tbState.SetDeserPhase( shift );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( deser160 ) // scan DESER160 and clock phase for header 7F8
{
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
 //DP tb.Pg_SetCmd(20, PG_RESR);
 //DP tb.Pg_SetCmd(0, PG_TOK);

  vector < uint16_t > data;

  vector < std::pair < int, int > > goodvalues;

  int x, y;
  printf( "deser phs 0     1     2     3     4     5     6     7\n" );

  for( y = 0; y < 25; y++ ) { // clk

    printf( "clk %2i:", y );

    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );

    for( x = 0; x < 8; x++ ) { // deser160 phase

      tb.Daq_Select_Deser160( x );
      tb.uDelay( 10 );

      tb.Daq_Start(  );

      tb.uDelay( 10 );

      tb.Pg_Single(  );

      tb.uDelay( 10 );

      tb.Daq_Stop(  );

      data.resize( tb.Daq_GetSize(  ), 0 );

      tb.Daq_Read( data, 100 );

      if( data.size(  ) ) {
        int h = data[0] & 0xffc;
        if( h == 0x7f8 ) {
          printf( " <%03X>", int ( data[0] & 0xffc ) );
          goodvalues.push_back( std::make_pair( y, x ) );
        }
        else
          printf( "  %03X ", int ( data[0] & 0xffc ) );
      }
      else
        printf( "  ... " );

    } // deser phase

    printf( "\n" );

  } // clk phase

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
#endif

  printf( "Old values: clk delay %i, deserPhase %i\n",
          tbState.GetClockPhase(  ), tbState.GetDeserPhase(  ) );

  if( goodvalues.size(  ) == 0 ) {

    printf
      ( "No value found where header could be read back - no adjustments made.\n" );
    tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) ); // back to default
    y = tbState.GetClockPhase(  ); // back to default
    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );
    return true;
  }
  printf( "Good values are:\n" );
  for( std::vector < std::pair < int, int > >::const_iterator it =
       goodvalues.begin(  ); it != goodvalues.end(  ); it++ ) {
    printf( "%i %i\n", it->first, it->second );
  }
  const int select = floor( 0.5 * goodvalues.size(  ) - 0.5 );
  tbState.SetClockPhase( goodvalues[select].first );
  tbState.SetDeserPhase( goodvalues[select].second );
  printf( "New values: clock delay %i, deserPhase %i\n",
          tbState.GetClockPhase(  ), tbState.GetDeserPhase(  ) );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) ); // set new
  y = tbState.GetClockPhase(  );
  tb.Sig_SetDelay( SIG_CLK, y );
  tb.Sig_SetDelay( SIG_CTR, y );
  tb.Sig_SetDelay( SIG_SDA, y + 15 );
  tb.Sig_SetDelay( SIG_TIN, y + 5 );

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dreset ) // dreset 3 = Deser400 reset
{
  int reset;
  PAR_INT( reset, 0, 255 ); // bits, 3 = 11
  tb.Daq_Deser400_Reset( reset );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dmodres )
{
  int reset;
  if( !PAR_IS_INT( reset, 0, 3 ) )
    reset = 3;
  tb.Daq_Deser400_Reset( reset );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dclose )
{
  int channel;
  if( !PAR_IS_INT( channel, 0, 7 ) )
    channel = 0;
  tb.Daq_Close( channel );
  tbState.SetDaqOpen( 0 );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dstart )
{
  int channel;
  if( !PAR_IS_INT( channel, 0, 7 ) )
    channel = 0;
  tb.Daq_Start( channel );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dstop )
{
  int channel;
  if( !PAR_IS_INT( channel, 0, 7 ) )
    channel = 0;
  tb.Daq_Stop( channel );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dsize )
{
  int channel;
  if( !PAR_IS_INT( channel, 0, 7 ) )
    channel = 0;
  unsigned int size = tb.Daq_GetSize( channel );
  printf( "size = %u\n", size );
  return true;
}

//------------------------------------------------------------------------------
void DecodeTbmHeader( unsigned int raw )
{
  int evNr = raw >> 8;
  int stkCnt = raw & 6;
  printf( "  EV(%3i) STF(%c) PKR(%c) STKCNT(%2i)",
          evNr,
          ( raw & 0x0080 ) ? '1' : '0',
          ( raw & 0x0040 ) ? '1' : '0', stkCnt );
}

//------------------------------------------------------------------------------
void DecodeTbmTrailer( unsigned int raw )
{
  int dataId = ( raw >> 6 ) & 0x3;
  int data = raw & 0x3f;
  printf
    ( "  NTP(%c) RST(%c) RSR(%c) SYE(%c) SYT(%c) CTC(%c) CAL(%c) SF(%c) D%i(%2i)",
      ( raw & 0x8000 ) ? '1' : '0', ( raw & 0x4000 ) ? '1' : '0',
      ( raw & 0x2000 ) ? '1' : '0', ( raw & 0x1000 ) ? '1' : '0',
      ( raw & 0x0800 ) ? '1' : '0', ( raw & 0x0400 ) ? '1' : '0',
      ( raw & 0x0200 ) ? '1' : '0', ( raw & 0x0100 ) ? '1' : '0', dataId,
      data );
}

//------------------------------------------------------------------------------
void DecodePixel( unsigned int raw )
{
  unsigned int ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
  raw >>= 9;
  int c = ( raw >> 12 ) & 7;
  c = c * 6 + ( ( raw >> 9 ) & 7 );

  int r2 = ( raw >> 6 ) & 7;
  int r1 = ( raw >> 3 ) & 7;
  int r0 = ( raw >> 0 ) & 7;

  if( tb.invertAddress ) {
    r2 ^= 0x7;
    r1 ^= 0x7;
    r0 ^= 0x7;
  }

  int r = r2 * 36 + r1 * 6 + r0;

  int y = 80 - r / 2;
  int x = 2 * c + ( r & 1 );
 //printf( "   Pixel [%05o] %2i/%2i: %3u", raw, x, y, ph);
  printf( " = pix %2i.%2i ph %3u", x, y, ph );
}

//------------------------------------------------------------------------------
#define DECBUFFERSIZE 2048

class Decoder
{
  int printEvery;

  int nReadout;
  int nPixel;

  FILE *f;
  int nSamples;
  uint16_t *samples;

  int x, y, ph;
public:
  Decoder(  ):printEvery( 0 ), nReadout( 0 ), nPixel( 0 ), f( 0 ),
	      nSamples( 0 ), samples( 0 )
  {
  }
  ~Decoder(  )
  {
    Close(  );
  }
  bool Open( const char *filename );
  void Close(  )
  {
    if( f )
      fclose( f );
    f = 0;
    delete[]samples;
  }
  bool Sample( uint16_t sample );
  void DumpSamples( int n );
  void Translate( unsigned long raw );
  uint16_t GetX(  )
  {
    return x;
  }
  uint16_t GetY(  )
  {
    return y;
  }
  uint16_t GetPH(  )
  {
    return ph;
  }
  void AnalyzeSamples(  );
};

//------------------------------------------------------------------------------
bool Decoder::Open( const char *filename )
{
  samples = new uint16_t[DECBUFFERSIZE];
  f = fopen( filename, "wt" );
  return f != 0;
}

//------------------------------------------------------------------------------
bool Decoder::Sample( uint16_t sample )
{
  if( sample & 0x8000 ) { // start marker
    if( nReadout && printEvery >= 1000 ) {
      AnalyzeSamples(  );
      printEvery = 0;
    }
    else
      printEvery++;
    nReadout++;
    nSamples = 0;
  }
  if( nSamples < DECBUFFERSIZE ) {
    samples[nSamples++] = sample;
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void Decoder::DumpSamples( int n )
{
  if( nSamples < n )
    n = nSamples;
  for( int i = 0; i < n; ++i )
    fprintf( f, " %04X", ( unsigned int ) ( samples[i] ) );
  fprintf( f, " ... %04X\n", ( unsigned int ) ( samples[nSamples - 1] ) );
}

//------------------------------------------------------------------------------
void Decoder::Translate( unsigned long raw )
{
  ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
  raw >>= 9;
  int c = ( raw >> 12 ) & 7;
  c = c * 6 + ( ( raw >> 9 ) & 7 );

  int r2 = ( raw >> 6 ) & 7;
  int r1 = ( raw >> 3 ) & 7;
  int r0 = ( raw >> 0 ) & 7;

  if( tb.invertAddress ) {
    r2 ^= 0x7;
    r1 ^= 0x7;
    r0 ^= 0x7;
  }

  int r = r2 * 36 + r1 * 6 + r0;

  y = 80 - r / 2;
  x = 2 * c + ( r & 1 );
}

//------------------------------------------------------------------------------
void Decoder::AnalyzeSamples(  )
{
  if( nSamples < 1 ) {
    nPixel = 0;
    return;
  }
  fprintf( f, "%5i: %03X: ", nReadout,
           ( unsigned int ) ( samples[0] & 0xfff ) );
  nPixel = ( nSamples - 1 ) / 2;
  int pos = 1;
  for( int i = 0; i < nPixel; ++i ) {
    unsigned long raw = ( samples[pos++] & 0xfff ) << 12;
    raw += samples[pos++] & 0xfff;
    Translate( raw );
    fprintf( f, " %2i", x );
  }
 // for( pos = 1; pos < nSamples; pos++) fprintf(f, " %03X", int(samples[pos]));
  fprintf( f, "\n" );
}

//------------------------------------------------------------------------------
CMD_PROC( dread ) // daq read and print as ROC data
{
  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      nrocs++;
  cout << nrocs << " ROCs defined" << endl;
  int tbmData = 0;
  if( nrocs > 1 )
    tbmData = 2; // header and trailer words

  uint32_t words_remaining = 0;
  vector < uint16_t > data;

  tb.Daq_Read( data, Blocksize, words_remaining );

  int size = data.size(  );

  cout << "words read: " << size << " = " << ( size - nrocs -
                                               tbmData ) /
    2 << " pixels" << endl;
  cout << "words remaining in DTB memory: " << words_remaining << endl;

  for( int i = 0; i < size; ++i ) {
    printf( " %4X", data[i] );
    Log.printf( " %4X", data[i] );
    if( i % 20 == 19 ) {
      printf( "\n" );
      Log.printf( "\n" );
    }
  }
  printf( "\n" );
  Log.printf( "\n" );
  Log.flush(  );

  cout << "PixelAddressInverted " << tb.GetPixelAddressInverted(  ) << endl;

 //decode:

  Decoder dec;

  uint32_t npx = 0;
  uint32_t nrr = 0;
  uint32_t ymax = 0;
  uint32_t nn[52][80] = { {0}
  };

  bool ldb = 0;

  if( h21 )
    delete h21;
  h21 = new TH2D( "HitMap",
                  "Hit map;col;row;hits", 52, -0.5, 51.5, 80, -0.5, 79.5 );

  bool even = 1;

  for( unsigned int i = 0; i < data.size(  ); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
      if( ldb )
        printf( "%X\n", data[i] );
      even = 0;
    }
    else if( even ) { // merge 2 words into one pixel int:

      if( i < data.size(  ) - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        dec.Translate( raw );

        uint16_t ix = dec.GetX(  );
        uint16_t iy = dec.GetY(  );
        uint16_t ph = dec.GetPH(  );
        double vc = ph;
        if( ldb )
          vc = PHtoVcal( ph, ix, iy );
        if( ldb )
          cout << setw( 3 ) << ix << setw( 3 ) << iy << setw( 4 ) << ph <<
            ",";
        if( ldb )
          cout << "(" << vc << ")";
        if( npx % 8 == 7 && ldb )
          cout << endl;
        if( ix < 52 && iy < 80 ) {
          nn[ix][iy]++; // hit map
          if( iy > ymax )
            ymax = iy;
          h21->Fill( ix, iy );
        }
        else
          nrr++; // error
        npx++;
      }
    }
    even = 1 - even;
  } // data
  if( ldb )
    cout << endl;

  cout << npx << " pixels" << endl;
  cout << nrr << " address errors" << endl;

  h21->Write(  );
  gStyle->SetOptStat( 10 ); // entries
  gStyle->SetStatY( 0.95 );
  h21->GetYaxis(  )->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update(  );
  cout << "histos 21" << endl;

 // hit map:
 /*
    if( npx > 0 ) {
    if( ymax < 79 ) ymax++; // print one empty
    for( int row = ymax; row >= 0; --row ) {
    cout << setw(2) << row << " ";
    for( int col = 0; col < 52; ++col )
    if( nn[col][row] )
    cout << "x";
    else
    cout << " ";
    cout << "|" << endl;
    }
    }
  */
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dreadm ) // module
{
  int channel;
  if( !PAR_IS_INT( channel, 0, 1 ) )
    channel = 0;

  uint32_t words_remaining = 0;
  vector < uint16_t > data;

  tb.Daq_Read( data, Blocksize, words_remaining, channel );

  int size = data.size(  );
  printf( "#samples: %i  remaining: %u\n", size, words_remaining );

  unsigned int hdr = 0, trl = 0;
  unsigned int raw = 0;

  // int TBM_eventnr,TBM_stackinfo,ColAddr,RowAddr,PulseHeight,TBM_trailerBits,TBM_readbackData;

  for( int i = 0; i < size; ++i ) {

    int d = data[i] & 0x0fff; // 12 data bits

    uint16_t v = ( data[i] >> 12 ) & 0xe; // e = 14 = 1110, flag

    switch ( v ) {

    case 10:
      printf( "\nTBM H12(%03X)", d );
      hdr = d;
      break;
    case 8:
      printf( "+H34(%03X) =", d );
      hdr = ( hdr << 8 ) + d;
      DecodeTbmHeader( hdr );
      break;

    case 4:
      printf( "\nROC-HEADER(%03X): ", d );
      break;

    case 0:
      printf( "\nR123(%03X)", d );
      raw = d;
      break;
    case 2:
      printf( "+R456(%03X)", d );
      raw = ( raw << 12 ) + d;
      DecodePixel( raw );
      break;

    case 14:
      printf( "\nTBM T12(%03X)", d );
      trl = d;
      break;
    case 12:
      printf( "+T34(%03X) =", d );
      trl = ( trl << 8 ) + d;
      DecodeTbmTrailer( trl );
      break;

    default:
      printf( "\nunknown data: %X = %d", v, v );
      break;
    }
  }

  for( int i = 0; i < size; ++i ) {
    int x = data[i] & 0xffff;
    Log.printf( " %04X", x );
    if( i % 100 == 99 )
      printf( "\n" );
  }
  printf( "\n" );
  Log.printf( "\n" );
  Log.flush(  );

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( takedata ) // takedata period (ROC, trigger f = 40 MHz / period)
// source:
// allon
// stretch 1 8 999
// takedata 10000
//
// noise: dac 12 122; allon; takedata 10000
//
// pulses: arm 11 0:33; takedata 999 (few err, rate not uniform)
// arm 0:51 11 = 52 pix = 312 BC = 7.8 us
// readout not synchronized with trigger?
{
  int period;
  PAR_INT( period, 1, 65500 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  uint32_t remaining;
  vector < uint16_t > data;

  unsigned int nev = 0;
 //unsigned int pev = 0; // previous nev
  unsigned int ndq = 0;
  unsigned int got = 0;
  unsigned int rst = 0;
  unsigned int npx = 0;
  unsigned int nrr = 0;

  bool ldb = 0;

  Decoder dec;

  uint32_t NN[52][80] = { {0} };
 //uint32_t PN[52][80] = {{0}}; // previous NN
  uint32_t PH[256] = { 0 };

  double duration = 0;
  double tprev = 0;

  if( h10 )
    delete h10;
  h10 = new
    TH1D( "pixels",
          "pixels per trigger;multiplicity [pixel];triggers",
          101, -0.5, 100.5 );

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "pixelPH", "pixel PH;pixel PH [ADC];pixels",
	  255, -0.5, 254.5 ); // 255 is overflow

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "pixel_charge",
          dacval[0][CtrlReg] == 0 ?
          "pixel charge;pixel charge [small Vcal DAC];pixels" :
          "pixel charge;pixel charge [large Vcal DAC];pixels",
          256, -0.5, 255.5 );

  if( h21 )
    delete h21;
  h21 = new TH2D( "HitMap",
                  "Hit map;col;row;hits", 52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h22 )
    delete h22;
  h22 = new TProfile2D( "PHMap",
                        dacval[0][CtrlReg] == 0 ?
                        "PH map;col;row;<PH> [small Vcal DAC]" :
                        "PH map;col;row;<PH> [large Vcal DAC]",
                        52, -0.5, 51.5, 80, -0.5, 79.5, 0, 500 );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );
  tb.Daq_Start(  );
  tb.uDelay( 100 );
  tb.Pg_Loop( period );

  tb.SetTimeout( 2000 ); // [ms] USB
  tb.Flush(  );

  while( !keypressed(  ) ) {

    tb.uDelay( 1000 ); // limit daq rate

    tb.Pg_Stop(  ); // stop triggers, necessary for clean data

    tb.uDelay( 100 );

    tb.Daq_Stop(  );
    tb.uDelay( 100 );
   //tb.uDelay(4000); // better
   //tb.uDelay(9000); // some overflow events (rest)

    tb.Daq_Read( data, Blocksize, remaining );

    ndq++;

   //tb.uDelay(1000);
    tb.Daq_Start(  );
    tb.uDelay( 100 );
    tb.Pg_Loop( period ); // start triggers
    tb.Flush(  );

    got += data.size(  );
    rst += remaining;

    gettimeofday( &tv, NULL );
    long s9 = tv.tv_sec;        // seconds since 1.1.1970
    long u9 = tv.tv_usec;       // microseconds
    duration = s9 - s0 + ( u9 - u0 ) * 1e-6;

    if( duration - tprev > 0.999 ) {

      cout << duration << " s, last " << data.size(  )
        << ",  rest " << remaining
        << ",  calls " << ndq
        << ",  events " << nev
        << ",  got " << got
        << ",  rest " << rst << ",  pix " << npx << ",  err " << nrr << endl;

      h21->Draw( "colz" );
      c1->Update(  );

      tprev = duration;

    }

   // decode data:

    int npxev = 0;
    bool even = 1;

    for( unsigned int i = 0; i < data.size(  ); ++i ) {

      if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
        if( ldb && i > 0 )
          cout << endl;
        if( ldb )
          printf( "%X", data[i] );
        even = 0;
        npxev = 0;
        nev++;
      }
      else if( even ) { // 24 data bits come in 2 words

        bool ldb1 = 0;

        if( i < data.size(  ) - 1 ) {

          unsigned long raw = ( data[i] & 0xfff ) << 12;
          raw += data[i + 1] & 0xfff; // even + odd

          dec.Translate( raw );

          uint16_t ix = dec.GetX(  );
          uint16_t iy = dec.GetY(  );
          uint16_t ph = dec.GetPH(  );
          double vc = PHtoVcal( ph, ix, iy );
          if( ldb )
            cout << " " << ix << "." << iy
              << ":" << ph << "(" << ( int ) vc << ")";
          if( ldb1 )
            cout << " " << ix << "." << iy
              << ":" << ph << "(" << ( int ) vc << ")";
          h11->Fill( ph );
          h12->Fill( vc );
          h21->Fill( ix, iy );
          h22->Fill( ix, iy, vc );
          if( ix < 52 && iy < 80 ) {
            NN[ix][iy]++; // hit map
            if( ph > 0 && ph < 256 )
              PH[ph]++;
          }
          else
            nrr++;
          npx++;
          npxev++;
        }
        else {
          nrr++;
          if( ldb1 )
            cout << " err at " << i << " in " << data.size(  );
        }

        if( ldb1 )
          cout << endl;

      } // even

      even = 1 - even;

      if( ( data[i] & 0x4000 ) == 0x4000 ) { // FPGA end marker
        h10->Fill( npxev );
      }

    } // data

    if( ldb )
      cout << endl;

   // check for ineff in armed pixels:
   /*
      int dev = nev - pev;
      for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {
      if( NN[col][row] < nev/2 ) continue; // not armed or noise
      int dn = NN[col][row] - PN[col][row];
      if( dn != dev )
      cout << dev << " events, pix " << setw(2) << col << setw(3) << row
      << " got " << dn << " hits" << endl;
      PN[col][row] = NN[col][row];
      }
      pev = nev;
    */
  } // while takedata

  tb.Pg_Stop(  ); // stop triggers, necessary for clean re-start
#ifdef DAQOPENCLOSE
  tb.Daq_Stop(  );
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
#endif
  tb.Flush(  );

  uint32_t nmx = 0;
  if( npx > 0 ) {
    for( int row = 79; row >= 0; --row ) {
      cout << setw( 2 ) << row << ": ";
      for( int col = 0; col < 52; ++col ) {
        if( NN[col][row] > nev / 2 )
          cout << " " << NN[col][row];
       //cout << " " << 100.0*NN[col][row]/(double)nev;
        else
          cout << " " << NN[col][row];
        if( NN[col][row] > nmx )
          nmx = NN[col][row];
      }
      cout << endl;
    }
  }
  cout << "sum " << npx << ", max " << nmx << endl;

  h10->Write(  );
  h11->Write(  );
  h12->Write(  );
  h21->Write(  );
  h22->Write(  );
  gStyle->SetOptStat( 10 ); // entries
  gStyle->SetStatY( 0.95 );
  h21->GetYaxis(  )->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update(  );
  cout << "histos 10, 11, 12, 21, 22" << endl;

  cout << endl;
  cout << "duration    " << duration << endl;
  cout << "events      " << nev << " = " << nev / duration << " Hz" << endl;
  cout << "DAQ calls   " << ndq << endl;
  cout << "words read  " << got << ", remaining " << rst << endl;
  cout << "data rate   " << 2 * got / duration << " bytes/s" << endl;
  cout << "pixels      " << npx << " = " << npx /
    ( double ) nev << "/ev " << endl;
  cout << "data errors " << nrr << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( onevst ) // pulse one pixel vs time
{
  int32_t col;
  int32_t row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  int period;
  PAR_INT( period, 1, 65500 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  uint32_t remaining;
  vector < uint16_t > data;

  unsigned int nev = 0;
 //unsigned int pev = 0; // previous nev
  unsigned int ndq = 0;
  unsigned int got = 0;
  unsigned int rst = 0;
  unsigned int npx = 0;
  unsigned int nrr = 0;

  bool ldb = 0;

  Decoder dec;

  double duration = 0;
  double tprev = 0;

  if( h11 )
    delete h11;
  h11 = new
    TProfile( "PH_vs_time",
              "PH vs time;time [s];<PH> [ADC]", 300, 0, 300, 0, 255 );

  tb.roc_Col_Enable( col, true );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );
  tb.Daq_Start(  );
  tb.uDelay( 100 );
  tb.Pg_Loop( period );

  tb.SetTimeout( 2000 ); // [ms] USB
  tb.Flush(  );

  int nsum = 0;
  double phsum = 0;

  while( !keypressed(  ) ) {

    tb.uDelay( 1000 ); // limit daq rate

    tb.Pg_Stop(  ); // stop triggers, necessary for clean data

    tb.uDelay( 100 );

    tb.Daq_Stop(  );
    tb.uDelay( 100 );

    tb.Daq_Read( data, Blocksize, remaining );

    ndq++;

   //tb.uDelay(1000);
    tb.Daq_Start(  );
    tb.uDelay( 100 );
    tb.Pg_Loop( period ); // start triggers
    tb.Flush(  );

    got += data.size(  );
    rst += remaining;

    gettimeofday( &tv, NULL );
    long s9 = tv.tv_sec;        // seconds since 1.1.1970
    long u9 = tv.tv_usec;       // microseconds
    duration = s9 - s0 + ( u9 - u0 ) * 1e-6;

    if( duration - tprev > 0.999 ) {

      double phavg = 0;
      if( nsum > 0 )
        phavg = phsum / nsum;

      cout << duration << " s, last " << data.size(  )
        << ",  rest " << remaining
        << ",  calls " << ndq
        << ",  events " << nev
        << ",  got " << got
        << ",  rest " << rst << ",  pix " << npx << ",  ph " << phavg << endl;

      h11->Draw(  );
      c1->Update(  );

      nsum = 0;
      phsum = 0;

      tprev = duration;

    }

   // decode data:

    bool even = 1;

    for( unsigned int i = 0; i < data.size(  ); ++i ) {

      if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
        if( ldb && i > 0 )
          cout << endl;
        if( ldb )
          printf( "%X", data[i] );
        even = 0;
        nev++;
      }
      else if( even ) { // 24 data bits come in 2 words

        bool ldb1 = 0;

        if( i < data.size(  ) - 1 ) {

          unsigned long raw = ( data[i] & 0xfff ) << 12;
          raw += data[i + 1] & 0xfff; // even + odd

          dec.Translate( raw );

          uint16_t ix = dec.GetX(  );
          uint16_t iy = dec.GetY(  );
          uint16_t ph = dec.GetPH(  );
          double vc = PHtoVcal( ph, ix, iy );
          if( ldb )
            cout << " " << ix << "." << iy
              << ":" << ph << "(" << ( int ) vc << ")";
          if( ldb1 )
            cout << " " << ix << "." << iy
              << ":" << ph << "(" << ( int ) vc << ")";
          if( ix == col && iy == row ) {
            h11->Fill( duration, ph );
            nsum++;
            phsum += ph;
          }
          else
            nrr++;
          npx++;
        }
        else {
          nrr++;
          if( ldb1 )
            cout << " err at " << i << " in " << data.size(  );
        }

        if( ldb1 )
          cout << endl;

      } // even

      even = 1 - even;

    } // data

    if( ldb )
      cout << endl;

  } // while takedata

  tb.Pg_Stop(  ); // stop triggers, necessary for clean re-start
#ifdef DAQOPENCLOSE
  tb.Daq_Stop(  );
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
#endif

  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal(  );
  tb.Flush(  );

  h11->Write(  );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11" << endl;

  cout << endl;
  cout << "duration    " << duration << endl;
  cout << "events      " << nev << " = " << nev / duration << " Hz" << endl;
  cout << "DAQ calls   " << ndq << endl;
  cout << "words read  " << got << ", remaining " << rst << endl;
  cout << "data rate   " << 2 * got / duration << " bytes/s" << endl;
  cout << "pixels      " << npx << " = " << npx /
    ( double ) nev << "/ev " << endl;
  cout << "data errors " << nrr << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( modtd ) // module take data (trigger f = 40 MHz / period)
// pulses:
//   modcaldel 44 66
//   arm 44 66
//   modtd 4000
// source or X-rays:
//   stretch does not work with modules
//   sm (caldel not needed for random triggers)
//   tdx (contains allon, modtd 4000)
{
  int period;
  PAR_INT( period, 1, 65500 ); // f = 40 MHz / period

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  uint32_t remaining0 = 0;
  uint32_t remaining1 = 0;
  vector < uint16_t > data0;
  vector < uint16_t > data1;
  data0.reserve( Blocksize );
  data1.reserve( Blocksize );

  unsigned int nev[2] = { 0 };
 //unsigned int pev = 0; // previous nev
  unsigned int ndq = 0;
  unsigned int got = 0;
  unsigned int rst = 0;
  unsigned int npx = 0;
  unsigned int nrr = 0;
  unsigned int nth[2] = { 0 }; // TBM headers
  unsigned int nrh[2] = { 0 }; // ROC headers
  unsigned int ntt[2] = { 0 }; // TBM trailers

  int PX[16] = { 0 };
  uint32_t NN[16][52][80] = { {{0}} };
 //uint32_t PN[16][52][80] = {{{0}}}; // previous NN
  uint32_t PH[16][52][80] = { {{0}} };

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "pixels",
          "pixels per trigger;pixel hits;triggers", 65, -0.5, 64.5 );

  if( h21 )
    delete h21;
 //int n = 4; // fat pixels
 //int n = 2; // big pixels
  int n = 1;                    // pixels
  h21 = new TH2D( "ModuleHitMap",
                  "Module hit map;col;row;hits",
                  8 * 52 / n, -0.5 * n, 8 * 52 - 0.5 * n,
                  2 * 80 / n, -0.5 * n, 2 * 80 - 0.5 * n );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis(  )->SetTitleOffset( 1.3 );

  if( h22 )
    delete h22;
  h22 = new TH2D( "ModulePHmap",
                  "Module PH map;col;row;sum PH [ADC]",
                  8 * 52 / n, -0.5 * n, 8 * 52 - 0.5 * n,
                  2 * 80 / n, -0.5 * n, 2 * 80 - 0.5 * n );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif

  tb.Daq_Select_Deser400(  );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  tb.Daq_Start( 0 );
  tb.Daq_Start( 1 );
  tb.uDelay( 1000 );

  tb.Flush(  );

  uint32_t trl = 0;             // need to remember from previous daq_read
  int nsec = 0;

  while( !keypressed(  ) ) {

    //tb.uDelay( 50000 ); // [us] limit daq rate, more efficient
    //tb.uDelay(  5000 ); // 2 kHz Sr 90 no Makrolon
    //tb.uDelay( 10000 ); // 4 kHz Sr 90 no Makrolon
    //tb.uDelay( 20000 ); // 5.1 kHz Sr 90 no Makrolon
    tb.uDelay( 40000 ); // 5.6 kHz Sr 90 no Makrolon

    tb.Pg_Stop(  ); // stop triggers, necessary for clean data

    tb.Daq_Stop( 0 );
    tb.Daq_Stop( 1 );

    tb.Daq_Read( data0, Blocksize, remaining0, 0 );
    tb.Daq_Read( data1, Blocksize, remaining1, 1 );

    ndq++;
    got += data0.size(  );
    rst += remaining0;
    got += data1.size(  );
    rst += remaining1;

    tb.Daq_Deser400_Reset( 3 ); // more stable ?

    tb.Daq_Start( 0 );
    tb.Daq_Start( 1 );

    tb.Pg_Loop( period ); // start triggers
    tb.Flush(  );

    bool ldb = 0;
   //if( ndq == 1 ) ldb = 1;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec;        // seconds since 1.1.1970
    long u2 = tv.tv_usec;       // microseconds
    double dt = s2 - s0 + ( u2 - u0 ) * 1e-6;

    if( ldb || int ( dt ) > nsec ) {

      h21->Draw( "colz" );
      c1->Update(  );

      cout << s2 - s0 + ( u2 - u0 ) * 1e-6
        << " s, last " << data0.size(  ) + data1.size(  )
        << ",  rest " << remaining0 + remaining1
        << ",  calls " << ndq << ",  events " << nev[0]
        << ",  got " << got
        << ",  rest " << rst << ",  pix " << npx << ",  err " << nrr << endl;
      nsec = int ( dt );
    }

   // decode data:

    for( int ch = 0; ch < 2; ++ch ) {

      int size = data0.size(  );
      if( ch == 1 )
        size = data1.size(  );
      uint32_t raw = 0;
      uint32_t hdr = 0;
      int32_t kroc = -1;        // will start at 0
      if( ch == 1 )
        kroc = 7; // will start at 8
      unsigned int npxev = 0;

      for( int ii = 0; ii < size; ++ii ) {

        int d = data0[ii] & 0xfff; // 12 bits data
        int v = ( data0[ii] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110
        if( ch == 1 ) {
          d = data1[ii] & 0xfff; // 12 bits data
          v = ( data1[ii] >> 12 ) & 0xe; // 3 flag bits
        }
        uint32_t ph = 0;
        int c = 0;
        int r = 0;
        int x = 0;
        int y = 0;

        switch ( v ) {

         // TBM header:
        case 10:
          hdr = d;
          nth[ch]++;
          npxev = 0;
          break;
        case 8:
          hdr = ( hdr << 8 ) + d;
          if( ldb ) {
            cout << "event " << nev[ch] << endl;
            cout << "TBM header";
            DecodeTbmHeader( hdr );
            cout << endl;
          }
          kroc = -1; // will start at 0
          if( ch == 1 )
            kroc = 7; // will start at 8
         /*
            if( nev[ch] > 0 && trl == 0 )
            cout << "TBM error: header without previous trailer in event "
            << nev[ch]
            << ", channel " << ch
            << endl;
          */
          trl = 0;
          break;

         // ROC header data:
        case 4:
          kroc++; // start at 0
          nrh[ch]++;
          if( ldb ) {
            if( kroc > 0 )
              cout << endl;
            cout << "ROC " << setw( 2 ) << kroc;
          }
          if( kroc > 15 ) {
            cout << "Error kroc " << kroc << endl;
            kroc = 15;
          }
          if( kroc == 0 && hdr == 0 )
            cout << "TBM error: no header " << nev[ch] << endl;
          hdr = 0;
          break;

         // pixel data:
        case 0:
          raw = d;
          break;
        case 2:
          raw = ( raw << 12 ) + d;
          npx++;
          npxev++;
          PX[kroc]++;
         //DecodePixel(raw);
          ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
          raw >>= 9;
          c = ( raw >> 12 ) & 7;
          c = c * 6 + ( ( raw >> 9 ) & 7 );
          r = ( raw >> 6 ) & 7;
          r = r * 6 + ( ( raw >> 3 ) & 7 );
          r = r * 6 + ( raw & 7 );
          y = 80 - r / 2;
          x = 2 * c + ( r & 1 );
          if( ldb )
            cout << " " << x << "." << y << ":" << ph;
          if( y > -1 && y < 80 && x > -1 && x < 52 ) {
            NN[kroc][x][y] += 1;
            PH[kroc][x][y] += ph;
            int l = kroc % 8;   // 0..7
            int m = kroc / 8;   // 0..1
            int xm = 52 * l + x; // 0..415  rocs 0 1 2 3 4 5 6 7
            if( m == 1 )
              xm = 415 - xm; // rocs 8 9 A B C D E F
            int ym = 159 * m + ( 1 - 2 * m ) * y; // 0..159
            h21->Fill( xm, ym );
            h22->Fill( xm, ym, ph );
          }
          break;

         // TBM trailer:
        case 14:
          trl = d;
          ntt[ch]++;
          h11->Fill( npxev );
          break;
        case 12:
          trl = ( trl << 8 ) + d;
          if( ldb ) {
            cout << endl;
            cout << "TBM trailer";
            DecodeTbmTrailer( trl );
            cout << endl;
          }
          nev[ch]++;
          break;

        default:
          printf( "\nunknown data: %X = %d", v, v );
          break;

        } // switch

      } // data

    } // ch

    if( ldb )
      cout << endl;

   // check for ineff in armed pixels:
   /*
      int dev = nev - pev;
      for( int roc = 0; roc < 16; ++roc )
      for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {
      if( NN[roc][col][row] < nev/2 ) continue; // not armed or noise
      int dn = NN[roc][col][row] - PN[roc][col][row];
      if( ldb && dn != dev )
      cout << dev << " events"
      << " ROC "
      << setw(2) << roc
      << " pix "
      << setw(2) << col
      << setw(3) << row
      << " hits " << dn << endl;
      PN[roc][col][row] = NN[roc][col][row];
      }

      pev = nev;
    */
  } // while

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  double dt = s9 - s0 + ( u9 - u0 ) * 1e-6;

  tb.Pg_Stop(  ); // stop triggers, necessary for clean re-start
 //tb.Daq_Close();
 //tb.Daq_DeselectAll();
  tb.Flush(  );

  h11->Write(  );
  h21->Write(  );
  h22->Write(  );
  h21->Draw( "colz" );
  c1->Update(  );
  cout << "histos 11, 21, 22" << endl;

  cout << endl;
  for( int roc = 0; roc < 16; ++roc )
    cout << "ROC " << setw( 2 ) << roc << ", hits " << PX[roc]
      << endl;

  cout << endl;
  cout << "run duration   " << dt << endl;
  cout << "DAQ calls      " << ndq << endl;

  cout << "events ch 0    " << nev[0] << endl;
  cout << "TBM headers 0  " << nth[0] << endl;
  cout << "TBM trailers 0 " << ntt[0] << endl;
  cout << "ROC headers 0  " << nrh[0] << endl;

  cout << "events ch 1    " << nev[1] << endl;
  cout << "TBM headers 1  " << nth[1] << endl;
  cout << "TBM trailers 1 " << ntt[1] << endl;
  cout << "ROC headers 1  " << nrh[1] << endl;

  cout << "words read     " << got << endl;
  cout << "data rate      " << 2 * got / dt / 1024 / 1024 << " MiB/s" << endl;
  cout << "pixels         " << npx
    << " = " << ( double ) npx / nev[0] << " / event"
    << " = " << ( double ) npx / nev[0] / 16 << " / event / ROC" << endl;
  cout << "address errors " << nrr << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( showclk )
{
  const unsigned int nSamples = 20;
  const int gain = 1;
  // PAR_INT( gain, 1, 4 );

  unsigned int i, k;
  vector < uint16_t > data[20];

  tb.Pg_Stop(  );
  tb.Pg_SetCmd( 0, PG_SYNC + 5 );
  tb.Pg_SetCmd( 1, PG_CAL + 6 );
  tb.Pg_SetCmd( 2, PG_TRG + 6 );
  tb.Pg_SetCmd( 3, PG_RESR + 6 );
  tb.Pg_SetCmd( 4, PG_REST + 6 );
  tb.Pg_SetCmd( 5, PG_TOK );

  tb.SignalProbeD1( 9 );
  tb.SignalProbeD2( 17 );
  tb.SignalProbeA2( PROBEA_CLK );
  tb.uDelay( 10 );
  tb.SignalProbeADC( PROBEA_CLK, gain - 1 );
  tb.uDelay( 10 );

  tb.Daq_Select_ADC( nSamples, 1, 1 );
  tb.uDelay( 1000 );
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  for( i = 0; i < 20; ++i ) {
    tb.Sig_SetDelay( SIG_CLK, 26 - i );
    tb.uDelay( 10 );
    tb.Daq_Start(  );
    tb.Pg_Single(  );
    tb.uDelay( 1000 );
   //tb.Daq_Stop();
    tb.Daq_Read( data[i], 1024 );
    if( data[i].size(  ) != nSamples ) {
      printf( "Data size %i: %i\n", i, int ( data[i].size(  ) ) );
      return true;
    }
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
#endif
  tb.Flush(  );

  int n = 20 * nSamples;
  vector < double >values( n );
  int x = 0;
  for( k = 0; k < nSamples; ++k )
    for( i = 0; i < 20; ++i ) {
      int y = ( data[i] )[k] & 0x0fff;
      if( y & 0x0800 )
        y |= 0xfffff000;
      values[x++] = y;
    }
 //Scope( "CLK", values);

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( showctr )
{
  const unsigned int nSamples = 60;
  const int gain = 1;
  // PAR_INT( gain, 1, 4 );

  unsigned int i, k;
  vector < uint16_t > data[20];

  tb.Pg_Stop(  );
  tb.Pg_SetCmd( 0, PG_SYNC + 5 );
  tb.Pg_SetCmd( 1, PG_CAL + 6 );
  tb.Pg_SetCmd( 2, PG_TRG + 6 );
  tb.Pg_SetCmd( 3, PG_RESR + 6 );
  tb.Pg_SetCmd( 4, PG_REST + 6 );
  tb.Pg_SetCmd( 5, PG_TOK );

  tb.SignalProbeD1( 9 );
  tb.SignalProbeD2( 17 );
  tb.SignalProbeA2( PROBEA_CTR );
  tb.uDelay( 10 );
  tb.SignalProbeADC( PROBEA_CTR, gain - 1 );
  tb.uDelay( 10 );

  tb.Daq_Select_ADC( nSamples, 1, 1 );
  tb.uDelay( 1000 );
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  for( i = 0; i < 20; ++i ) {
    tb.Sig_SetDelay( SIG_CTR, 26 - i );
    tb.uDelay( 10 );
    tb.Daq_Start(  );
    tb.Pg_Single(  );
    tb.uDelay( 1000 );
   //tb.Daq_Stop();
    tb.Daq_Read( data[i], 1024 );
    if( data[i].size(  ) != nSamples ) {
      printf( "Data size %i: %i\n", i, int ( data[i].size(  ) ) );
      return true;
    }
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
#endif
  tb.Flush(  );

  int n = 20 * nSamples;
  vector < double >values( n );
  int x = 0;
  for( k = 0; k < nSamples; ++k )
    for( i = 0; i < 20; ++i ) {
      int y = ( data[i] )[k] & 0x0fff;
      if( y & 0x0800 )
        y |= 0xfffff000;
      values[x++] = y;
    }
 //Scope( "CTR", values);

 /*
    FILE *f = fopen( "X:\\developments\\adc\\data\\adc.txt", "wt" );
    if( !f) { printf( "Could not open File!\n" ); return true; }
    double t = 0.0;
    for( k=0; k<100; ++k) for( i=0; i<20; ++i)
    {
    int x = (data[i])[k] & 0x0fff;
    if( x & 0x0800) x |= 0xfffff000;
    fprintf(f, "%7.2f %6i\n", t, x);
    t += 1.25;
    }
    fclose(f);
  */
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( showsda )
{
  const unsigned int nSamples = 52;
  unsigned int i, k;
  vector < uint16_t > data[20];

  tb.SignalProbeD1( 9 );
  tb.SignalProbeD2( 17 );
  tb.SignalProbeA2( PROBEA_SDA );
  tb.uDelay( 10 );
  tb.SignalProbeADC( PROBEA_SDA, 0 );
  tb.uDelay( 10 );

  tb.Daq_Select_ADC( nSamples, 2, 7 );
  tb.uDelay( 1000 );
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  for( i = 0; i < 20; ++i ) {
    tb.Sig_SetDelay( SIG_SDA, 26 - i );
    tb.uDelay( 10 );
    tb.Daq_Start(  );
    tb.roc_Pix_Trim( 12, 34, 5 );
    tb.uDelay( 1000 );
   //tb.Daq_Stop();
    tb.Daq_Read( data[i], 1024 );
    if( data[i].size(  ) != nSamples ) {
      printf( "Data size %i: %i\n", i, int ( data[i].size(  ) ) );
      return true;
    }
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
#endif
  tb.Flush(  );

  int n = 20 * nSamples;
  vector < double >values( n );
  int x = 0;
  for( k = 0; k < nSamples; ++k )
    for( i = 0; i < 20; ++i ) {
      int y = ( data[i] )[k] & 0x0fff;
      if( y & 0x0800 )
        y |= 0xfffff000;
      values[x++] = y;
    }
 //Scope( "SDA", values);

  return true;
}

//------------------------------------------------------------------------------
void decoding_show2( vector < uint16_t > &data ) // bit-wise print
{
  int size = data.size(  );
  if( size > 6 )
    size = 6;
  for( int i = 0; i < size; ++i ) {
    uint16_t x = data[i];
    for( int k = 0; k < 12; ++k ) { // 12 valid bits per word
      Log.puts( ( x & 0x0800 ) ? "1" : "0" );
      x <<= 1;
      if( k == 3 || k == 7 || k == 11 )
        Log.puts( " " );
    }
  }
}

//------------------------------------------------------------------------------
void decoding_show( vector < uint16_t > *data )
{
  int i;
  for( i = 1; i < 16; ++i )
    if( data[i] != data[0] )
      break;
  decoding_show2( data[0] );
  Log.puts( "  " );
  if( i < 16 )
    decoding_show2( data[i] );
  else
    Log.puts( " no diff" );
  Log.puts( "\n" );
}

//------------------------------------------------------------------------------
CMD_PROC( decoding )
{
  unsigned short t;
  vector < uint16_t > data[16];
  tb.Pg_SetCmd( 0, PG_TOK + 0 );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 10 );
  Log.section( "decoding" );
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  for( t = 0; t < 44; t++ ) {
    tb.Sig_SetDelay( SIG_CLK, t );
    tb.Sig_SetDelay( SIG_TIN, t + 5 );
    tb.uDelay( 10 );
    for( int i = 0; i < 16; ++i ) {
      tb.Daq_Start(  );
      tb.Pg_Single(  );
      tb.uDelay( 200 );
     //tb.Daq_Stop();
      tb.Daq_Read( data[i], 200 );
    }
    Log.printf( "%3i ", int ( t ) );
    decoding_show( data );
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
#endif
  Log.flush(  );

  return true;
}

// -- Wafer Test Adapter commands ----------------------------------------
/*
  CMD_PROC(vdreg)    // regulated VD
  {
  double v = tb.GetVD_Reg();
  printf( "\n VD_reg = %1.3fV\n", v);
  return true;
  }

  CMD_PROC(vdcap)    // unregulated VD for contact test
  {
  double v = tb.GetVD_CAP();
  printf( "\n VD_cap = %1.3fV\n", v);
  return true;
  }

  CMD_PROC(vdac)     // regulated VDAC
  {
  double v = tb.GetVDAC_CAP();
  printf( "\n V_dac = %1.3fV\n", v);
  return true;
  }
*/

//------------------------------------------------------------------------------
CMD_PROC( tbmdis )
{
  tb.tbm_Enable( false );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tbmsel )
{
  int hub, port;
  PAR_INT( hub, 0, 31 );
  PAR_INT( port, 0, 6 ); // port 0 = ROC 0-3, 1 = 4-7, 2 = 8-11, 3 = 12-15, 6 = 0-15
  tb.tbm_Enable( true );
  tb.tbm_Addr( hub, port );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( modsel )
{
  int hub;
  PAR_INT( hub, 0, 31 );
  tb.tbm_Enable( true );
  tb.mod_Addr( hub );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tbmset )
{
  int reg, value;
  PAR_INT( reg, 0, 255 );
  PAR_INT( value, 0, 255 );
  tb.tbm_Set( reg, value );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( select ) // define active ROCs
{
  int rocmin, rocmax;
  PAR_RANGE( rocmin, rocmax, 0, 15 );

  int i;
  for( i = 0; i < rocmin; ++i )
    roclist[i] = 0;
  for( ; i <= rocmax; ++i )
    roclist[i] = 1;
  for( ; i < 16; ++i )
    roclist[i] = 0;

  tb.Daq_Deser400_OldFormat( true ); // 3.0 for TBM08

  vector < uint8_t > trimvalues( 4160 ); // uint8 in 2.15
  for( int i = 0; i < 4160; ++i )
    trimvalues[i] = 15; // 15 = no trim

 // set up list of ROCs in FPGA:

  vector < uint8_t > roci2cs;
  roci2cs.reserve( 16 );
  for( uint8_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] )
      roci2cs.push_back( roc );

  tb.SetI2CAddresses( roci2cs );

  cout << "setTrimValues for ROC";
  for( uint8_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      cout << " " << ( int ) roc << flush;
      tb.roc_I2cAddr( roc );
      tb.SetTrimValues( roc, trimvalues );
    }
  cout << endl;
  tb.roc_I2cAddr( rocmin );
  DO_FLUSH;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dac ) // e.g. dac 25 222 [8]
{
  int addr, value;
  PAR_INT( addr, 1, 255 );
  PAR_INT( value, 0, 255 );

  int rocmin, rocmax;
  if( !PAR_IS_INT( rocmin, 0, 15 ) )  { // no user input
    rocmin = 0;
    rocmax = 15;
  }
  else
    rocmax = rocmin; // one ROC

  for( int i = rocmin; i <= rocmax; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_SetDAC( addr, value ); // set corrected
      dacval[i][addr] = value;
      Log.printf( "[SETDAC] %i  %i\n", addr, value );
    }

  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vana )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_SetDAC( Vana, value );
      dacval[i][Vana] = value;
      Log.printf( "[SETDAC] %i  %i\n", Vana, value );
    }

  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vtrim )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_SetDAC( Vtrim, value );
      dacval[i][Vtrim] = value;
      Log.printf( "[SETDAC] %i  %i\n", Vtrim, value );
    }

  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vthr )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_SetDAC( VthrComp, value );
      dacval[i][VthrComp] = value;
      Log.printf( "[SETDAC] %i  %i\n", VthrComp, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vcal )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_SetDAC( Vcal, value );
      dacval[i][Vcal] = value;
      Log.printf( "[SETDAC] %i  %i\n", Vcal, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( ctl )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_SetDAC( CtrlReg, value );
      dacval[i][CtrlReg] = value;
      Log.printf( "[SETDAC] %i  %i\n", CtrlReg, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( wbc )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_SetDAC( WBC, value );
      dacval[i][WBC] = value;
      Log.printf( "[SETDAC] %i  %i\n", WBC, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( optia ) // DP  optia ia [mA] for one ROC
{
  int target;
  PAR_INT( target, 10, 80 );

  Log.section( "OPTIA", false );
  Log.printf( " Ia %i mA\n", target );

  int val = dacval[0][Vana];
  double ia = tb.GetIA(  ) * 1E3; // [mA]
  double diff = target + 0.1 - ia; // besser zuviel als zuwenig

  const double slope = 6;       // 255 DACs / 40 mA
  const double eps = 0.25;

  int iter = 0;
  cout << iter << ". " << val << "  " << ia << "  " << diff << endl;

  while( fabs( diff ) > eps && iter < 11 && val > 0 && val < 255 ) {

    int stp = int ( fabs( slope * diff ) );
    if( stp == 0 )
      stp = 1;
    if( diff < 0 )
      stp = -stp;
    val += stp;
    if( val < 0 )
      val = 0;
    else if( val > 255 )
      val = 255;
    tb.SetDAC( Vana, val );
    tb.mDelay( 200 );
    ia = tb.GetIA(  ) * 1E3; // contains flush
    dacval[0][Vana] = val;
    Log.printf( "[SETDAC] %i  %i\n", Vana, val );
    diff = target + 0.1 - ia;
    iter++;
    cout << iter << ". " << val << "  " << ia << "  " << diff << endl;
  }
  Log.flush(  );
  cout << "set Vana to " << val << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( show )
{
  int rocmin, rocmax;
  if( !PAR_IS_INT( rocmin, 0, 15 ) ) { // no user input
    rocmin = 0;
    rocmax = 15;
  }
  else
    rocmax = rocmin; // one ROC

  Log.section( "DACs", true );
  for( int32_t roc = rocmin; roc <= rocmax; ++roc )
    if( roclist[roc] ) {
      cout << "ROC " << setw( 2 ) << roc << endl;
      Log.printf( "ROC %2i\n", roc );

      for( int j = 1; j < 256; ++j ) {
        if( dacval[roc][j] < 0 ) continue; // DAC not active
	cout << setw( 3 ) << j
	     << "  " << dacName[j] << "\t"
	     << setw( 5 ) << dacval[roc][j] << endl;
	Log.printf( "%3i  %s %4i\n", j, dacName[j].c_str(  ),
		    dacval[roc][j] );
      } // dac
    } // roc
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( show1 ) // print one DAC for all ROCs
{
  int dac;
  PAR_INT( dac, 1, 255 ) ;

  for( int32_t roc = 0; roc <= 15; ++roc )
    if( roclist[roc] ) {
      cout << "ROC " << setw( 2 ) << roc;
      for( int j = 1; j < 256; ++j ) {
        if( dacval[roc][j] < 0 ) continue; // DAC not active
	if( j == dac )
	  cout << "  " << dacName[j]
	       << setw( 5 ) << dacval[roc][j] << endl;
      } // dac
    } //roc
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( wdac ) // write DACs to file
{
  char cdesc[80];
  PAR_STRING( cdesc, 80 );
  string desc = cdesc;

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      nrocs++;

  ostringstream fname;          // output string stream

  if( nrocs == 1 )
    fname << "dacParameters_c" << Chip << "_" << desc.c_str(  ) << ".dat";
  else
    fname << "dacParameters_D" << Module << "_" << desc.c_str(  ) << ".dat";

  ofstream dacFile( fname.str(  ).c_str(  ) ); // love it!

  for( size_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      for( int idac = 1; idac < 256; ++idac )
        if( dacval[roc][idac] > -1 ) {
          dacFile << setw( 3 ) << idac
            << "  " << dacName[idac] << "\t"
            << setw( 5 ) << dacval[roc][idac] << endl;
        } // dac
    } // ROC
  cout << "DAC values written to " << fname.str(  ) << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rddac ) // read DACs from file
{
  char cdesc[80];
  PAR_STRING( cdesc, 80 );
  string desc = cdesc;

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      nrocs++;

  ostringstream fname;          // output string stream

  if( nrocs == 1 )
    fname << "dacParameters_c" << Chip << "_" << desc.c_str(  ) << ".dat";
  else
    fname << "dacParameters_D" << Module << "_" << desc.c_str(  ) << ".dat";

  ifstream dacFile( fname.str(  ).c_str(  ) );

  if( !dacFile ) {
    cout << "dac file " << fname.str(  ) << " does not exist" << endl;
    return 0;
  }
  cout << "read dac values from " << fname.str(  ) << endl;
  Log.printf( "[RDDAC] %s\n", fname.str(  ).c_str(  ) );

  int idac;
  string dacname;
  int vdac;
  int roc = -1;
  while( !dacFile.eof(  ) ) {
    dacFile >> idac >> dacname >> vdac;
    if( idac == 1 ) {
      roc++;
      cout << "ROC " << roc << endl;
      tb.roc_I2cAddr( roc );
    }
    if( idac < 0 )
      cout << "illegal dac number " << idac;
    else if( idac > 255 )
      cout << "illegal dac number " << idac;
    else {
      tb.roc_SetDAC( idac, vdac );
      dacval[roc][idac] = vdac;
      cout << setw( 3 ) << idac
        << "  " << dacName[idac] << "\t" << setw( 5 ) << vdac << endl;
      Log.printf( "[SETDAC] %i  %i\n", idac, vdac );
    }
  }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( wtrim ) // write trim bits to file
{
  char cdesc[80];
  PAR_STRING( cdesc, 80 );
  string desc = cdesc;

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      nrocs++;

  ostringstream fname;          // output string stream

  if( nrocs == 1 )
    fname << "trimParameters_c" << Chip << "_" << desc.c_str(  ) << ".dat";
  else
    fname << "trimParameters_D" << Module << "_" << desc.c_str(  ) << ".dat";

  ofstream trimFile( fname.str(  ).c_str(  ) );

  for( size_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      for( int col = 0; col < 52; ++col )
        for( int row = 0; row < 80; ++row ) {
          trimFile << setw( 2 ) << modtrm[roc][col][row]
            << "  Pix" << setw( 3 ) << col << setw( 3 ) << row << endl;
        }
    }
  cout << "trim values written to " << fname.str(  ) << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rdtrim ) // read trim bits from file
{
  char cdesc[80];
  PAR_STRING( cdesc, 80 );
  string desc = cdesc;

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      nrocs++;

  ostringstream fname;          // output string stream

  if( nrocs == 1 )
    fname << "trimParameters_c" << Chip << "_" << desc.c_str(  ) << ".dat";
  else
    fname << "trimParameters_D" << Module << "_" << desc.c_str(  ) << ".dat";

  ifstream trimFile( fname.str(  ).c_str(  ) );

  if( !trimFile ) {
    cout << "trim file " << fname.str(  ) << " does not exist" << endl;
    return 0;
  }
  cout << "read trim values from " << fname.str(  ) << endl;
  Log.printf( "[RDTRIM] %s\n", fname.str(  ).c_str(  ) );

  string Pix;                   // dummy
  int icol;
  int irow;
  int itrm;
  vector < uint8_t > trimvalues( 4160 );
  for( size_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      for( int col = 0; col < 52; ++col )
        for( int row = 0; row < 80; ++row ) {
          trimFile >> itrm >> Pix >> icol >> irow;
          if( icol != col )
            cout << "pixel number out of order at "
              << col << " " << row << endl;
          else if( irow != row )
            cout << "pixel number out of order at "
              << col << " " << row << endl;
          else {
            modtrm[roc][col][row] = itrm;
            int i = 80 * col + row;
            trimvalues[i] = itrm;
          }
        } // pix
      tb.roc_I2cAddr( roc );
      tb.SetTrimValues( roc, trimvalues ); // load into FPGA
      DO_FLUSH;
    } // roc

  return true;
}

//------------------------------------------------------------------------------
int32_t dacStrt( int32_t num )  // min DAC range
{
  if( num == VD )
    return 1500; // ROC looses settings at lower voltage
  else if( num == VA )
    return 500; // [mV]
  else
    return 0;
}

//------------------------------------------------------------------------------
int32_t dacStop( int32_t num )  // max DAC value
{
  if( num < 1 )
    return 0;
  else if( num > 255 )
    return 0;
  else if( num == Vdig || num == Vcomp || num == Vbias_sf )
    return 15; // 4-bit
  else if( num == CtrlReg )
    return 4;
  else if( num == VD )
    return 3000;
  else if( num == VA )
    return 2000;
  else
    return 255; // 8-bit
}

//------------------------------------------------------------------------------
int32_t dacStep( int32_t num )
{
  if( num == VD )
    return 10;
  else if( num == VA )
    return 10;
  else
    return 1; // 8-bit
}

//------------------------------------------------------------------------------
CMD_PROC( cole )
{
  int col, colmin, colmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      for( col = colmin; col <= colmax; col += 2 ) // DC is enough
        tb.roc_Col_Enable( col, 1 );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( cold )
{
  int col, colmin, colmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      for( col = colmin; col <= colmax; col += 2 )
        tb.roc_Col_Enable( col, 0 );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pixe )
{
  int32_t col, colmin, colmax;
  int32_t row, rowmin, rowmax;
  int32_t trim;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );
  PAR_INT( trim, 0, 15 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      for( col = colmin; col <= colmax; ++col )
        for( row = rowmin; row <= rowmax; ++row )
          tb.roc_Pix_Trim( col, row, trim );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pixi ) // info
{
  int32_t roc = 0;
  int32_t col;
  int32_t row;
 //PAR_INT( roc, 0, 15 );
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  if( roclist[roc] ) {
    cout << "trim bits " << setw( 2 ) << modtrm[roc][col][row]
      << ", thr " << modthr[roc][col][row] << endl;
  }
  else
    cout << "ROC not active" << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pixt ) // set trim bits
{
  int32_t roc = 0;
  int32_t col;
  int32_t row;
  int32_t trm;
 //PAR_INT( roc, 0, 15 );
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( trm, 0, 15 );

  if( roclist[roc] ) {

    modtrm[roc][col][row] = trm;

    tb.roc_I2cAddr( roc );
    vector < uint8_t > trimvalues( 4160 );

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        int i = 80 * col + row;
        trimvalues[i] = modtrm[roc][col][row];
      } // row
    tb.SetTrimValues( roc, trimvalues ); // load into FPGA
    DO_FLUSH;
  }
  else
    cout << "ROC not active" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( arm ) // DP arm 2 2 or arm 0:51 2  activate pixel for cal
{
  int32_t col, colmin, colmax;
  int32_t row, rowmin, rowmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );
  for( int32_t i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      for( col = colmin; col <= colmax; ++col ) {
        tb.roc_Col_Enable( col, 1 );
        for( row = rowmin; row <= rowmax; ++row ) {
          int trim = modtrm[i][col][row];
          tb.roc_Pix_Trim( col, row, trim );
          tb.roc_Pix_Cal( col, row, false );
        }
      }
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pixd )
{
  int32_t col, colmin, colmax;
  int32_t row, rowmin, rowmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      for( col = colmin; col <= colmax; ++col )
        for( row = rowmin; row <= rowmax; ++row )
          tb.roc_Pix_Mask( col, row );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( cal )
{
  int32_t col, colmin, colmax;
  int32_t row, rowmin, rowmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      for( col = colmin; col <= colmax; ++col )
        for( row = rowmin; row <= rowmax; ++row )
          tb.roc_Pix_Cal( col, row, false );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( cals )
{
  int32_t col, colmin, colmax;
  int32_t row, rowmin, rowmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      for( col = colmin; col <= colmax; ++col )
        for( row = rowmin; row <= rowmax; ++row )
          tb.roc_Pix_Cal( col, row, true );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( cald )
{
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_ClrCal(  );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( mask )
{
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_Chip_Mask(  );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( daci ) // currents vs dac
// noise activity peak: VthrComp
// cole 0:51
// pixe 0:51 0:79 15
// daci 12
{
  int32_t roc = 0;
 //PAR_INT( roc, 0, 15 );

  int32_t dac;
  PAR_INT( dac, 0, 32 );

  if( dacval[roc][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  tb.roc_I2cAddr( roc );

  int val = dacval[roc][dac];

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int32_t dacstep = dacStep( dac );
  int32_t nstp = ( dacstop - dacstrt ) / dacstep + 1;

  Log.section( "DACCURRENT", false );
  Log.printf( " DAC %i  %i:%i\n", dac, 0, dacstop );

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "ID_DAC%02i", dac ),
          Form( "Digital current vs %s;%s [DAC];ID [mA]",
                dacName[dac].c_str(  ), dacName[dac].c_str(  ) ), nstp,
          dacstrt - 0.5, dacstop + 0.5 );

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "IA_DAC%02i", dac ),
          Form( "Analog current vs %s;%s [DAC];IA [mA]",
                dacName[dac].c_str(  ), dacName[dac].c_str(  ) ), nstp,
          dacstrt - 0.5, dacstop + 0.5 );

  for( int32_t i = dacstrt; i <= dacstop; i += dacstep ) {

    tb.SetDAC( dac, i );
    tb.mDelay( 50 );

    double id = tb.GetID(  ) * 1E3; // [mA]
    double ia = tb.GetIA(  ) * 1E3;

    Log.printf( "%i %5.1f %5.1f\n", i, id, ia );
    printf( "%3i %5.1f %5.1f\n", i, id, ia );

    h11->Fill( i, id );
    h12->Fill( i, ia );
  }
  Log.flush(  );

  tb.SetDAC( dac, val ); // restore
  tb.Flush(  );

  h11->Write(  );
  h12->Write(  );
  h11->SetStats( 0 );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11, 12" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vthrcompi ) // Id vs VthrComp: noise peak (amplitude depends on Temp?)
// peak position depends on Vana
{
  Log.section( "Id-vs-VthrComp", false );

  int32_t dacstop = dacStop( VthrComp );
  int32_t nstp = dacstop + 1;
  vector < double >yvec( nstp, 0.0 );
  double ymax = 0;
  int32_t imax = 0;

  Log.printf( " DAC %i: %i:%i\n", VthrComp, 0, dacstop );

 // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, true );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.Flush(  );

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "ID_VthrComp",
          "Digital current vs comparator threshold;VthrComp [DAC];ID [mA]",
          256, -0.5, 255.5 );

  for( int32_t i = 0; i <= dacstop; ++i ) {

    tb.SetDAC( VthrComp, i );
    tb.mDelay( 20 );

    double id = tb.GetID(  ) * 1000.0; // [mA]

    h11->Fill( i, id );

    Log.printf( "%3i %5.1f\n", i, id );
    printf( "%i %5.1f mA\n", i, id );

    yvec[i] = id;
    if( id > ymax ) {
      ymax = id;
      imax = i;
    }
  }

 // all off:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 0 );
    for( int row = 0; row < 80; ++row ) {
      tb.roc_Pix_Mask( col, row );
    }
  }
  tb.roc_ClrCal(  );
  tb.Flush(  );

  h11->Write(  );
  h11->SetStats( 0 );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11" << endl;

  cout << "peak " << ymax << " mA" << " at " << imax << endl;

  double maxd1 = 0;
  double maxd2 = 0;
  double maxd4 = 0;
  double maxd7 = 0;
  double maxd11 = 0;
  double maxd16 = 0;
  double maxd22 = 0;
  double maxd29 = 0;
  int maxi1 = 0;
  int maxi2 = 0;
  int maxi4 = 0;
  int maxi7 = 0;
  int maxi11 = 0;
  int maxi16 = 0;
  int maxi22 = 0;
  int maxi29 = 0;
  for( int i = 0; i <= dacstop - 29; ++i ) {
    double ni = yvec[i];
    double d1 = yvec[i + 1] - ni;
    double d2 = yvec[i + 2] - ni;
    double d4 = yvec[i + 4] - ni;
    double d7 = yvec[i + 7] - ni;
    double d11 = yvec[i + 11] - ni;
    double d16 = yvec[i + 16] - ni;
    double d22 = yvec[i + 22] - ni;
    double d29 = yvec[i + 29] - ni;
    if( d1 > maxd1 ) {
      maxd1 = d1;
      maxi1 = i;
    }
    if( d2 > maxd2 ) {
      maxd2 = d2;
      maxi2 = i;
    }
    if( d4 > maxd4 ) {
      maxd4 = d4;
      maxi4 = i;
    }
    if( d7 > maxd7 ) {
      maxd7 = d7;
      maxi7 = i;
    }
    if( d11 > maxd11 ) {
      maxd11 = d11;
      maxi11 = i;
    }
    if( d16 > maxd16 ) {
      maxd16 = d16;
      maxi16 = i;
    }
    if( d22 > maxd22 ) {
      maxd22 = d22;
      maxi22 = i;
    }
    if( d29 > maxd29 ) {
      maxd29 = d29;
      maxi29 = i;
    }
  }
  cout << "max d1  " << maxd1 << " at " << maxi1 << endl;
  cout << "max d2  " << maxd2 << " at " << maxi2 << endl;
  cout << "max d4  " << maxd4 << " at " << maxi4 << endl;
  cout << "max d7  " << maxd7 << " at " << maxi7 << endl;
  cout << "max d11 " << maxd11 << " at " << maxi11 << endl;
  cout << "max d16 " << maxd16 << " at " << maxi16 << endl;
  cout << "max d22 " << maxd22 << " at " << maxi22 << endl;
  cout << "max d29 " << maxd29 << " at " << maxi29 << endl;

  int32_t val = maxi22 - 8;     // safety for digV2.1
  val = maxi29 - 30; // safety for digV2
  if( val < 0 )
    val = 0;
  tb.SetDAC( VthrComp, val );
  tb.Flush(  );
  dacval[0][VthrComp] = val;
  Log.printf( "[SETDAC] %i  %i\n", VthrComp, val );
  Log.flush(  );

  cout << "set VthrComp to " << val << endl;

  return true;
}

//------------------------------------------------------------------------------
// utility function for Pixel PH and cnt
bool GetPixData( int roc, int col, int row, int nTrig,
                 int &nReadouts, double &PHavg, double &PHrms )
{
  nReadouts = 0;
  PHavg = -1;
  PHrms = -1;

  uint16_t flags = 0;
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  uint16_t mTrig = abs( nTrig );

  tb.roc_Col_Enable( col, true );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );
  tb.Daq_Start(  );
  tb.uDelay( 100 );

  tb.LoopSingleRocOnePixelCalibrate( roc, col, row, mTrig, flags );

 //tb.Daq_Stop();

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize(  ) );

  tb.Daq_Read( data, Blocksize );

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal(  );
  tb.Flush(  );

  Decoder dec;
  bool even = 1;

  int cnt = 0;
  int phsum = 0;
  int phsu2 = 0;

  bool extra = 0;

  for( unsigned int i = 0; i < data.size(  ); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
      even = 0;
    }
    else if( even ) { // merge 2 data words into one int:

      if( i < data.size(  ) - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        dec.Translate( raw );

        uint16_t ix = dec.GetX(  );
        uint16_t iy = dec.GetY(  );
        uint16_t ph = dec.GetPH(  );

        if( ix == col && iy == row ) {
          cnt++;
          phsum += ph;
          phsu2 += ph * ph;
        }
        else {
          cout << " + " << ix << " " << iy << " " << ph;
          extra = 1;
        }
      }
    }

    even = 1 - even;

  } // data loop
  if( extra )
    cout << endl;

  nReadouts = cnt;
  if( cnt > 0 ) {
    PHavg = ( double ) phsum / cnt;
    PHrms = sqrt( ( double ) phsu2 / cnt - PHavg * PHavg );
  }

  return 1;
}

//------------------------------------------------------------------------------
CMD_PROC( fire ) // fire col row (nTrig) [send cal and read ROC]
{
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  int nTrig;
  if( !PAR_IS_INT( nTrig, -65500, 65500 ) )
    nTrig = 1;

  tb.roc_Col_Enable( col, true );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  bool cals = nTrig < 0;
  tb.roc_Pix_Cal( col, row, cals );

  cout << "fire"
    << " col " << col
    << " row " << row << " trim " << trim << " nTrig " << nTrig << endl;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );
  tb.Daq_Start(  );
  tb.uDelay( 100 );
  tb.Flush(  );

  for( int k = 0; k < abs( nTrig ); ++k ) {
    tb.Pg_Single(  );
    tb.uDelay( 20 );
  }
  tb.Flush(  );

  tb.Daq_Stop(  );
  cout << "  DAQ size " << tb.Daq_GetSize(  ) << endl;

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize(  ) );

  tb.Daq_Read( data, Blocksize );

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

 // pixel off:

  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal(  );
  tb.Flush(  );

  cout << "  data size " << data.size(  );
  for( size_t i = 0; i < data.size(  ); ++i ) {
    if( ( data[i] & 0xffc ) == 0x7f8 )
      cout << ":";
    printf( " %4X", data[i] ); // full word with FPGA markers
   //printf( " %3X", data[i] & 0xfff ); // 12 bits per word
  }
  cout << endl;

  Decoder dec;
  bool even = 1;

  int evt = 0;
  int cnt = 0;
  int phsum = 0;
  int phsu2 = 0;
  double vcsum = 0;

  bool extra = 0;

  for( unsigned int i = 0; i < data.size(  ); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // ROC header
      even = 0;
      if( evt > 0 )
        cout << endl;
      evt++;
      cout << "evt " << setw( 2 ) << evt;
    }
    else if( even ) { // merge 2 words into one pixel int:

      if( i < data.size(  ) - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        dec.Translate( raw );

        uint16_t ix = dec.GetX(  );
        uint16_t iy = dec.GetY(  );
        uint16_t ph = dec.GetPH(  );
        double vc = PHtoVcal( ph, ix, iy );

        cout << " pix " << setw( 2 ) << ix << setw( 3 ) << iy << setw( 4 ) << ph;

        if( ix == col && iy == row ) {
          cnt++;
          phsum += ph;
          phsu2 += ph * ph;
          vcsum += vc;
        }
        else {
          cout << " + " << ix << " " << iy << " " << ph;
          extra = 1;
        }
      }
    }

    even = 1 - even;

  } // data

  if( extra )
    cout << endl;

  double ph = -1;
  double rms = -1;
  double vc = -1;
  if( cnt > 0 ) {
    ph = ( double ) phsum / cnt;
    rms = sqrt( ( double ) phsu2 / cnt - ph * ph );
    vc = vcsum / cnt;
  }
  cout << "  pixel " << setw( 2 ) << col << setw( 3 ) << row
    << " responses " << cnt
    << ", PH " << ph << ", RMS " << rms << ", Vcal " << vc << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( fire2 ) // fire2 col row (nTrig) [2-row correlation]
// fluctuates wildly
{
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 1;

  tb.roc_Col_Enable( col, true );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

  int row2 = row + 1;
  if( row2 == 80 )
    row2 = row - 1;
  trim = modtrm[0][col][row2];
  tb.roc_Pix_Trim( col, row2, trim );
  tb.roc_Pix_Cal( col, row2, false );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );
  tb.Daq_Start(  );
  tb.uDelay( 100 );

  for( int k = 0; k < nTrig; ++k ) {
    tb.Pg_Single(  );
    tb.uDelay( 200 );
  }

 //tb.Daq_Stop();
  cout << "DAQ size " << tb.Daq_GetSize(  ) << endl;

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize(  ) );

  tb.Daq_Read( data, Blocksize );

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

  cout << "data size " << data.size(  );
  for( size_t i = 0; i < data.size(  ); ++i ) {
    if( ( data[i] & 0xffc ) == 0x7f8 )
      cout << " :";
    printf( " %4X", data[i] ); // full word with FPGA markers
   //printf( " %3X", data[i] & 0xfff ); // 12 bits per word
  }
  cout << endl;

  vector < CRocPixel > pixels; // datastream.h
  pixels.reserve( 2 );

  Decoder dec;
  bool even = 1;

  int evt = 0;
  int cnt[2] = { 0 };
  int phsum[2] = { 0 };
  int phsu2[2] = { 0 };
  double vcsum[2] = { 0 };
  int php[2] = { 0 };
  int n2 = 0;
  int ppsum = 0;

  for( unsigned int i = 0; i < data.size(  ); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // ROC header
      even = 0;
      evt++;
      pixels.clear(  );
      php[0] = -1;
      php[1] = -1;
    }
    else if( even ) { // merge 2 words into one pixel int:

      if( i < data.size(  ) - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        CRocPixel pix;
        pix.raw = data[i] << 12;
        pix.raw += data[i + 1] & 0xfff;
        pix.DecodeRaw(  );
        pixels.push_back( pix );

        dec.Translate( raw );

        uint16_t ix = dec.GetX(  );
        uint16_t iy = dec.GetY(  );
        uint16_t ph = dec.GetPH(  );
        double vc = PHtoVcal( ph, ix, iy );

        if( ix == col && iy == row ) {
          cnt[0]++;
          phsum[0] += ph;
          phsu2[0] += ph * ph;
          vcsum[0] += vc;
          php[0] = ph;
        }
        else if( ix == col && iy == row2 ) {
          cnt[1]++;
          phsum[1] += ph;
          phsu2[1] += ph * ph;
          vcsum[1] += vc;
          php[1] = ph;
        }
        else {
          cout << " + " << ix << " " << iy << " " << ph;
        }

       // check for end of event:

        if( data[i + 1] & 0x4000 && php[0] > 0 && php[1] > 0 ) {
          ppsum += php[0] * php[1];
          n2++;
        } // event end

      } // data size

    } // even

    even = 1 - even;

  } // data
  cout << endl;

  if( n2 > 0 && cnt[0] > 1 && cnt[1] > 1 ) {
    double ph0 = ( double ) phsum[0] / cnt[0];
    double rms0 = sqrt( ( double ) phsu2[0] / cnt[0] - ph0 * ph0 );
    double ph1 = ( double ) phsum[1] / cnt[1];
    double rms1 = sqrt( ( double ) phsu2[1] / cnt[1] - ph1 * ph1 );
    double cov = ( double ) ppsum / n2 - ph0 * ph1;
    double cor = 0;
    if( rms0 > 1e-9 && rms1 > 1e-9 )
      cor = cov / rms0 / rms1;

    cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
      << " responses " << cnt[0]
      << ", <PH> " << ph0 << ", RMS " << rms0 << endl;
    cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row2
      << " responses " << cnt[1]
      << ", <PH> " << ph1 << ", RMS " << rms1 << endl;
    cout << "cov " << cov << ", corr " << cor << endl;
  }

  tb.roc_Pix_Mask( col, row );
  tb.roc_Pix_Mask( col, row2 );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal(  );
  tb.Flush(  );

  return true;
}

//------------------------------------------------------------------------------
// utility function: single pixel dac scan
bool DacScanPix( const uint8_t roc, const uint8_t col, const uint8_t row,
                 const uint8_t dac, const uint8_t stp, const int16_t nTrig,
                 vector < int16_t > &nReadouts,
                 vector < double >&PHavg, vector < double >&PHrms )
{
  bool ldb = 0;

  tb.roc_I2cAddr( roc );
  tb.roc_Col_Enable( col, true );
  int trim = modtrm[roc][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

  vector < uint8_t > trimvalues( 4160 );

  for( int x = 0; x < 52; x++ ) {
    for( int y = 0; y < 80; y++ ) {
      int trim = modtrm[roc][x][y];
      int i = 80 * x + y;
      trimvalues[i] = trim;
    } // row y
  } // col x
  tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  int tbmch = roc / 8;          // 0 or 1

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      nrocs++;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, tbmch );
#endif
  if( nrocs == 1 )
    tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  else {
    tb.Daq_Select_Deser400(  );
    tb.Daq_Deser400_Reset( 3 );
  }
  tb.uDelay( 100 );
  tb.Daq_Start( tbmch );
  tb.uDelay( 100 );
  tb.Flush(  );

  uint16_t flags = 0;
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS
 //flags |= 0x0020; // don't use DAC correction table
 //flags |= 0x0010; // FLAG_FORCE_MASK (needs trims)

 // scan dac:

  int vdac = dacval[roc][dac]; // remember

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );

  uint16_t mTrig = abs( nTrig );

  if( ldb )
    cout << "  scan Dac " << ( int ) dac
	 << " for Pix " << ( int ) roc << " " << ( int ) col << " " << ( int ) row
	 << " mTrig " << mTrig << ", flags hex " << hex << flags << dec << endl;

  bool done = 0;
  try {
    done =
      tb.LoopSingleRocOnePixelDacScan( roc, col, row, mTrig, flags, dac, stp,
                                       dacstrt, dacstop );
  }
  catch( CRpcError & e ) {
    e.What(  );
    return 0;
  }

 // header = 1 word
 // pixel = +2 words
 // size = 256 * mTrig * (1 or 3) = 7680 for mTrig 10, all respond

  tb.Daq_Stop( tbmch ); // don't need to stop with FW 2.11

  if( ldb ) {
    cout << "  DAQ size " << tb.Daq_GetSize( tbmch ) << endl;
    cout << "  DAQ fill " << ( int ) tb.Daq_FillLevel( tbmch ) << endl;
    cout << ( done ? "done" : "not done" ) << endl;
  }

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize( tbmch ) );

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest, tbmch ); // 32768 gives zero
    if( ldb )
      cout << "  data size " << data.size(  )
	   << ", remaining " << rest << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest, tbmch );
      data.insert( data.end(  ), dataB.begin(  ), dataB.end(  ) );
      if( ldb )
	cout << "  data size " << data.size(  )
	     << ", remaining " << rest << endl;
      dataB.clear(  );
    }
  }
  catch( CRpcError & e ) {
    e.What(  );
    return 0;
  }

#ifdef DAQOPENCLOSE
  tb.Daq_Close( tbmch );
 //tb.Daq_DeselectAll();
#endif

  tb.SetDAC( dac, vdac ); // restore dac value
  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal(  );
  tb.Flush(  );

 // unpack data:

  if( nrocs == 1 ) {

    int pos = 0;

    try {

      int32_t nstp = ( dacstop - dacstrt ) / stp + 1;

      int nmin = mTrig;
      int nmax = 0;

      for( int32_t j = 0; j < nstp; ++j ) { // DAC steps

        int cnt = 0;
        int phsum = 0;
        int phsu2 = 0;

        for( int k = 0; k < mTrig; ++k ) {

         //cout << setw(5) << pos;
          int hdr;
          vector < PixelReadoutData > vpix = GetEvent( data, pos, hdr ); // analyzer
         //cout << " " << hex << pix.hdr << dec << " " << pix.n << endl;
          for( size_t ipx = 0; ipx < vpix.size(  ); ++ipx )
            if( vpix[ipx].x == col && vpix[ipx].y == row ) {
              cnt++;
              int ph = vpix[ipx].p;
              phsum += ph;
              phsu2 += ph * ph;
            }

        } // trig

        nReadouts.push_back( cnt );
        if( cnt > nmax )
          nmax = cnt;
        if( cnt < nmin )
          nmin = cnt;

        double ph = -1.0;
        double rms = -0.1;
        if( cnt > 0 ) {
          ph = ( double ) phsum / cnt;
          rms = sqrt( ( double ) phsu2 / cnt - ph * ph );
        }
        PHavg.push_back( ph );
        PHrms.push_back( rms );

      } // dacs

      int thr = nmin + ( nmax - nmin ) / 2; // 50% level
      int thrup = dacstrt;
      int thrdn = dacstop;

      for( size_t i = 0; i < nReadouts.size(  ) - 1; ++i ) {
        bool below = 1;
        if( nReadouts[i] > thr )
          below = 0;
        if( below ) {
          if( nReadouts[i + 1] > thr )
            thrup = dacstrt + i + 1; // first above
        }
        else if( nReadouts[i + 1] < thr )
          thrdn = dacstrt + i + 1; // first below
      }

      if( ldb )
	cout << "  responses min " << nmin << " max " << nmax
	     << ", step above 50% at " << thrup
	     << ", step below 50% at " << thrdn << endl;

    } // try

    catch( int e ) {
      cout << "  Data error " << e << " at pos " << pos << endl;
      for( int i = pos - 8;
           i >= 0 && i < pos + 8 && i < int ( data.size(  ) ); ++i )
        cout << setw( 12 ) << i << hex << setw( 5 ) << data.at( i )
	     << dec << endl;
      return false;
    }

  } // single ROC

  else { // Module:

    bool ldbm = 0;

    int event = 0;

    int cnt = 0;
    int phsum = 0;

    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = 8 * tbmch - 1; // will start at 0 or 8

   // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data.size(  ); ++i ) {

      int d = data[i] & 0xfff;  // 12 bits data
      int v = ( data[i] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

       // TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
       //DecodeTbmHeader(hdr);
        if( ldbm )
          cout << "event " << setw( 6 ) << event;
        kroc = 8 * tbmch - 1; // new event, will start at 0 or 8
        break;

       // ROC header data:
      case 4:
        kroc++;
        break;

       // pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
       //DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
        c = ( raw >> 12 ) & 7;
        c = c * 6 + ( ( raw >> 9 ) & 7 );
        r = ( raw >> 6 ) & 7;
        r = r * 6 + ( ( raw >> 3 ) & 7 );
        r = r * 6 + ( raw & 7 );
        y = 80 - r / 2;
        x = 2 * c + ( r & 1 );
        if( ldbm )
          cout << setw( 3 ) << kroc << setw( 3 ) << x << setw( 3 ) << y <<
            setw( 4 ) << ph;
        if( kroc == roc && x == col && y == row ) {
          cnt++;
          phsum += ph;
        }
        break;

       // TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
       //DecodeTbmTrailer(trl);
        if( event % mTrig == mTrig - 1 ) {
          nReadouts.push_back( cnt );
          double ph = ( cnt > 0 ) ? ( double ) phsum / cnt : -1.0;
          PHavg.push_back( ph );
          cnt = 0;
          phsum = 0;
        }
        if( ldbm )
          cout << endl;
        event++;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    if( ldb )
      cout << "  events " << event
	   << " = " << event / mTrig << " dac values" << endl;

  } // module

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( caldel ) // scan and set CalDel using one pixel
{
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 100;

  Log.section( "CALDEL", false );
  Log.printf( " pixel %i %i\n", col, row );

  uint8_t dac = CalDel;
  int val = dacval[0][CalDel];
  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( 0, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

  int nstp = nReadouts.size(  );
  cout << "DAC steps " << nstp << endl;

  int wbc = dacval[0][WBC];

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "caldel_%02i_%02i_wbc%03i", col, row, wbc ),
          Form( "CalDel scan col %i row %i WBC %i;CalDel [DAC];responses",
                col, row, wbc ), nstp, -0.5, nstp - 0.5 );

 // analyze:

  int i0 = 0;
  int i9 = 0;

  for( int j = 0; j < nstp; ++j ) {

    int cnt = nReadouts.at( j );

    cout << " " << cnt;
    Log.printf( "%i %i\n", j, cnt );
    h11->Fill( j, cnt );

    if( cnt == nTrig ) {
      i9 = j; // end of plateau
      if( i0 == 0 )
        i0 = j; // begin of plateau
    }

  } // caldel
  cout << endl;
  cout << "eff plateau from " << i0 << " to " << i9 << endl;

  h11->Write(  );
  h11->SetStats( 0 );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11" << endl;

  if( i9 > 0 ) {
    int i2 = i0 + ( i9 - i0 ) / 4;
    tb.SetDAC( CalDel, i2 );
    dacval[0][CalDel] = i2;
    printf( "set CalDel to %i\n", i2 );
    Log.printf( "[SETDAC] %i  %i\n", CalDel, i2 );
  }
  else
    tb.SetDAC( CalDel, val ); // back to default
  tb.Flush(  );

  Log.flush(  );

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( caldelmap ) // map caldel left edge, 22 s (nTrig 10), 57 s (nTrig 99)
{
  Log.section( "CALDELMAP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  const int vdac = dacval[0][CalDel];
  const int vctl = dacval[0][CtrlReg];
  const int vcal = dacval[0][Vcal];
  tb.SetDAC( CtrlReg, 4 );
  tb.SetDAC( Vcal, 255 ); // max pulse

  int nTrig = 16;
  int start = dacval[0][CalDel] - 10;
  int step = 1;
  int thrLevel = nTrig / 2;
  bool xtalk = 0;
  bool cals = 0;

#ifndef DAQOPENCLOSE
  tb.Daq_Stop(  ); // tb.PixelThreshold starts with Daq_Open
  tb.Daq_Close(  );
#endif

  if( h21 )
    delete h21;
  h21 = new TH2D( "CalDelMap",
                  "CalDel map;col;row;CalDel range left edge [DAC]",
                  52, -0.5, 51.5, 80, -0.5, 79.5 );

  int minthr = 255;
  int maxthr = 0;

  for( int col = 0; col < 52; ++col ) {

    cout << endl << setw( 2 ) << col << " ";
    tb.roc_Col_Enable( col, 1 );

    for( int row = 0; row < 80; ++row ) {
     //for( int row = 20; row < 21; ++row ) {

      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );

      int thr = tb.PixelThreshold( col, row,
                                   start, step,
                                   thrLevel, nTrig,
                                   CalDel, xtalk, cals );
     // PixelThreshold ends with dclose
      cout << setw( 4 ) << thr << flush;
      if( row % 20 == 19 )
        cout << endl << "   ";
      Log.printf( " %i", thr );
      h21->Fill( col, row, thr );
      if( thr > maxthr )
        maxthr = thr;
      if( thr < minthr )
        minthr = thr;

      tb.roc_Pix_Mask( col, row );
      tb.roc_ClrCal(  );
      tb.Flush(  );

    } //rows

    Log.printf( "\n" );
    tb.roc_Col_Enable( col, 0 );
    tb.Flush(  );

  } // cols

  cout << endl;
  cout << "min thr" << setw( 4 ) << minthr << endl;
  cout << "max thr" << setw( 4 ) << maxthr << endl;

  tb.SetDAC( Vcal, vcal ); // restore dac value
  tb.SetDAC( CalDel, vdac ); // restore dac value
  tb.SetDAC( CtrlReg, vctl );

#ifndef DAQOPENCLOSE
  tb.Daq_Open( tbState.GetDaqSize(  ) ); // PixelThreshold ends with dclose
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
#endif

  tb.Flush(  );

 //double defaultLeftMargin = c1->GetLeftMargin();
 //double defaultRightMargin = c1->GetRightMargin();
 //c1->SetLeftMargin(0.10);
 //c1->SetRightMargin(0.18);
  h21->Write(  );
  h21->SetStats( 0 );
  h21->SetMinimum( minthr );
  h21->SetMaximum( maxthr );
  h21->GetYaxis(  )->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update(  );
  cout << "histos 21" << endl;
 //c1->SetLeftMargin(defaultLeftMargin);
 //c1->SetRightMargin(defaultRightMargin);

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( modcaldel ) // set caldel for modules (using one pixel) 
{
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  Log.section( "MODCALDEL", false );
  Log.printf( " pixel %i %i\n", col, row );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  int mm[16];
 //int m0[16];
 //int m9[16];

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << endl << setw( 2 ) << "ROC " << roc << endl;
   /*
      tb.roc_I2cAddr(roc);
      tb.roc_Col_Enable( col, true );
      int trim = modtrm[roc][col][row];
      tb.roc_Pix_Trim( col, row, trim );
      tb.roc_Pix_Cal( col, row, false );
    */
   // scan caldel:

   //int nTrig = 99;
    int nTrig = 10;
    uint8_t dac = CalDel;

    vector < int16_t > nReadouts; // size 0
    vector < double >PHavg;
    vector < double >PHrms;
    nReadouts.reserve( 256 ); // size 0, capacity 256
    PHavg.reserve( 256 );
    PHrms.reserve( 256 );

    DacScanPix( roc, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

    tb.roc_Pix_Mask( col, row );
    tb.roc_Col_Enable( col, 0 );
    tb.roc_ClrCal(  );
    tb.Flush(  );

    int nstp = nReadouts.size(  );
    cout << "DAC steps " << nstp << endl;

   // analyze:

    int nm = 0;
    int i0 = 0;
    int i9 = 0;

    for( int j = 0; j < nstp; ++j ) {

      int cnt = nReadouts.at( j );

      cout << " " << cnt;
      Log.printf( "%i %i\n", j, cnt );

      if( cnt > nm ) {
        nm = cnt;
        i0 = j; // begin of plateau
      }
      if( cnt >= nm ) {
        i9 = j; // end of plateau
      }

    } // caldel
    cout << endl;
    cout << "eff plateau " << nm << " from " << i0 << " to " << i9 << endl;

    mm[roc] = nm;
   //m0[roc] = i0;
   //m9[roc] = i9;

    if( nm > nTrig / 2 ) {
     //int i2 = i0 + (i9-i0)/2; // mid OK for Vcal
     //int i2 = i0 + (i9-i0)/3; // left edge better for cals
      int i2 = i0 + ( i9 - i0 ) / 4; // left edge better for cals
      tb.SetDAC( CalDel, i2 );
      DO_FLUSH;
      dacval[roc][CalDel] = i2;
      printf( "set CalDel to %i\n", i2 );
      Log.printf( "[SETDAC] %i  %i\n", CalDel, i2 );
    }

  } // rocs
  cout << endl;
  for( int roc = 0; roc < 16; ++roc )
    cout << setw( 2 ) << roc << " CalDel " << setw( 3 ) << dacval[roc][CalDel]
      << " plateau height " << mm[roc]
      << endl;

  Log.flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
// utility function: single pixel threshold on module or ROC
int ThrPix( const uint8_t roc, const uint8_t col, const uint8_t row,
            const uint8_t dac, const uint8_t stp, const int16_t nTrig )
{
  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, stp, nTrig, nReadouts, PHavg, PHrms ); // ROC or mod

  int nstp = nReadouts.size(  );

 // analyze:

  int nmx = 0;

  for( int j = 0; j < nstp; ++j ) {
    int cnt = nReadouts.at( j );
    if( cnt > nmx )
      nmx = cnt;
  }

  int thr = 0;
  for( int j = 0; j < nstp; ++j )
    if( nReadouts.at( j ) <= 0.5 * nmx )
      thr = j; // overwritten until passed

  return thr;
}

//------------------------------------------------------------------------------
CMD_PROC( modpixsc ) // S-curve for modules, one pix per ROC
{
  int col, row, nTrig;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( nTrig, 1, 65000 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // one pix per ROC:

  vector < uint8_t > rocAddress;

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    rocAddress.push_back( roc );
    tb.roc_I2cAddr( roc );
    tb.SetDAC( CtrlReg, 0 ); // small Vcal
    tb.roc_Col_Enable( col, 1 );
    int trim = modtrm[roc][col][row];
    tb.roc_Pix_Trim( col, row, trim );
   //tb.roc_Pix_Cal( col, row, false );
  }
  tb.uDelay( 1000 );
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif
  tb.Daq_Select_Deser400(  );
  tb.uDelay( 100 );
  tb.Daq_Start( 0 );
  tb.Daq_Start( 1 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0;           // or flags = FLAG_USE_CALS;
 //flags |= 0x0010; // FLAG_FORCE_MASK (needs trims)

  int dac = Vcal;
  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int nstp = dacstop - dacstrt + 1;

  gettimeofday( &tv, NULL );
  long s1 = tv.tv_sec;          // seconds since 1.1.1970
  long u1 = tv.tv_usec;         // microseconds

  try {
    tb.LoopMultiRocOnePixelDacScan( rocAddress, col, row, nTrig, flags, dac,
                                    dacstrt, dacstop );
  }
  catch( CRpcError & e ) {
    e.What(  );
    return 0;
  }

  cout << "waiting for FPGA..." << endl;
  int dSize[2];
  dSize[0] = tb.Daq_GetSize( 0 );
  dSize[1] = tb.Daq_GetSize( 1 );

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec;          // seconds since 1.1.1970
  long u2 = tv.tv_usec;         // microseconds
  double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;

  cout << "LoopMultiRocOnePixelDacScan takes " << dt << " s"
    << " = " << dt / 16 / nstp / nTrig * 1e6 << " us / pix" << endl;

 // read and unpack data:

  int cnt[16][256] = { {0} };

 // TBM channels:

  for( int tbmch = 0; tbmch < 2; ++tbmch ) {

    cout << "DAQ size channel " << tbmch
      << " = " << dSize[tbmch] << " words " << endl;

   //cout << "DAQ_Stop..." << endl;
   //tb.Daq_Stop(tbmch);

    vector < uint16_t > data;
    data.reserve( dSize[tbmch] );

    try {
      uint32_t rest;
      tb.Daq_Read( data, Blocksize, rest, tbmch ); // 32768 gives zero
      cout << "data size " << data.size(  ) << ", remaining " << rest << endl;
      while( rest > 0 ) {
        vector < uint16_t > dataB;
        dataB.reserve( Blocksize );
        tb.uDelay( 5000 );
        tb.Daq_Read( dataB, Blocksize, rest, tbmch );
        data.insert( data.end(  ), dataB.begin(  ), dataB.end(  ) );
        cout << "data size " << data.size(  )
	     << ", remaining " << rest << endl;
        dataB.clear(  );
      }
    }
    catch( CRpcError & e ) {
      e.What(  );
      return 0;
    }

   // decode:

    bool ldb = 0;

    uint32_t nev = 0;
    uint32_t nrh = 0;           // ROC headers
    uint32_t npx = 0;
    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = 8 * tbmch - 1; // will start at 0 or 8
    uint8_t idc = 0;

   // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data.size(  ); ++i ) {

      int d = data[i] & 0xfff;  // 12 bits data
      int v = ( data[i] >> 12 ) & 0xe; // 3 flag bits

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

       // TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
       //DecodeTbmHeader(hdr);
        if( ldb )
          cout << "event " << setw( 6 ) << nev;
        kroc = 8 * tbmch - 1; // new event, will start at 0 or 8
        idc = nev / nTrig; // 0..255
        break;

       // ROC header data:
      case 4:
        kroc++; // start at 0
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }
        nrh++;
        break;

       // pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
       //DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
        c = ( raw >> 12 ) & 7;
        c = c * 6 + ( ( raw >> 9 ) & 7 );
        r = ( raw >> 6 ) & 7;
        r = r * 6 + ( ( raw >> 3 ) & 7 );
        r = r * 6 + ( raw & 7 );
        y = 80 - r / 2;
        x = 2 * c + ( r & 1 );
        if( ldb )
          cout << setw( 3 ) << kroc << setw( 3 )
            << x << setw( 3 ) << y << setw( 4 ) << ph;
        npx++;
        if( x == col && y == row && kroc >= 0 && kroc < 16 )
          cnt[kroc][idc]++;
        break;

       // TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
       //DecodeTbmTrailer(trl);
        if( ldb )
          cout << endl;
        nev++;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "TBM ch " << tbmch
      << ": events " << nev
      << " (expect " << nstp * nTrig << ")"
      << ", ROC headers " << nrh
      << " (expect " << nstp * nTrig * 8 << ")" << ", pixels " << npx << endl;

  } // tbmch

  gettimeofday( &tv, NULL );
  long s3 = tv.tv_sec;          // seconds since 1.1.1970
  long u3 = tv.tv_usec;         // microseconds
  double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
  cout << "Daq_Read takes " << dtr << " s"
    << " = " << 2 * ( dSize[0] + dSize[1] ) / dtr / 1024 / 1024 << " MiB/s"
    << endl;

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    cout << "ROC " << setw( 2 ) << roc << ":" << endl;
    int i10 = 0;
    int i50 = 0;
    int i90 = 0;
    for( int idc = 0; idc < nstp; ++idc ) {
      int ni = cnt[roc][idc];
      cout << setw( 3 ) << ni;
      if( ni <= 0.1 * nTrig )
        i10 = idc; // -1.28155 sigma
      if( ni <= 0.5 * nTrig )
        i50 = idc;
      if( ni <= 0.9 * nTrig )
        i90 = idc; // +1.28155 sigma
    }
    cout << endl;
    cout << "thr " << i50 << ", width " << i90 - i10 << endl;
  }

 // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask(  );
    tb.roc_ClrCal(  );
  }
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

  Log.flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( modsc ) // S-curve for modules, all pix
{
  int nTrig;
  PAR_INT( nTrig, 1, 65000 );

  int dacstep = 1;
  if( !PAR_IS_INT( dacstep, 1, 255 ) )
    dacstep = 1;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // all pix per ROC:

  vector < uint8_t > rocAddress;
  rocAddress.reserve( 16 );

  cout << "set ROC";
  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << " " << roc;
    rocAddress.push_back( roc );
    tb.roc_I2cAddr( roc );

    tb.SetDAC( CtrlReg, 0 ); // small Vcal

    vector < uint8_t > trimvalues( 4160 );

    for( int col = 0; col < 52; ++col ) {
      //tb.roc_Col_Enable( col, 1 );
      for( int row = 0; row < 80; ++row ) {
        int trim = modtrm[roc][col][row];
       //tb.roc_Pix_Trim( col, row, trim );
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row
    } // col

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  } // roc

  tb.uDelay( 1000 );
  tb.Flush(  );

  cout << " = " << rocAddress.size(  ) << " ROCs" << endl;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif
  tb.Daq_Select_Deser400(  );
  tb.uDelay( 100 );
  tb.Daq_Start( 0 );
  tb.Daq_Start( 1 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0;           // or flags = FLAG_USE_CALS;

 //flags |= 0x0010; // FLAG_FORCE_MASK (needs trims)

  uint8_t dac = Vcal;
  uint8_t dacstrt = dacStrt( dac );
  uint8_t dacstop = dacStop( dac );

  int nstp = ( dacstop - dacstrt ) / dacstep + 1;

  cout << "scan DAC " << int ( dac )
  << " from " << int ( dacstrt )
  << " to " << int ( dacstop )
  << " in " << nstp << " steps of " << int ( dacstep )
  << endl;

  vector < uint16_t > data[2];
  data[0].reserve( nstp * nTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail
  data[1].reserve( nstp * nTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail

  bool done = 0;
  double dtloop = 0;
  double dtread = 0;

  while( done == 0 ) {

    cout << "loop..." << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec;  // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    try {
      done =
        tb.LoopMultiRocAllPixelsDacScan( rocAddress, nTrig, flags,
                                         dac, dacstep, dacstrt, dacstop );
     // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
     // loop cols
     //   loop rows
     //     activate this pix on all ROCs
     //     loop dacs
     //       loop trig
    }
    catch( CRpcError & e ) {
      e.What(  );
      return 0;
    }

    cout << ( done ? "done" : "not done" ) << endl;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec;        // seconds since 1.1.1970
    long u2 = tv.tv_usec;       // microseconds
    dtloop += s2 - s1 + ( u2 - u1 ) * 1e-6;

    cout << "waiting for FPGA..." << endl;
    int dSize[2];
    dSize[0] = tb.Daq_GetSize( 0 );
    dSize[1] = tb.Daq_GetSize( 1 );

   // read:

    for( int tbmch = 0; tbmch < 2; ++tbmch ) {

      cout << "DAQ size channel " << tbmch
        << " = " << dSize[tbmch] << " words " << endl;

     //cout << "DAQ_Stop..." << endl;
     //tb.Daq_Stop(tbmch);

      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );

      try {
        uint32_t rest;
        tb.Daq_Read( dataB, Blocksize, rest, tbmch );
        data[tbmch].insert( data[tbmch].end(  ), dataB.begin(  ),
                            dataB.end(  ) );
        cout << "data[" << tbmch << "] size " << data[tbmch].size(  )
	     << ", remaining " << rest << endl;
        while( rest > 0 ) {
          dataB.clear(  );
          tb.uDelay( 5000 );
          tb.Daq_Read( dataB, Blocksize, rest, tbmch );
          data[tbmch].insert( data[tbmch].end(  ), dataB.begin(  ),
                              dataB.end(  ) );
          cout << "data[" << tbmch << "] size " << data[tbmch].size(  )
	       << ", remaining " << rest << endl;
        }
      }
      catch( CRpcError & e ) {
        e.What(  );
        return 0;
      }

    } // TBM ch

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec;        // seconds since 1.1.1970
    long u3 = tv.tv_usec;       // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

  } // while not done

  cout << "LoopMultiRocAllPixelDacScan takes " << dtloop << " s" << endl;
  cout << "Daq_Read takes " << dtread << " s"
    << " = " << 2 * ( data[0].size(  ) +
                      data[1].size(  ) ) / dtread / 1024 /
    1024 << " MiB/s" << endl;

 // decode:

 //int cnt[16][52][80][256] = {{{{0}}}}; // 17'039'360, seg fault (StackOverflow)

  int ****cnt = new int ***[16];
  for( int i = 0; i < 16; ++i )
    cnt[i] = new int **[52];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      cnt[i][j] = new int *[80];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        cnt[i][j][k] = new int[nstp];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        for( int l = 0; l < nstp; l++ )
          cnt[i][j][k][l] = 0;

  bool ldb = 0;

  for( int tbmch = 0; tbmch < 2; ++tbmch ) {

    cout << "DAQ size channel " << tbmch
      << " = " << data[tbmch].size(  ) << " words " << endl;

    uint32_t nev = 0;
    uint32_t nrh = 0;           // ROC headers
    uint32_t npx = 0;
    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = 8 * tbmch - 1; // will start at 0 or 8
    uint8_t idc = 0;

   // nDAC * nTrig * ( TBM header, 8 * ( ROC header, one pixel ), TBM trailer )

    for( size_t i = 0; i < data[tbmch].size(  ); ++i ) {

      int d = data[tbmch].at( i ) & 0xfff; // 12 bits data
      int v = ( data[tbmch].at( i ) >> 12 ) & 0xf; // 3 flag bits

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

       // TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
       //DecodeTbmHeader(hdr);
        if( ldb )
          cout << "event " << setw( 6 ) << nev;
        kroc = 8 * tbmch - 1; // new event, will start at 0 or 8
        idc = ( nev / nTrig ) % nstp; // 0..nstp-1
        break;

       // ROC header data:
      case 4:
        kroc++; // start at 0 or 8
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }
        nrh++;
        break;

       // pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
       //DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
        c = ( raw >> 12 ) & 7;
        c = c * 6 + ( ( raw >> 9 ) & 7 );
        r = ( raw >> 6 ) & 7;
        r = r * 6 + ( ( raw >> 3 ) & 7 );
        r = r * 6 + ( raw & 7 );
        y = 80 - r / 2;
        x = 2 * c + ( r & 1 );
        if( ldb )
          cout << setw( 3 ) << kroc << setw( 3 )
            << x << setw( 3 ) << y << setw( 4 ) << ph;
        npx++;
        if( x >= 0 && x < 52 && y >= 0 && y < 80 && kroc >= 0 && kroc < 16 )
          cnt[kroc][x][y][idc]++;
        break;

       // TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
       //DecodeTbmTrailer(trl);
        if( ldb )
          cout << endl;
        nev++;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "TBM ch " << tbmch
	 << ": events " << nev
	 << " (expect " << 4160 * nstp * nTrig << ")"
	 << ", ROC headers " << nrh
	 << " (expect " << 4160 * nstp * nTrig * 8 << ")"
	 << ", pixels " << npx << endl;

  } // tbmch

  int iw = 3;
  if( nTrig > 99 )
    iw = 4;

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "thr_dist",
	  "Threshold distribution;threshold [small Vcal DAC];pixels",
	  255, -0.5, 254.5 );

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "noisedist",
          "Noise distribution;width of threshold curve [small Vcal DAC];pixels",
          51, -0.5, 50.5 );

  if( h13 )
    delete h13;
  h13 = new
    TH1D( "maxdist",
          "Response distribution;responses;pixels",
          nTrig + 1, -0.5, nTrig + 0.5 );

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleThrMap",
                  "Module threshold map;col;row;threshold [small Vcal DAC]",
                  8 * 52, -0.5, 8 * 52 - 0.5, 2 * 80, -0.5, 2 * 80 - 0.5 );
  h21->GetYaxis(  )->SetTitleOffset( 1.3 );

  if( h22 )
    delete h22;
  h22 = new TH2D( "ModuleNoiseMap",
                  "Module noise map;col;row;width of threshold curve [small Vcal DAC]",
                  8 * 52, -0.5, 8 * 52 - 0.5, 2 * 80, -0.5, 2 * 80 - 0.5 );
  h22->GetYaxis(  )->SetTitleOffset( 1.3 );

  if( h23 )
    delete h23;
  h23 = new TH2D( "ModuleResponseMap",
                  "Module response map;col;row;responses",
                  8 * 52, -0.5, 8 * 52 - 0.5, 2 * 80, -0.5, 2 * 80 - 0.5 );
  h23->GetYaxis(  )->SetTitleOffset( 1.3 );

 // S-curves:

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;
    cout << "ROC " << setw( 2 ) << roc << ":" << endl;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        int nmx = 0;
        for( int idc = 0; idc < nstp; ++idc )
          if( cnt[roc][col][row][idc] > nmx )
            nmx = cnt[roc][col][row][idc];

        int i10 = 0;
        int i50 = 0;
        int i90 = 0;
        bool ldb = 0;
        if( col == 22 && row == 33 )
          ldb = 1; // one example pix

        for( int idc = 0; idc < nstp; ++idc ) {
          int ni = cnt[roc][col][row][idc];
          if( ldb )
            cout << setw( iw ) << ni;
          if( ni <= 0.1 * nmx )
            i10 = idc; // -1.28155 sigma
          if( ni <= 0.5 * nmx )
            i50 = idc;
          if( ni <= 0.9 * nmx )
            i90 = idc; // +1.28155 sigma
        }

        if( ldb )
          cout << endl;
        if( ldb )
          cout << "thr " << i50 * dacstep
	       << ", width " << ( i90 - i10 ) * dacstep << endl;

        modthr[roc][col][row] = i50 * dacstep;

        h11->Fill( i50 * dacstep );
        h12->Fill( ( i90 - i10 ) * dacstep );
        h13->Fill( nmx );
        int l = roc % 8;        // 0..7
        int m = roc / 8;        // 0..1
        int xm = 52 * l + col;  // 0..415  rocs 0 1 2 3 4 5 6 7
        if( m == 1 )
          xm = 415 - xm; // rocs 8 9 A B C D E F
        int ym = 159 * m + ( 1 - 2 * m ) * row; // 0..159
        h21->Fill( xm, ym, i50 * dacstep );
        h22->Fill( xm, ym, ( i90 - i10 ) * dacstep );
        h23->Fill( xm, ym, nmx );

      } // row

    } // col

  } // roc

 // De-Allocate memory to prevent memory leak

  for( int i = 0; i < 16; ++i ) {
    for( int j = 0; j < 52; ++j ) {
      for( int k = 0; k < 80; ++k )
        delete[]cnt[i][j][k];
      delete[]cnt[i][j];
    }
    delete[]cnt[i];
  }
  delete[]cnt;

  h11->Write(  );
  h12->Write(  );
  h13->Write(  );
  h21->Write(  );
  h22->Write(  );
  h23->Write(  );
  h21->Draw( "colz" );
  c1->Update(  );
  cout << "histos 11, 12, 13, 21, 22, 23" << endl;

 // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask(  );
    tb.roc_ClrCal(  );
  }
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

  Log.flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: print ROC threshold map

void printThrMap( const bool longmode, int roc, int &nok )
{
  int sum = 0;
  nok = 0;
  int su2 = 0;
  int vmin = 255;
  int vmax = 0;
  int colmin = -1;
  int rowmin = -1;
  int colmax = -1;
  int rowmax = -1;

  cout << "thresholds for ROC: " << setw( 2 ) << roc << endl;

  for( int row = 79; row >= 0; --row ) {

    if( longmode )
      cout << setw( 2 ) << row << ":";

    for( int col = 0; col < 52; ++col ) {

      int thr = modthr[roc][col][row];
      if( longmode )
        cout << " " << thr;
      if( col == 25 )
        if( longmode )
          cout << endl << "   ";
      Log.printf( " %i", thr );

      if( thr < 255 ) {
        sum += thr;
        su2 += thr * thr;
        nok++;
      }
      else
        cout << "thr " << setw( 3 ) << thr
          << " for pix " << setw( 2 ) << col << setw( 3 ) << row
          << " at trim " << setw( 2 ) << modtrm[roc][col][row]
          << endl;

      if( thr > 0 && thr < vmin ) {
        vmin = thr;
        colmin = col;
        rowmin = row;
      }

      if( thr > vmax && thr < 255 ) {
        vmax = thr;
        colmax = col;
        rowmax = row;
      }

    } // cols

    if( longmode )
      cout << endl;
    Log.printf( "\n" );

  } // rows

  Log.flush(  );

  cout << "  valid thresholds " << nok << endl;
  if( nok > 0 ) {
    cout << "  min thr " << vmin
      << " at" << setw( 3 ) << colmin << setw( 3 ) << rowmin << endl;
    cout << "  max thr " << vmax
      << " at" << setw( 3 ) << colmax << setw( 3 ) << rowmax << endl;
    double mid = ( double ) sum / ( double ) nok;
    double rms = sqrt( ( double ) su2 / ( double ) nok - mid * mid );
    cout << "  mean thr " << mid << ", rms " << rms << endl;
  }
  cout << "  VthrComp " << dacval[roc][VthrComp] << endl;
  cout << "  Vtrim " << dacval[roc][Vtrim] << endl;
  cout << "  CalDel " << dacval[roc][CalDel] << endl;

}

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 7.7.2014: module threshold map

void ModThrMap( int strt, int stop, int step, int nTrig, int xtlk, int cals )
{
  if( strt < 0 ) {
    cout << "ModThrMap: bad start " << strt << endl;
    return;
  }
  if( stop > 255 ) {
    cout << "ModThrMap: bad stop " << stop << endl;
    return;
  }
  if( strt > stop ) {
    cout << "ModThrMap: sttr " << strt << " > stop " << stop << endl;
    return;
  }
  if( step < 1 ) {
    cout << "ModThrMap: bad step " << step << endl;
    return;
  }
  if( nTrig < 1 ) {
    cout << "ModThrMap: bad nTrig " << nTrig << endl;
    return;
  }

 // all pix per ROC:

  vector < uint8_t > rocAddress;
  rocAddress.reserve( 16 );

  cout << "set ROC";

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << " " << roc;
    rocAddress.push_back( roc );
    tb.roc_I2cAddr( roc );

    tb.SetDAC( CtrlReg, 0 ); // small Vcal

    vector < uint8_t > trimvalues( 4160 );

    for( int col = 0; col < 52; ++col ) {
      tb.roc_Col_Enable( col, 1 );
      for( int row = 0; row < 80; ++row ) {
        int trim = modtrm[roc][col][row];
       //tb.roc_Pix_Trim( col, row, trim );
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row
    } // col

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA (done before?)

  } // roc

  tb.uDelay( 1000 );
  tb.Flush(  );

  cout << " = " << rocAddress.size(  ) << " ROCs" << endl;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif
  tb.Daq_Select_Deser400(  );
  tb.uDelay( 100 );
  tb.Daq_Start( 0 );
  tb.Daq_Start( 1 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0;
  if( cals )
    flags = 0x0002; // FLAG_CALS
  if( xtlk )
    flags |= 0x0004; // FLAG_XTALK, see pxar/core/api/api.h

 //flags |= 0x0010; // FLAG_FORCE_MASK (is default)

  uint8_t dac = Vcal;
  int nstp = ( stop - strt ) / step + 1;

  cout << "scan DAC " << int ( dac )
  << " from " << int ( strt )
  << " to " << int ( stop )
  << " in " << nstp << " steps of " << int ( step )
  << endl;

  vector < uint16_t > data[2];
  data[0].reserve( nstp * nTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail
  data[1].reserve( nstp * nTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail

  bool done = 0;
  double dtloop = 0;
  double dtread = 0;
  timeval tv;

  while( done == 0 ) {

    cout << "\tloop..." << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec;        // seconds since 1.1.1970
    long u1 = tv.tv_usec;       // microseconds

    try {
      done =
        tb.LoopMultiRocAllPixelsDacScan( rocAddress, nTrig, flags,
                                         dac, step, strt, stop );
     // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
     // loop cols
     //   loop rows
     //     activate this pix on all ROCs
     //     loop dacs
     //       loop trig
    }
    catch( CRpcError & e ) {
      e.What(  );
      return;
    }

    cout << ( done ? "\tdone" : "\tnot done" ) << endl;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec;        // seconds since 1.1.1970
    long u2 = tv.tv_usec;       // microseconds
    dtloop += s2 - s1 + ( u2 - u1 ) * 1e-6;

    cout << "\twaiting for FPGA..." << endl;
    int dSize[2];
    dSize[0] = tb.Daq_GetSize( 0 );
    dSize[1] = tb.Daq_GetSize( 1 );

   // read:

    for( int tbmch = 0; tbmch < 2; ++tbmch ) {

      cout << "\tDAQ size channel " << tbmch
        << " = " << dSize[tbmch] << " words " << endl;

     //cout << "\tDAQ_Stop..." << endl;
     //tb.Daq_Stop(tbmch);

      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );

      try {
        uint32_t rest;
        tb.Daq_Read( dataB, Blocksize, rest, tbmch );
        data[tbmch].insert( data[tbmch].end(  ), dataB.begin(  ),
                            dataB.end(  ) );
        cout << "\tdata[" << tbmch << "] size " << data[tbmch].size(  )
          << ", remaining " << rest << endl;
        while( rest > 0 ) {
          dataB.clear(  );
          tb.uDelay( 5000 );
          tb.Daq_Read( dataB, Blocksize, rest, tbmch );
          data[tbmch].insert( data[tbmch].end(  ), dataB.begin(  ),
                              dataB.end(  ) );
          cout << "\tdata[" << tbmch << "] size " << data[tbmch].size(  )
            << ", remaining " << rest << endl;
        }
      }
      catch( CRpcError & e ) {
        e.What(  );
        return;
      }

    } // TBM ch

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec;        // seconds since 1.1.1970
    long u3 = tv.tv_usec;       // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

  } // while not done

  cout << "\tLoopMultiRocAllPixelDacScan takes " << dtloop << " s" << endl;
  cout << "\tDaq_Read takes " << dtread << " s"
    << " = " << 2 * ( data[0].size(  ) +
                      data[1].size(  ) ) / dtread / 1024 /
    1024 << " MiB/s" << endl;

 // decode:

 //int cnt[16][52][80][256] = {{{{0}}}}; // 17'039'360, seg fault (StackOverflow)

  int ****cnt = new int ***[16];
  for( int i = 0; i < 16; ++i )
    cnt[i] = new int **[52];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      cnt[i][j] = new int *[80];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
       //cnt[i][j][k] = new int[256];
        cnt[i][j][k] = new int[nstp];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        for( int l = 0; l < nstp; l++ )
          cnt[i][j][k][l] = 0;

  bool ldb = 0;

  for( int tbmch = 0; tbmch < 2; ++tbmch ) {

    cout << "\tDAQ size channel " << tbmch
      << " = " << data[tbmch].size(  ) << " words " << endl;

    uint32_t nev = 0;
    uint32_t nrh = 0;           // ROC headers
    uint32_t npx = 0;
    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = 8 * tbmch - 1; // will start at 0 or 8
    uint8_t idc = 0;            // 0..255

   // nDAC * nTrig * ( TBM header, 8 * ( ROC header, one pixel ), TBM trailer )

    for( size_t i = 0; i < data[tbmch].size(  ); ++i ) {

      int d = data[tbmch].at( i ) & 0xfff; // 12 bits data
      int v = ( data[tbmch].at( i ) >> 12 ) & 0xf; // 3 flag bits

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

       // TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
       //DecodeTbmHeader(hdr);
        if( ldb )
          cout << "event " << setw( 6 ) << nev;
        kroc = 8 * tbmch - 1; // new event, will start at 0 or 8
        idc = ( nev / nTrig ) % nstp; // 0..nstp-1
        break;

       // ROC header data:
      case 4:
        kroc++; // start at 0 or 8
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }
        nrh++;
        break;

       // pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
       //DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
        c = ( raw >> 12 ) & 7;
        c = c * 6 + ( ( raw >> 9 ) & 7 );
        r = ( raw >> 6 ) & 7;
        r = r * 6 + ( ( raw >> 3 ) & 7 );
        r = r * 6 + ( raw & 7 );
        y = 80 - r / 2;
        x = 2 * c + ( r & 1 );
        if( ldb )
          cout << setw( 3 ) << kroc << setw( 3 )
            << x << setw( 3 ) << y << setw( 4 ) << ph;
        npx++;
        if( x >= 0 && x < 52 && y >= 0 && y < 80 && kroc >= 0 && kroc < 16 )
          cnt[kroc][x][y][idc]++;
        break;

       // TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
       //DecodeTbmTrailer(trl);
        if( ldb )
          cout << endl;
        nev++;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "\tTBM ch " << tbmch
      << ": events " << nev
      << " (expect " << 4160 * nstp * nTrig << ")"
      << ", ROC headers " << nrh
      << " (expect " << 4160 * nstp * nTrig * 8 << ")"
      << ", pixels " << npx << endl;

  } // tbmch

  int iw = 3;
  if( nTrig > 99 )
    iw = 4;

 // S-curves:
  bool lex = 0; // example

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;
    if( lex ) cout << "\tROC " << setw( 2 ) << roc << ":" << endl;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        int nmx = 0;
        for( int idc = 0; idc < nstp; ++idc )
          if( cnt[roc][col][row][idc] > nmx )
            nmx = cnt[roc][col][row][idc];

        int i10 = 0;
        int i50 = 0;
        int i90 = 0;
        bool ldb = 0;
        if( lex && col == 22 && row == 33 )
          ldb = 1; // one example pix

        for( int idc = 0; idc < nstp; ++idc ) {
          int ni = cnt[roc][col][row][idc];
          if( ldb )
            cout << setw( iw ) << ni;
          if( ni <= 0.1 * nmx )
            i10 = idc; // -1.28155 sigma
          if( ni <= 0.5 * nmx )
            i50 = idc;
          if( ni <= 0.9 * nmx )
            i90 = idc; // +1.28155 sigma
        }

        if( ldb )
          cout << endl;
        if( ldb )
          cout << "\tthr " << i50 * step
	       << ", width " << ( i90 - i10 ) * step << endl;

        modthr[roc][col][row] = i50 * step;

      } // row

    } // col

  } // roc

 // De-Allocate memory to prevent memory leak

  for( int i = 0; i < 16; ++i ) {
    for( int j = 0; j < 52; ++j ) {
      for( int k = 0; k < 80; ++k )
        delete[]cnt[i][j][k];
      delete[]cnt[i][j];
    }
    delete[]cnt[i];
  }
  delete[]cnt;

 // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask(  );
    tb.roc_ClrCal(  );
  }
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

}

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 8.7.2014: set global threshold per ROC. works. adjust CalDel
CMD_PROC( modvthrcomp )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "MODVTHRCOMP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // take threshold map, find softest pixel

  int strt = 0;
  int stop = 255;               // Vcal scan range
  int step = 4;                 // coarse

  const int nTrig = 10;
  const int xtlk = 0;
  const int cals = 0;

  cout << "measuring Vcal threshold map in range "
    << strt << " to " << stop
    << " in steps of " << step << " with " << nTrig << " triggers" << endl;

  ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // fills modthr

  int nok = 0;
  for( size_t roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    printThrMap( 0, roc, nok );
  }

  cout << endl;

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // one pixel per ROC:

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    int colmin = -1;
    int rowmin = -1;
    int vmin = 255;

    for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {

        int thr = modthr[roc][col][row];
        if( thr < vmin ) {
          vmin = thr;
          colmin = col;
          rowmin = row;
        }

      } // cols

    cout << "ROC " << roc << endl;
    cout << "  min thr " << vmin << " at " << colmin << " " << rowmin << endl;

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // setVthrComp using min pixel:

    tb.roc_I2cAddr( roc );

    tb.SetDAC( CtrlReg, 0 ); // small Vcal

    int dac = Vcal;
    step = 1; // fine Vcal

   // VthrComp has negative slope: higher = softer

    int vstp = -1;              // towards harder threshold
    if( vmin > target )
      vstp = 1; // towards softer threshold

    int vthr = dacval[roc][VthrComp];

    for( ; vthr < 255 && vthr > 0; vthr += vstp ) {

      tb.SetDAC( VthrComp, vthr );
      tb.uDelay( 100 );
      tb.Flush(  );

      int thr = ThrPix( roc, colmin, rowmin, dac, step, nTrig );

      cout << "VthrComp " << setw( 3 ) << vthr << " thr " << setw( 3 ) << thr
        << endl;
      if( vstp * thr <= vstp * target ) // signed
        break;
    } // vthr

    vthr -= 1; // margin towards harder threshold
    tb.SetDAC( VthrComp, vthr );
    cout << "set VthrComp to " << vthr << endl;
    dacval[roc][VthrComp] = vthr;
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    tb.Flush(  );

  } // rocs

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << "ROC " << setw( 2 ) << roc
      << " VthrComp " << setw( 3 ) << dacval[roc][VthrComp]
      << endl;
  }

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
// global:

int oldthr[16][52][80] = { {{0}} };
int oldtrm[16][52][80] = { {{0}} };

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 9.7.2014: trim bit correction for modules

void ModTrimStep( int target, int correction,
                  int step, int nTrig, int xtlk, int cals )
{
  int minthr = 255;
  int maxthr = 0;

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        int thr = modthr[roc][col][row];

        oldthr[roc][col][row] = thr; // remember

        if( thr < minthr )
          minthr = thr;
        if( thr > maxthr )
          maxthr = thr;

        int trim = modtrm[roc][col][row];

        oldtrm[roc][col][row] = trim; // remember

        if( thr > target && thr < 255 )
          trim -= correction; // make softer
        else
          trim += correction; // make harder

        if( trim < 0 )
          trim = 0;
        if( trim > 15 )
          trim = 15;

        modtrm[roc][col][row] = trim;

      } // rows

    } // cols

  } // rocs

  int strt = minthr - 4 * step;
  if( strt < 0 )
    strt = 0;
  int stop = maxthr + 4 * step;
  if( stop > 255 )
    stop = 255;

  cout << endl
    << "TrimStep with trim bit correction " << correction
    << endl
    << "  measure Vcal threshold in range " << strt << " to " << stop
    << " in steps of " << step << endl;

 // measure new result:

  ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // fills modthr

 //int nok = 0; printThrMap( 0, roc, nok );

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    for( int col = 0; col < 52; ++col ) {
      for( int row = 0; row < 80; ++row ) {

        int thr = modthr[roc][col][row];
        int old = oldthr[roc][col][row];

        if( abs( thr - target ) > abs( old - target ) ) {
          modtrm[roc][col][row] = oldtrm[roc][col][row]; // go back
          modthr[roc][col][row] = old;
        }

      } // rows
    } // cols
  } // rocs

}

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 8.7.2014: trim module
CMD_PROC( modtrim )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "TRIM", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // activate ROCs:

  vector < uint8_t > rocAddress;
  rocAddress.reserve( 16 );

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    rocAddress.push_back( roc );
    tb.roc_I2cAddr( roc );

    tb.SetDAC( CtrlReg, 0 ); // this ROC, small Vcal

    vector < uint8_t > trimvalues( 4160 );

    for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {
        int trim = 15;          // no trim
        modtrm[roc][col][row] = trim;
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  } // rocs

  tb.uDelay( 1000 );
  tb.Flush(  );

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // take threshold map, find hardest pixel

  int strt = 0;
  int stop = 255;               // Vcal scan range
  int step = 4;                 // coarse

  const int nTrig = 10;
  const int xtlk = 0;
  const int cals = 0;

  cout << "measuring Vcal threshold map in range "
    << strt << " to " << stop
    << " in steps of " << step << " with " << nTrig << " triggers" << endl;

  ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // fills modthr

  int nok = 0;
  for( size_t roc = 0; roc < 16; ++roc ) {
    printThrMap( 0, roc, nok );
  }

  cout << endl;

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );
    cout << "ROC " << roc << endl;

    int colmax = -1;
    int rowmax = -1;
    int vmin = 255;
    int vmax = 0;

    for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {

        int thr = modthr[roc][col][row];
        if( thr > vmax && thr < 255 ) {
          vmax = thr;
          colmax = col;
          rowmax = row;
        }
        if( thr < vmin )
          vmin = thr;
      } // cols

    if( vmin < target ) {
      cout << "min threshold below target on ROC " << roc
        << ": skipped (try lower VthrComp)" << endl;
      continue; // skip this ROC
    }

    cout << "max thr " << vmax << " at " << colmax << " " << rowmax << endl;


    tb.SetDAC( CtrlReg, 0 ); // this ROC, small Vcal

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // setVtrim using max pixel:

    modtrm[roc][colmax][rowmax] = 0; // 0 = strongest

    int dac = Vcal;
    step = 1; // fine Vcal

    int itrim = 1;

    for( ; itrim < 253; itrim += 2 ) {

      tb.SetDAC( Vtrim, itrim );
      tb.uDelay( 100 );
      tb.Flush(  );

      int thr = ThrPix( roc, colmax, rowmax, dac, step, nTrig );

      cout << setw( 3 ) << itrim << "  " << setw( 3 ) << thr << endl;
      if( thr < target )
        break;

    } // itrim

    itrim += 2; // margin
    tb.SetDAC( Vtrim, itrim );
    cout << "set Vtrim to " << itrim << endl;
    dacval[roc][Vtrim] = itrim;

    tb.roc_Pix_Mask( colmax, rowmax );
    tb.roc_Col_Enable( colmax, 0 );
    tb.roc_ClrCal(  );
    tb.Flush(  );

  } // rocs

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << "ROC " << setw( 2 ) << roc
      << " VthrComp " << setw( 3 ) << dacval[roc][VthrComp]
      << ", Vtrim " << setw( 3 ) << dacval[roc][Vtrim]
      << endl;
  }

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // iterate trim bits:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {
        modtrm[roc][col][row] = 7; // half way
        oldthr[roc][col][row] = modthr[roc][col][row];
        oldtrm[roc][col][row] = 15; // old
      }
  } // rocs

  cout << endl << "measuring Vcal threshold map with trim 7" << endl;

  step = 4;

  ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // uses modtrm, fills modthr

 // printThrMap( 0, roc, nok );

  int correction = 4;
  step = 4;

  ModTrimStep( target, correction, step, nTrig, xtlk, cals ); // fills modtrm, fills modthr

 //printThrMap( 0, roc, nok );

  correction = 2;
  step = 2;

  ModTrimStep( target, correction, step, nTrig, xtlk, cals ); // fills modtrm, fills modthr

 //printThrMap( 0, roc, nok );

  correction = 1;
  step = 1;

  ModTrimStep( target, correction, step, nTrig, xtlk, cals ); // fills modtrm, fills modthr

 //printThrMap( 0, roc, nok );

  correction = 1;
  step = 1;

  ModTrimStep( target, correction, step, nTrig, xtlk, cals ); // fills modtrm, fills modthr

 //printThrMap( 0, roc, nok );

  cout << "SetTrimValues in FPGA" << endl;

 // restore:

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] );
    tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore

    vector < uint8_t > trimvalues( 4160 ); // uint8 in 2.15
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        int i = 80 * col + row;
        trimvalues[i] = modtrm[roc][col][row];
      }
    tb.SetTrimValues( roc, trimvalues );
  }
  tb.Flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( phdac ) // phdac col row dac (PH vs dac)
{
  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 10;

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  Log.section( "PHDAC", false );
  Log.printf( " pixel %i %i DAC %i\n", col, row, dac );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( 0, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

  int nstp = nReadouts.size(  );

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "ph_dac%02i_%02i_%02i", dac, col, row ),
          Form( "PH vs %s col %i row %i;%s [DAC];<PH> [ADC]",
                dacName[dac].c_str(  ), col, row, dacName[dac].c_str(  ) ),
          nstp, -0.5, nstp - 0.5 );

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "vcal_dac%02i_%02i_%02i", dac, col, row ),
          Form( dacval[0][CtrlReg] == 0 ?
                "Vcal vs %s col %i row %i;%s [DAC];calibrated PH [small Vcal DAC]"
                :
                "Vcal vs %s col %i row %i;%s [DAC];calibrated PH [large Vcal DAC]",
                dacName[dac].c_str(  ), col, row, dacName[dac].c_str(  ) ),
          nstp, -0.5, nstp - 0.5 );

  if( h13 )
    delete h13;
  h13 = new
    TH1D( Form( "rms_dac%02i_%02i_%02i", dac, col, row ),
          Form( "RMS vs %s col %i row %i;%s [DAC];PH RMS [ADC]",
                dacName[dac].c_str(  ), col, row, dacName[dac].c_str(  ) ),
          nstp, -0.5, nstp - 0.5 );

 // tb.CalibrateDacScan runs on the FPGA
 // kinks in PH vs DAC
 // update DAC correction table?
 /*
    bool slow = 0;
    if( dac != 25 ) { // 25 = Vcal is corrected on FPGA
    slow = 1;
    cout << "use slow PH with corrected DAC" << endl;
    int trim = 15;
    int nTrig = 10;
    int32_t dacstrt = dacStrt( dac );
    int32_t dacstop = dacStop( dac );
    nstp = dacstop - dacstrt + 1;
    PHavg.resize(nstp);
    nReadouts.resize(nstp);
    for( int32_t i = dacstrt; i <= dacstop; ++i ) {
    tb.SetDAC( dac, i ); // corrected, less kinky
    tb.mDelay(1);
    int ph = tb.PH( col, row, trim, nTrig ); // works on FW 1.15
    PHavg.at(i) = (ph > -0.5) ? ph : -1.0; // overwrite
    nReadouts.at(i) = nTrig;
    }
    }
  */
 // print:

  double phmin = 255;

  for( int32_t i = 0; i < nstp; ++i ) {

    double ph = PHavg.at( i );
   //cout << setw(4) << ((ph > -0.1 ) ? int(ph+0.5) : -1) << "(" << setw(3) << nReadouts.at(i) << ")";
    Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );
    if( ph > -0.5 && ph < phmin )
      phmin = ph;
    if( nReadouts.at( i ) > 0 ) {
      h11->Fill( i, ph );
      h13->Fill( i, PHrms.at( i ) );
      double vc = PHtoVcal( ph, col, row );
      h12->Fill( i, vc );
     //cout << setw(3) << i << "  " << ph << "  " << vc << endl;
    }
  } // dacs
  cout << endl;
  Log.printf( "\n" );
  cout << "min PH " << phmin << endl;
  Log.flush(  );

  h11->Write(  );
  h12->Write(  );
  h13->Write(  );
  h11->SetStats( 0 );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11, 12, 13" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( calsdac ) // calsdac col row dac (cals PH vs dac)
{
  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 10;

  Log.section( "CALSDAC", false );
  Log.printf( " pixel %i %i DAC %i\n", col, row, dac );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  tb.SetDAC( CtrlReg, 4 ); // want large Vcal

  DacScanPix( 0, col, row, dac, 1, -nTrig, nReadouts, PHavg, PHrms ); // negative nTrig = cals

  tb.SetDAC( CtrlReg, dacval[0][CtrlReg] ); // restore
  tb.Flush(  );

  int nstp = nReadouts.size(  );

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "cals_dac%02i_%02i_%02i", dac, col, row ),
          Form( "CALS vs  %s col %i row %i;%s [DAC];<CALS> [ADC]",
                dacName[dac].c_str(  ), col, row, dacName[dac].c_str(  ) ),
          nstp, -0.5, nstp - 0.5 );

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "resp_cals_dac%02i_%02i_%02i", dac, col, row ),
          Form( "CALS responses vs  %s col %i row %i;%s [DAC];responses",
                dacName[dac].c_str(  ), col, row, dacName[dac].c_str(  ) ),
          nstp, -0.5, nstp - 0.5 );

 // print:

  double phmin = 255;

  for( int32_t i = 0; i < nstp; ++i ) {

    double ph = PHavg.at( i );
    cout << setw( 4 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
      <<"(" << setw( 3 ) << nReadouts.at( i ) << ")";
    Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );
    if( ph > -0.5 && ph < phmin )
      phmin = ph;
    h11->Fill( i, ph );
    h12->Fill( i, nReadouts.at( i ) );

  } // dacs
  cout << endl;
  Log.printf( "\n" );
  cout << "min PH " << phmin << endl;
  Log.flush(  );

  h11->Write(  );
  h12->Write(  );
  h12->SetStats( 0 );
  h12->Draw(  );
  c1->Update(  );
  cout << "histos 11, 12" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( effdac ) // effdac col row dac [stp] [nTrig] (efficiency vs dac)
{
  int roc = 0;
  int col, row, dac;
 //PAR_INT( roc, 0, 15 );
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers
  int stp;
  if( !PAR_IS_INT( stp, 1, 255 ) )
    stp = 1;
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 100;

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  Log.section( "EFFDAC", false );
  Log.printf( " pixel %i %i DAC %i\n", col, row, dac );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, stp, nTrig, nReadouts, PHavg, PHrms );

  int nstp = nReadouts.size(  );

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "eff_dac%02i_roc%02i_col%02i_row%02i_stp%02i",
		dac, roc, col, row, stp ),
          Form( "Eff vs %s ROC %i col %i row %i;%s [DAC] step %i;responses",
                dacName[dac].c_str(  ), roc, col, row, dacName[dac].c_str(  ), stp ),
	  nstp, -0.5, ( nstp - 0.5 ) * stp );

 // print and plot:

  for( int32_t i = 0; i < nstp; ++i ) {

    int cnt = nReadouts.at( i );
    cout << setw( 4 ) << cnt;
    int idac = dacStrt( dac ) + i * stp;
    Log.printf( "%i %i\n", idac, cnt );
    h11->Fill( idac, cnt );

  } // dacs
  cout << endl;
  Log.printf( "\n" );
  Log.flush(  );

  h11->Write(  );
  h11->SetStats( 0 );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( thrdac ) // thrdac col row dac (thr vs dac)
{
  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  Log.section( "THRDAC", false );
  Log.printf( " pixel %i %i DAC %i\n", col, row, dac );

  const int vdac = dacval[0][dac];

  tb.SetDAC( CtrlReg, 0 ); // small Vcal

  int nTrig = 20;
  int start = 30;
  int step = 1;
  int thrLevel = nTrig / 2;
  bool xtalk = 0;
  bool cals = 0;
  int trim = modtrm[0][col][row];

 // scan dac:

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int32_t dacstep = dacStep( dac );
  int nstp = ( dacstop - dacstrt ) / dacstep + 1;

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_dac%02i_%02i_%02i_bits%02i", dac, col, row, trim ),
          Form
          ( "thr vs %s col %i row %i bits %i;%s [DAC];threshold [small Vcal DAC]",
            dacName[dac].c_str(  ), col, row, trim, dacName[dac].c_str(  ) ),
          nstp, dacstrt - 0.5, dacstop + 0.5 );

#ifndef DAQOPENCLOSE
  tb.Daq_Stop(  ); // tb.PixelThreshold starts with Daq_Open
  tb.Daq_Close(  );
  tb.uDelay( 1000 );
#endif

  tb.roc_Col_Enable( col, true );
  tb.roc_Pix_Trim( col, row, trim );

  tb.Flush(  );

  int tmin = 255;
  int imin = 0;

  for( int32_t i = dacstrt; i <= dacstop; i += dacstep ) {

    tb.SetDAC( dac, i );
    tb.uDelay( 1000 );
    tb.Flush(  );

   // NIOS code pixel_dtb.cc
   //for( int16_t i = 0; i < nTriggers; ++i) {
   //  Pg_Single();
   //  uDelay(4); // 4 us => WBC < 160
   //}

    int thr = tb.PixelThreshold( col, row,
                                 start, step,
                                 thrLevel, nTrig,
                                 Vcal, xtalk, cals );
    h11->Fill( i, thr );
    cout << " " << thr << flush;
    Log.printf( "%i %i\n", i, thr );

    if( thr < tmin ) {
      tmin = thr;
      imin = i;
    }

  } // dacs

  cout << endl;
  cout << "small Vcal [DAC]" << endl;
  cout << "min Vcal threshold " << tmin << " at dac " << dac << " " << imin <<
    endl;

  h11->Write(  );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11" << endl;

  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal(  );
  tb.roc_Pix_Mask( col, row );
  tb.SetDAC( dac, vdac ); // restore dac value
  tb.SetDAC( CtrlReg, dacval[0][CtrlReg] );

#ifndef DAQOPENCLOSE
  tb.Daq_Open( tbState.GetDaqSize(  ) ); // PixelThreshold ends with dclose
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
#endif

  tb.Flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( modthrdac ) // modthrdac col row dac (thr vs dac on module)
{
  int roc = 0;
  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  if( dacval[roc][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  Log.section( "MODTHRDAC", false );
  Log.printf( " pixel %i %i DAC %i\n", col, row, dac );

  const int vdac = dacval[roc][dac];

  tb.roc_I2cAddr( roc );
  tb.SetDAC( CtrlReg, 0 ); // small Vcal
  tb.Flush(  );

  int nTrig = 20;               // for S-curve
  int step = 1;
 //bool xtlk = 0;
 //bool cals = 0;

 // scan dac:

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int32_t dacstep = dacStep( dac );
  int nstp = ( dacstop - dacstrt ) / dacstep + 1;
  int trim = modtrm[roc][col][row];

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_dac%02i_%02i_%02i_bits%02i", dac, col, row, trim ),
          Form
          ( "thr vs %s col %i row %i bits %i;%s [DAC];threshold [small Vcal DAC]",
            dacName[dac].c_str(  ), col, row, trim, dacName[dac].c_str(  ) ),
          nstp, dacstrt - 0.5, dacstop + 0.5 );

  int tmin = 255;
  int imin = 0;

  for( int32_t i = dacstrt; i <= dacstop; i += dacstep ) {

    tb.SetDAC( dac, i );
    tb.uDelay( 1000 );
    tb.Flush(  );

    int thr = ThrPix( roc, col, row, Vcal, step, nTrig );

    h11->Fill( i, thr );
    cout << " " << thr << flush;
    Log.printf( "%i %i\n", i, thr );

    if( thr < tmin ) {
      tmin = thr;
      imin = i;
    }

  } // dacs

  cout << endl;
  cout << "small Vcal [DAC]" << endl;
  cout << "min Vcal threshold " << tmin << " at dac " << dac << " " << imin <<
    endl;

  h11->Write(  );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11" << endl;

  tb.SetDAC( dac, vdac ); // restore dac value
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] );

  tb.Flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
// utility function for ROC PH and eff map
bool GetRocData( int nTrig, vector < int16_t > &nReadouts,
                 vector < double >&PHavg, vector < double >&PHrms )
{
  uint16_t flags = 0;
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  uint16_t mTrig = abs( nTrig );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );
  tb.Daq_Start(  );
  tb.uDelay( 100 );

 // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
   /*
      for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim ); // done on FPGA in TriggerLoops
      }
    */
  }
  tb.uDelay( 1000 );
  tb.Flush(  );

  cout << "  GetRocData mTrig " << mTrig << ", flags " << flags << endl;

  try {
    tb.LoopSingleRocAllPixelsCalibrate( 0, mTrig, flags );
  }
  catch( CRpcError & e ) {
    e.What(  );
    return 0;
  }

  tb.Daq_Stop(  );

 // header = 1 word
 // pixel = +2 words
 // size = 4160 * nTrig * 3 = 124'800 words

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize(  ) );
  cout << "  DAQ size " << tb.Daq_GetSize(  ) << endl;

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest ); // 32768 gives zero
    cout << "  data size " << data.size(  )
	 << ", remaining " << rest << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(  ), dataB.begin(  ), dataB.end(  ) );
      cout << "  data size " << data.size(  )
	   << ", remaining " << rest << endl;
      dataB.clear(  );
    }
  }
  catch( CRpcError & e ) {
    e.What(  );
    return 0;
  }

 // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask(  );
  tb.roc_ClrCal(  );
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

 // unpack data:

  PixelReadoutData pix;

  int pos = 0;
  int err = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      int cnt = 0;
      int phsum = 0;
      int phsu2 = 0;

      for( int k = 0; k < mTrig; ++k ) {

        err = DecodePixel( data, pos, pix ); // analyzer

        if( err )
          cout << "  error " << err << " at trig " << k
            << ", pix " << col << " " << row << ", pos " << pos << endl;
        if( err )
          break;

        if( pix.n > 0 )
          if( pix.x == col && pix.y == row ) {
            cnt++;
            phsum += pix.p;
            phsu2 += pix.p * pix.p;
          }
      } // trig

      if( err )
        break;

      nReadouts.push_back( cnt ); // 0 = no response
      double ph = -1.0;
      double rms = -0.1;
      if( cnt > 0 ) {
        ph = ( double ) phsum / cnt;
        rms = sqrt( ( double ) phsu2 / cnt - ph * ph );
      }
      PHavg.push_back( ph );
      PHrms.push_back( rms );

    } // row

    if( err )
      break;

  } // col

  return 1;
}

//------------------------------------------------------------------------------
CMD_PROC( effmap )
{
  int nTrig;                    // DAQ size 1'248'000 at nTrig = 100 if fully efficient
  PAR_INT( nTrig, 1, 65000 );

  const int vctl = dacval[0][CtrlReg];
  const int vcal = dacval[0][Vcal];

  Log.section( "EFFMAP", false );
  Log.printf( " nTrig %i Vcal %i CtrlReg %i\n", nTrig, vcal, vctl );
  cout << "effmap with " << nTrig << " triggers"
    << " at Vcal " << vcal << ", CtrlReg " << vctl << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  cout << "waiting for FPGA..." << endl;

  GetRocData( nTrig, nReadouts, PHavg, PHrms );

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec;          // seconds since 1.1.1970
  long u2 = tv.tv_usec;         // microseconds
  double dt = s2 - s0 + ( u2 - u0 ) * 1e-6;

  cout << "LoopSingleRocAllPixelsCalibrate takes " << dt << " s"
    << " = " << dt / 4160 / nTrig * 1e6 << " us / pix" << endl;

  if( h11 )
    delete h11;
  h11 = new TH1D( Form( "Responses_Vcal%i_CR%i", vcal, vctl ),
                  Form( "Responses at Vcal %i, CtrlReg %i;responses;pixels",
                        vcal, vctl ), nTrig + 1, -0.5, nTrig + 0.5 );

  if( h21 )
    delete h21;
  h21 = new TH2D( Form( "AliveMap_Vcal%i_CR%i", vcal, vctl ),
                  Form( "Alive map at Vcal %i, CtrlReg %i;col;row;responses",
                        vcal, vctl ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  int nok = 0;
  int nActive = 0;
  int nPerfect = 0;

  size_t j = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      int cnt = nReadouts.at( j );

      h11->Fill( cnt );
      h21->Fill( col, row, cnt );
      if( cnt > 0 )
        nok++;
      if( cnt >= nTrig / 2 )
        nActive++;
      if( cnt == nTrig )
        nPerfect++;
      if( cnt < nTrig )
        cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
          << " responses " << cnt << endl;
      Log.printf( " %i", cnt );

      ++j;
      if( j == nReadouts.size(  ) )
        break;

    } // row

    Log.printf( "\n" );
    if( j == nReadouts.size(  ) )
      break;

  } // col

  h11->Write(  );
  h21->Write(  );
  h21->SetStats( 0 );
  h21->SetMinimum( 0 );
  h21->SetMaximum( int ( nTrig / 20 ) * 20 + 20 );
  h21->GetYaxis(  )->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update(  );
  cout << "histos 11, 21" << endl;

  cout << endl;
  cout << " >0% pixels: " << nok << endl;
  cout << ">50% pixels: " << nActive << endl;
  cout << "100% pixels: " << nPerfect << endl;

  Log.flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( modmap ) // pixelAlive for modules
{
  int nTrig = 10;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 10;

  Log.section( "MODMAP", false );
  Log.printf( " nTrig %i\n", nTrig );
  cout << "modmap with " << nTrig << " triggers" << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // all on:

  vector < uint8_t > rocAddress;

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    rocAddress.push_back( roc );
    vector < uint8_t > trimvalues( 4160 );
    for( int col = 0; col < 52; ++col ) {
      //tb.roc_Col_Enable( col, 1 );
      for( int row = 0; row < 80; ++row ) {
        int trim = modtrm[roc][col][row];
	//tb.roc_Pix_Trim( col, row, trim );
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row
    } // col
    tb.SetTrimValues( roc, trimvalues ); // load into FPGA
  } // roc
  tb.uDelay( 1000 );
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif
  tb.Daq_Select_Deser400(  );
  tb.uDelay( 100 );
  tb.Daq_Start( 0 );
  tb.Daq_Start( 1 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0;           // or flags = FLAG_USE_CALS;

 //flags |= 0x0100; // FLAG_FORCE_UNMASKED: noisy

  vector < uint16_t > data[2];
  data[0].reserve( nTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail
  data[1].reserve( nTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail

 // measure:

  bool done = 0;
  double dtloop = 0;
  double dtread = 0;

  while( done == 0 ) {

    cout << "loop..." << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec;        // seconds since 1.1.1970
    long u1 = tv.tv_usec;       // microseconds

    try {
      done = tb.LoopMultiRocAllPixelsCalibrate( rocAddress, nTrig, flags );
    }
    catch( CRpcError & e ) {
      e.What(  );
      return 0;
    }

    cout << ( done ? "done" : "not done" ) << endl;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec;        // seconds since 1.1.1970
    long u2 = tv.tv_usec;       // microseconds
    dtloop += s2 - s1 + ( u2 - u1 ) * 1e-6;

    cout << "waiting for FPGA..." << endl;
    int dSize[2];
    dSize[0] = tb.Daq_GetSize( 0 );
    dSize[1] = tb.Daq_GetSize( 1 );

   // read TBM channels:

    for( int tbmch = 0; tbmch < 2; ++tbmch ) {

      cout << "DAQ size channel " << tbmch
	   << " = " << dSize[tbmch] << " words " << endl;

     //cout << "DAQ_Stop..." << endl;
     //tb.Daq_Stop(tbmch);

      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );

      try {
        uint32_t rest;
        tb.Daq_Read( dataB, Blocksize, rest, tbmch );
        data[tbmch].insert( data[tbmch].end(  ), dataB.begin(  ),
                            dataB.end(  ) );
        cout << "data[" << tbmch << "] size " << data[tbmch].size(  )
	     << ", remaining " << rest << endl;
        while( rest > 0 ) {
          dataB.clear(  );
          tb.uDelay( 5000 );
          tb.Daq_Read( dataB, Blocksize, rest, tbmch );
          data[tbmch].insert( data[tbmch].end(  ), dataB.begin(  ),
                              dataB.end(  ) );
          cout << "data[" << tbmch << "] size " << data[tbmch].size(  )
	       << ", remaining " << rest << endl;
        }
      }
      catch( CRpcError & e ) {
        e.What(  );
        return 0;
      }

    } // tbmch

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec;        // seconds since 1.1.1970
    long u3 = tv.tv_usec;       // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

  } // while not done

  cout << "LoopMultiRocAllPixelsCalibrate takes " << dtloop << " s" << endl;
  cout << "Daq_Read takes " << dtread << " s"
    << " = " << 2 * ( data[0].size(  ) +
                      data[1].size(  ) ) / dtread / 1024 /
    1024 << " MiB/s" << endl;

 // read and analyze:

  int PX[16] = { 0 };
  int event = 0;

  if( h11 )
    delete h11;
  h11 = new TH1D( Form( "Responses_Vcal%i_CR%i",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  Form( "Responses at Vcal %i, CtrlReg %i;responses;pixels",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  nTrig + 1, -0.5, nTrig + 0.5 );

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleHitMap",
                  "Module hit map;col;row;hits",
                  8 * 52, -0.5, 8 * 52 - 0.5, 2 * 80, -0.5, 2 * 80 - 0.5 );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis(  )->SetTitleOffset( 1.3 );

  if( h22 )
    delete h22;
  h22 = new TH2D( "ModulePHmap",
                  "Module PH map;col;row;sum PH [ADC]",
                  8 * 52, -0.5, 8 * 52 - 0.5, 2 * 80, -0.5, 2 * 80 - 0.5 );

 // TBM channels:

  bool ldb = 0;

  for( int tbmch = 0; tbmch < 2; ++tbmch ) {

    cout << "DAQ size channel " << tbmch
      << " = " << data[tbmch].size(  ) << " words " << endl;

    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = 8 * tbmch - 1; // will start at 0 or 8

   // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data[tbmch].size(  ); ++i ) {

      int d = data[tbmch].at( i ) & 0xfff; // 12 bits data
      int v = ( data[tbmch].at( i ) >> 12 ) & 0xe; // 3 flag bits

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

       // TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
       //DecodeTbmHeader(hdr);
        if( ldb )
          cout << "event " << setw( 6 ) << event;
        kroc = 8 * tbmch - 1; // new event, will start at 0 or 8
        break;

       // ROC header data:
      case 4:
        kroc++; // start at 0 or 8
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }
        break;

       // pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
       //DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
        c = ( raw >> 12 ) & 7;
        c = c * 6 + ( ( raw >> 9 ) & 7 );
        r = ( raw >> 6 ) & 7;
        r = r * 6 + ( ( raw >> 3 ) & 7 );
        r = r * 6 + ( raw & 7 );
        y = 80 - r / 2;
        x = 2 * c + ( r & 1 );
        if( ldb )
          cout << setw( 3 ) << kroc << setw( 3 )
            << x << setw( 3 ) << y << setw( 4 ) << ph;
        PX[kroc]++;
        { // start a scope to make compiler happy
          int l = kroc % 8;     // 0..7
          int m = kroc / 8;     // 0..1
          int xm = 52 * l + x;  // 0..415  rocs 0 1 2 3 4 5 6 7
          if( m == 1 )
            xm = 415 - xm; // rocs 8 9 A B C D E F
          int ym = 159 * m + ( 1 - 2 * m ) * y; // 0..159
          h21->Fill( xm, ym );
          h22->Fill( xm, ym, ph );
        }
        break;

       // TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
       //DecodeTbmTrailer(trl);
        if( ldb )
          cout << endl;
        if( tbmch == 0 )
          event++;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "TBM ch " << tbmch
	 << " events " << event
	 << " = " << event / nTrig << " pixels / ROC" << endl;

  } // tbmch

 // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask(  );
    tb.roc_ClrCal(  );
  }
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

  Log.flush(  );

  cout << endl;
  for( int roc = 0; roc < 16; ++roc )
    cout << "ROC " << setw( 2 ) << roc << ", hits " << PX[roc]
      << endl;

  for( int ibin = 1; ibin <= h21->GetNbinsX(  ); ++ibin )
    for( int jbin = 1; jbin <= h21->GetNbinsY(  ); ++jbin )
      h11->Fill( h21->GetBinContent( ibin, jbin ) );

  h11->Write(  );
  h21->Write(  );
  h22->Write(  );
  h21->SetMinimum( 0 );
  h21->SetMaximum( nTrig );
  h21->Draw( "colz" );
  c1->Update(  );
  cout << "histos 11, 21, 22" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: measure ROC threshold Vcal map, uses tb.PixelThreshold

void RocThrMap( int roc, int start, int step, int nTrig, int xtalk, int cals )
{
  if( roc < 0 ) {
    cout << "invalid roc " << roc << endl;
    return;
  }
  if( roc > 15 ) {
    cout << "invalid roc " << roc << endl;
    return;
  }

  const int thrLevel = nTrig / 2;

  tb.roc_I2cAddr( roc ); // just to be sure

#ifndef DAQOPENCLOSE
  tb.Daq_Stop(  ); // tb.PixelThreshold starts with Daq_Open
  tb.Daq_Close(  );
  tb.uDelay( 1000 );
#endif

  for( int col = 0; col < 52; ++col ) {

    tb.roc_Col_Enable( col, true );
    cout << setw( 3 ) << col;
    cout.flush(  );

    for( int row = 0; row < 80; ++row ) {

      int trim = modtrm[roc][col][row];
      tb.roc_Pix_Trim( col, row, trim );

      int thr = tb.PixelThreshold( col, row,
                                   start, step,
                                   thrLevel, nTrig,
                                   Vcal, xtalk, cals );

      if( thr == 255 ) // try again
        thr = tb.PixelThreshold( col, row,
                                 start, step,
                                 thrLevel, nTrig, Vcal, xtalk, cals );

      modthr[roc][col][row] = thr;

      tb.roc_Pix_Mask( col, row );

    } // rows

    tb.roc_Col_Enable( col, 0 );

  } // cols

  cout << endl;
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore

#ifndef DAQOPENCLOSE
  tb.Daq_Open( tbState.GetDaqSize(  ) ); // PixelThreshold ends with dclose
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
#endif

  tb.Flush(  );
}

//------------------------------------------------------------------------------
CMD_PROC( thrmap ) // for single ROCs, uses tb.PixelThreshold
{
  int guess;
  PAR_INT( guess, 0, 255 );

  Log.section( "THRMAP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  const int nTrig = 20;

  const int step = 1;
  const int xtalk = 0;
  const int cals = 0;

  int npx[16] = { 0 };

  const int vthr = dacval[0][VthrComp];
  const int vtrm = dacval[0][Vtrim];

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_dist_Vthr%i_Vtrm%i", vthr, vtrm ),
	  Form( "Threshold distribution Vthr %i Vtrim %i;threshold [small Vcal DAC];pixels", vthr, vtrm ),
	  255, -0.5, 254.5 ); // 255 = overflow

  if( h21 )
    delete h21;
  h21 = new TH2D( Form( "ThrMap_Vthr%i_Vtrm%i", vthr, vtrm ),
                  Form
                  ( "Threshold map at Vthr %i, Vtrim %i;col;row;threshold [small Vcal DAC]",
                    vthr, vtrm ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

 // loop over ROCs:

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << setw( 2 ) << "ROC " << roc << endl;

    tb.roc_I2cAddr( roc );
    tb.SetDAC( CtrlReg, 0 );

    RocThrMap( roc, guess, step, nTrig, xtalk, cals ); // fills modthr

    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    tb.Flush(  );

    int nok = 0;
    printThrMap( 1, roc, nok );
    npx[roc] = nok;

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        h11->Fill( modthr[roc][col][row] );
        h21->Fill( col, row, modthr[roc][col][row] );
      }
  } // rocs

  cout << endl;
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] )
      cout << "ROC " << setw( 2 ) << roc << " valid thr " << npx[roc]
        << endl;

  h11->Write(  );
  h21->Write(  );

  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11, 21" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( thrmapsc ) // raw data S-curve: 60 s / ROC
{
  int stop = 255;
  int ils = 0;                  // small Vcal

  if( !PAR_IS_INT( stop, 30, 255 ) ) {
    stop = 255;
    ils = 4; // large Vcal
  }

  int dac = Vcal;

  Log.section( "THRMAPSC", false );
  Log.printf( " stop at %i\n", stop );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  int roc = 0;

  tb.SetDAC( CtrlReg, ils ); // measure at small or large Vcal

  int nTrig = 20;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );
  tb.Daq_Start(  );
  tb.uDelay( 100 );

 // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.uDelay( 1000 );
  tb.Flush(  );

  uint16_t flags = 0;           // normal CAL
  if( ils )
    flags = 0x0002; // FLAG_USE_CALS;
  if( flags > 0 )
    cout << "CALS used..." << endl;

  if( dac == 11 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac == 12 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac == 25 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  int16_t dacLower1 = 0;
  int16_t dacUpper1 = stop;
  int32_t nstp = dacUpper1 - dacLower1 + 1;

 // measure:

  cout << "pulsing 4160 pixels with " << nTrig << " triggers for "
    << nstp << " DAC steps may take " << int ( 4160 * nTrig * nstp * 6e-6 ) +
    1 << " s..." << endl;

 // header = 1 word
 // pixel = +2 words
 // size = 256 dacs * 4160 pix * nTrig * 3 words = 32 MW

  vector < uint16_t > data;
  data.reserve( nstp * nTrig * 4160 * 3 );

  bool done = 0;
  double tloop = 0;
  double tread = 0;

  while( done == 0 ) {

    cout << "loop..." << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec;        // seconds since 1.1.1970
    long u1 = tv.tv_usec;       // microseconds

    try {
      done =
        tb.LoopSingleRocAllPixelsDacScan( roc, nTrig, flags, dac, dacLower1,
                                          dacUpper1 );
     // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
     // loop cols
     //   loop rows
     //     activate this pix on all ROCs
     //     loop dacs
     //       loop trig
    }
    catch( CRpcError & e ) {
      e.What(  );
      return 0;
    }

    cout << "waiting for FPGA..." << endl;
    int dSize = tb.Daq_GetSize(  );

   //tb.Daq_Stop(); // avoid extra (noise) data

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec;        // seconds since 1.1.1970
    long u2 = tv.tv_usec;       // microseconds
    double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;
    tloop += dt;
    cout << "LoopSingleRocAllPixelsDacScan takes " << dt << " s"
      << " = " << tloop / 4160 / nTrig / nstp * 1e6 << " us / pix" << endl;
    cout << ( done ? "done" : "not done" ) << endl;

    cout << "DAQ size " << dSize << " words" << endl;

    vector < uint16_t > dataB;
    dataB.reserve( Blocksize );

    try {
      uint32_t rest;
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(  ), dataB.begin(  ), dataB.end(  ) );
      cout << "data size " << data.size(  )
	   << ", remaining " << rest << endl;
      while( rest > 0 ) {
        dataB.clear(  );
        tb.Daq_Read( dataB, Blocksize, rest );
        data.insert( data.end(  ), dataB.begin(  ), dataB.end(  ) );
        cout << "data size " << data.size(  )
	     << ", remaining " << rest << endl;
      }
    }
    catch( CRpcError & e ) {
      e.What(  );
      return 0;
    }

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec;        // seconds since 1.1.1970
    long u3 = tv.tv_usec;       // microseconds
    double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
    tread += dtr;
    cout << "Daq_Read takes " << tread << " s"
      << " = " << 2 * dSize / tread / 1024 / 1024 << " MiB/s" << endl;

  } // while not done

 // all off:

  for( int col = 0; col < 52; ++col )
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask(  );
  tb.roc_ClrCal(  );
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] );
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "thr_dist",
          "Threshold distribution;threshold [small Vcal DAC];pixels",
          256, -0.5, 255.5 );

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "noisedist",
          "Noise distribution;width of threshold curve [small Vcal DAC];pixels",
          51, -0.5, 50.5 );

  if( h13 )
    delete h13;
  h13 = new
    TH1D( "maxdist",
          "Response distribution;responses;pixels",
          nTrig + 1, -0.5, nTrig + 0.5 );

 // unpack data:

  PixelReadoutData pix;

  int pos = 0;
  int err = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

     //cout << endl << setw(2) << col << setw(3) << row << endl;

      int cnt[256] = { 0 };
      int nmx = 0;

      for( int32_t j = 0; j < nstp; ++j ) { // DAC steps

        uint32_t idc = dacLower1 + j;

        for( int k = 0; k < nTrig; ++k ) {

          err = DecodePixel( data, pos, pix ); // analyzer

          if( err )
            cout << "error " << err << " at trig " << k
              << ", stp " << j
              << ", pix " << col << " " << row << ", pos " << pos << endl;
          if( err )
            break;

          if( pix.n > 0 ) {
            if( pix.x == col && pix.y == row ) {
              cnt[idc]++;
            }
          }

        } // trig

        if( err )
          break;

        if( cnt[idc] > nmx )
          nmx = cnt[idc];

      } // dac

      if( err )
        break;

     // analyze S-curve:

      int i10 = 0;
      int i50 = 0;
      int i90 = 0;
      bool ldb = 0;
      if( col == 22 && row == 22 )
        ldb = 1; // one example pix
      for( int idc = 0; idc < nstp; ++idc ) {
        int ni = cnt[idc];
        if( ldb )
          cout << setw( 3 ) << ni;
        if( ni <= 0.1 * nmx )
          i10 = idc; // -1.28155 sigma
        if( ni <= 0.5 * nmx )
          i50 = idc;
        if( ni <= 0.9 * nmx )
          i90 = idc; // +1.28155 sigma
      }
      if( ldb )
        cout << endl;
      if( ldb )
        cout << "thr " << i50 << ", width " << i90 - i10 << endl;
      modthr[roc][col][row] = i50;
      h11->Fill( i50 );
      h12->Fill( i90 - i10 );
      h13->Fill( nmx );

    } // row

    if( err )
      break;

  } // col

  Log.flush(  );

  int nok = 0;
  printThrMap( 1, roc, nok );

  h11->Write(  );
  h12->Write(  );
  h13->Write(  );
  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11, 12, 13" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
// global:

int rocthr[52][80] = { {0} };
int roctrm[52][80] = { {0} };

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: trim bit correction

void TrimStep( int roc,
               int target, int correction,
               int guess, int step, int nTrig, int xtalk, int cals )
{
  cout << endl
    << "TrimStep for ROC " << roc
    << " with correction " << correction << endl;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      int thr = modthr[roc][col][row];

      rocthr[col][row] = thr; // remember

      int trim = modtrm[roc][col][row];

      roctrm[col][row] = trim; // remember

      if( thr > target && thr < 255 )
        trim -= correction; // make softer
      else
        trim += correction; // make harder

      if( trim < 0 )
        trim = 0;
      if( trim > 15 )
        trim = 15;

      modtrm[roc][col][row] = trim;

    } // rows

  } // cols

 // measure new result:

  RocThrMap( roc, guess, step, nTrig, xtalk, cals ); // fills modthr

  int nok = 0;
  printThrMap( 0, roc, nok );

  for( int col = 0; col < 52; ++col ) {
    for( int row = 0; row < 80; ++row ) {

      int thr = modthr[roc][col][row];
      int old = rocthr[col][row];

      if( abs( thr - target ) > abs( old - target ) ) {
        modtrm[roc][col][row] = roctrm[col][row]; // go back
        modthr[roc][col][row] = old;
      }

    } // rows
  } // cols

}

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 2.6.2014: set ROC global threshold
CMD_PROC( vthrcomp )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  int guess;
  if( !PAR_IS_INT( guess, 0, 255 ) )
    guess = 50;

  Log.section( "VTHRCOMP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  const int nTrig = 20;
  const int thrLevel = nTrig / 2;
  const int step = 1;
  const int xtalk = 0;
  const int cals = 0;

  size_t roc = 0;

  tb.roc_I2cAddr( roc );

  int vthr = dacval[roc][VthrComp];
  int vtrm = dacval[roc][Vtrim];

  cout << endl
       << setw( 2 ) << "ROC " << roc
       << ", VthrComp " << vthr << ", Vtrim " << vtrm << endl;

  tb.SetDAC( CtrlReg, 0 ); // this ROC, small Vcal

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // take threshold map, find softest pixel

  cout << "measuring Vcal threshold map from guess " << guess << endl;

  RocThrMap( roc, guess, step, nTrig, xtalk, cals ); // fills modthr

  int nok = 0;
  printThrMap( 0, roc, nok );

  if( h10 )
    delete h10;
  h10 = new
    TH1D( Form( "thr_dist_Vthr%i_Vtrm%i", vthr, vtrm ),
	  Form( "Threshold distribution Vthr %i Vtrim %i;threshold [small Vcal DAC];pixels", vthr, vtrm ),
	  255, -0.5, 254.5 ); // 255 = overflow

  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row )
      h10->Fill( modthr[roc][col][row] );

  h10->Draw(  );
  c1->Update(  );
  h10->Write(  );

  int vmin = 255;
  int colmin = -1;
  int rowmin = -1;

  for( int row = 79; row >= 0; --row )
    for( int col = 0; col < 52; ++col ) {

      int thr = modthr[roc][col][row];
      if( thr < vmin ) {
	vmin = thr;
	colmin = col;
	rowmin = row;
      }
    } // cols

  cout << "min thr " << vmin << " at " << colmin << " " << rowmin << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // setVthrComp using min pixel:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_vs_VthrComp_col%i_row%i", colmin, rowmin ),
	  Form
	  ( "Threshold vs VthrComp pix %i %i target %i;VthrComp [DAC];threshold [small Vcal DAC]",
	    colmin, rowmin, target ), 200, 0, 200 );

  // VthrComp has negative slope: higher = softer

  int vstp = -1; // towards harder threshold
  if( vmin > target )
    vstp = 1; // towards softer threshold

  tb.roc_Col_Enable( colmin, true );
  int tbits = modtrm[roc][colmin][rowmin];
  tb.roc_Pix_Trim( colmin, rowmin, tbits );

#ifndef DAQOPENCLOSE
  tb.Daq_Stop(  ); // tb.PixelThreshold starts with Daq_Open
  tb.Daq_Close(  );
  tb.uDelay( 1000 );
#endif

  tb.Flush(  );

  guess = vmin;

  for( ; vthr < 255 && vthr > 0; vthr += vstp ) {

    tb.SetDAC( VthrComp, vthr );
    tb.uDelay( 1000 );
    tb.Flush(  );

    int thr =
      tb.PixelThreshold( colmin, rowmin,
			 guess, step,
			 thrLevel, nTrig,
			 Vcal, xtalk, cals );

    h11->Fill( vthr, thr );
    cout << setw( 3 ) << vthr << setw( 4 ) << thr << endl;

    if( vstp * thr <= vstp * target ) // signed
      break;

  } // vthr

  h11->Draw(  );
  c1->Update(  );
  h11->Write(  );

  vthr -= 1; // margin towards harder threshold
  tb.SetDAC( VthrComp, vthr );
  cout << "set VthrComp to " << vthr << endl;
  dacval[roc][VthrComp] = vthr;

  tb.roc_Pix_Mask( colmin, rowmin );
  tb.roc_Col_Enable( colmin, 0 );
  tb.roc_ClrCal(  );
  tb.Flush(  );

  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.Flush(  );
  cout << "roc " << roc << " back to Vcal " << dacval[roc][Vcal]
       << ", CtrlReg " << dacval[roc][CtrlReg]
       << endl;

#ifndef DAQOPENCLOSE
  tb.Daq_Open( tbState.GetDaqSize(  ) ); // PixelThreshold ends with dclose
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.Flush(  );
#endif

  cout << "histos 10,11" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: trim procedure, 83 s / ROC
CMD_PROC( trim )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  int guess;
  if( !PAR_IS_INT( guess, 0, 255 ) )
    guess = 50;

  Log.section( "TRIM", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  const int nTrig = 20;
  const int thrLevel = nTrig / 2;
  const int step = 1;
  const int xtalk = 0;
  const int cals = 0;

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );

    const int vthr = dacval[roc][VthrComp];
    int vtrm = dacval[roc][Vtrim];

    cout << endl
      << setw( 2 ) << "ROC " << roc << ", VthrComp " << vthr << endl;

    tb.SetDAC( CtrlReg, 0 ); // this ROC, small Vcal

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // take threshold map, find hardest pixel

    int tbits = 15;             // 15 = off

    for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col )
        modtrm[roc][col][row] = tbits;

    cout << "measuring Vcal threshold map from guess " << guess << endl;

    RocThrMap( roc, guess, step, nTrig, xtalk, cals ); // fills modthr

    int nok = 0;
    printThrMap( 0, roc, nok );

    if( h10 )
      delete h10;
    h10 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i", vthr, vtrm, tbits ),
	    Form( "Threshold distribution Vthr %i Vtrim %i bits %i;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ),
	    255, -0.5, 254.5 ); // 255 = overflow

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h10->Fill( modthr[roc][col][row] );

    h10->Draw(  );
    c1->Update(  );
    h10->Write(  );

    cout << endl;

    if( nok < 1 ) {
      cout << "skip ROC " << roc << " (wrong CalDel ?)" << endl;
      continue; // skip this ROC
    }

    int vmin = 255;
    int vmax = 0;
    int colmax = -1;
    int rowmax = -1;

    for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {

        int thr = modthr[roc][col][row];
        if( thr > vmax && thr < 255 ) {
          vmax = thr;
          colmax = col;
          rowmax = row;
        }
        if( thr < vmin )
          vmin = thr;
      } // cols

    if( vmin < target ) {
      cout << "min threshold below target on ROC " << roc
        << ": skipped (try lower VthrComp)" << endl;
      continue; // skip this ROC
    }

    cout << "max thr " << vmax << " at " << colmax << " " << rowmax << endl;

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // setVtrim using max pixel:

    tbits = 0; // 0 = strongest

    guess = target;

    tb.roc_Col_Enable( colmax, true );

    if( h11 )
      delete h11;
    h11 = new
      TH1D( Form( "thr_vs_Vtrim_col%i_row%i", colmax, rowmax ),
            Form
            ( "Threshold vs Vtrim pix %i %i;Vtrim [DAC];threshold [small Vcal DAC]",
              colmax, rowmax ), 128, 0, 256 );

#ifndef DAQOPENCLOSE
    tb.Daq_Stop(  ); // tb.PixelThreshold starts with Daq_Open
    tb.Daq_Close(  );
    tb.uDelay( 1000 );
#endif
    tb.Flush(  );

    vtrm = 1;
    for( ; vtrm < 253; vtrm += 2 ) {

      tb.SetDAC( Vtrim, vtrm );
      tb.uDelay( 100 );
      tb.Flush(  );

      tb.roc_Pix_Trim( colmax, rowmax, tbits );
      int thr = tb.PixelThreshold( colmax, rowmax,
                                   guess, step,
                                   thrLevel, nTrig,
                                   Vcal, xtalk, cals );

      h11->Fill( vtrm, thr );
     //cout << setw(3) << vtrm << "  " << setw(3) << thr << endl;
      if( thr < target )
        break;
    } // vtrm

    h11->Draw(  );
    c1->Update(  );
    h11->Write(  );

    vtrm += 2; // margin
    tb.SetDAC( Vtrim, vtrm );
    cout << "set Vtrim to " << vtrm << endl;
    dacval[roc][Vtrim] = vtrm;

    tb.roc_Pix_Mask( colmax, rowmax );
    tb.roc_Col_Enable( colmax, 0 );
    tb.roc_ClrCal(  );
    tb.Flush(  );

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // iterate trim bits:

    tbits = 7; // half way

    for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {
        rocthr[col][row] = modthr[roc][col][row]; // remember, BUG fixed
        roctrm[col][row] = modtrm[roc][col][row]; // old
        modtrm[roc][col][row] = tbits;
      }

    cout << endl << "measuring Vcal threshold map with trim 7" << endl;

    RocThrMap( roc, guess, step, nTrig, xtalk, cals ); // uses modtrm, fills modthr

    printThrMap( 0, roc, nok );

    if( h12 )
      delete h12;
    h12 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i", vthr, vtrm, tbits ),
	    Form( "Threshold distribution Vthr %i Vtrim %i bits %i;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ),
	    255, -0.5, 254.5 ); // 255 = overflow

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h12->Fill( modthr[roc][col][row] );

    h12->Draw(  );
    c1->Update(  );
    h12->Write(  );

    int correction = 4;

    TrimStep( roc, target, correction, guess, step, nTrig, xtalk, cals ); // fills modtrm, fills modthr

    if( h13 )
      delete h13;
    h13 = new TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_4", vthr, vtrm, tbits ), Form( "Threshold distribution Vthr %i Vtrim %i bits %i 4;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ), 255, -0.5, 254.5 ); // 255 = overflow

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h13->Fill( modthr[roc][col][row] );

    h13->Draw(  );
    c1->Update(  );
    h13->Write(  );

    correction = 2;

    TrimStep( roc, target, correction, guess, step, nTrig, xtalk, cals ); // fills modtrm, fills modthr

    if( h14 )
      delete h14;
    h14 = new TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_4_2", vthr, vtrm, tbits ), Form( "Threshold distribution Vthr %i Vtrim %i bits %i 4 2;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ), 255, -0.5, 254.5 ); // 255 = overflow

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h14->Fill( modthr[roc][col][row] );

    h14->Draw(  );
    c1->Update(  );
    h14->Write(  );

    correction = 1;

    TrimStep( roc, target, correction, guess, step, nTrig, xtalk, cals ); // fills modtrm, fills modthr

    if( h15 )
      delete h15;
    h15 = new TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_4_2_1", vthr, vtrm, tbits ), Form( "Threshold distribution Vthr %i Vtrim %i bits %i 4 2 1;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ), 255, -0.5, 254.5 ); // 255 = overflow

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h15->Fill( modthr[roc][col][row] );

    h15->Draw(  );
    c1->Update(  );
    h15->Write(  );

    correction = 1;

    TrimStep( roc, target, correction, guess, step, nTrig, xtalk, cals ); // fills modtrm, fills modthr

    if( h16 )
      delete h16;
    h16 = new TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_4_2_1_1", vthr, vtrm, tbits ), Form( "Threshold distribution Vthr %i Vtrim %i bits %i 4 2 1 1;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ), 255, -0.5, 254.5 ); // 255 = overflow

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h16->Fill( modthr[roc][col][row] );

    h16->Draw(  );
    c1->Update(  );
    h16->Write(  );

    vector < uint8_t > trimvalues( 4160 ); // uint8 in 2.15
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        int i = 80 * col + row;
        trimvalues[i] = roctrm[col][row];
      }
    tb.SetTrimValues( roc, trimvalues );
    cout << "SetTrimValues in FPGA" << endl;

    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
    tb.Flush(  );
    cout << "roc " << roc << " back to Vcal " << dacval[roc][Vcal]
	 << ", CtrlReg " << dacval[roc][CtrlReg]
	 << endl;

  } // rocs

#ifndef DAQOPENCLOSE
  tb.Daq_Open( tbState.GetDaqSize(  ) ); // PixelThreshold ends with dclose
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.Flush(  );
#endif

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 30.5.2014: fine tune trim bits (full efficiency)
CMD_PROC( trimbits )
{
  Log.section( "TRIMBITS", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  const int nTrig = 100;

  size_t roc = 0;

  if( roclist[roc] == 0 )
    return 0;

  tb.roc_I2cAddr( roc );

  cout << endl
    << setw( 2 ) << "ROC " << roc << ", VthrComp " << dacval[roc][VthrComp]
    << ", Vtrim " << dacval[roc][Vtrim]
    << endl;

 // measure efficiency map:

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( nTrig, nReadouts, PHavg, PHrms );

 // analyze:

  size_t j = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80 && j < nReadouts.size(  ); ++row ) {

      int cnt = nReadouts.at( j );
      ++j;

      if( cnt == nTrig )
        continue; // perfect

      int trm = modtrm[roc][col][row];

      cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
        << " trimbits " << trm << " responses " << cnt << endl;

      if( trm == 15 )
        continue;

     // increase trim bits:

      trm++;

      for( ; trm <= 15; ++trm ) {

        modtrm[roc][col][row] = trm;

        vector < uint8_t > trimvalues( 4160 );

        for( int ix = 0; ix < 52; ix++ )
          for( int iy = 0; iy < 80; iy++ ) {
            int i = 80 * ix + iy;
            trimvalues[i] = modtrm[roc][ix][iy];
          } // iy
        tb.SetTrimValues( roc, trimvalues ); // load into FPGA
        tb.Flush(  );

        double ph;
        double rms;
        GetPixData( roc, col, row, nTrig, cnt, ph, rms );

        cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
          << " trimbits " << trm << " responses " << cnt << endl;

        if( cnt == nTrig )
          break;

      } // trm

    } // row

  } // col

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tune ) // adjust PH gain and offset to fit into ADC range
{
  int roc = 0;
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  //tb.SetDAC( CtrlReg, 4 ); // large Vcal
  //tb.Flush(  );

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // scan Vcal for one pixel

  int dac = Vcal;
  int16_t nTrig = 9;

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

  if( nReadouts.size(  ) < 256 ) {
    cout << "only " << nReadouts.size(  ) << " Vcal points"
	 << ". choose different pixel, check CalDel, check Ia, or give up"
	 << endl;
    return 0;
  }

  if( nReadouts.at( nReadouts.size(  ) - 1 ) < nTrig ) {
    cout << "only " << nReadouts.at( nReadouts.size(  ) - 1 )
	 << " responses at " << nReadouts.size(  ) - 1
	 << ". choose different pixel, check CalDel, check Ia, or give up" <<
      endl;
    return 0;
  }

 // scan from end, search for smallest responding Vcal:

  int minVcal = 255;

  for( int idac = nReadouts.size(  ) - 1; idac >= 0; --idac )
    if( nReadouts.at( idac ) == nTrig )
      minVcal = idac;
  cout << "min responding Vcal " << minVcal << endl;
  minVcal += 1; // safety

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // offset and gain DACs:

  int offsdac = VoffsetOp;      // digV2, positive slope
  if( Chip >= 400 )
    offsdac = VoffsetRO; // digV2.1, positive slope

  const int gaindac = VIref_ADC;

  cout << "start offset dac " << offsdac << " at " << dacval[roc][offsdac] <<
    endl;
  cout << "start gain   dac " << gaindac << " at " << dacval[roc][gaindac] <<
    endl;

 // set gain to minimal (at DAC 255), to avoid overflow or underflow:

  tb.SetDAC( gaindac, 255 );
  tb.Flush(  );

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // Scan PH vs offset at Vcal 255:

  tb.SetDAC( Vcal, 255 ); // max Vcal
  vector < double >PHmax;
  PHmax.reserve( 256 );
  nReadouts.clear(  );
  PHrms.clear(  );
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmax, PHrms );

 // Scan PH vs offset at minVcal:

  tb.SetDAC( Vcal, minVcal );
  vector < double >PHmin;
  PHmin.reserve( 256 );
  nReadouts.clear(  );
  PHrms.clear(  );
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmin, PHrms );

 // use offset to center PH at 132:

  int offs = 0;
  double phmid = 0;
  for( size_t idac = 0; idac < PHmin.size(  ); ++idac ) {
    double phmean = 0.5 * ( PHmin.at( idac ) + PHmax.at( idac ) );
    if( fabs( phmean - 132 ) < fabs( phmid - 132 ) ) {
      offs = idac;
      phmid = phmean;
    }
  }

  cout << "mid PH " << phmid << " at offset " << offs << endl;

  tb.SetDAC( offsdac, offs );
  tb.Flush(  );

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // scan PH vs gain at Vcal 255 and at minVcal

  tb.SetDAC( Vcal, 255 );
  vector < double >PHtop;
  PHtop.reserve( 256 );
  nReadouts.clear(  );
  PHrms.clear(  );

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHtop, PHrms );

  tb.SetDAC( Vcal, minVcal );
  vector < double >PHbot;
  PHbot.reserve( 256 );
  nReadouts.clear(  );
  PHrms.clear(  );

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHbot, PHrms );

 // set gain:

  int gain = PHtop.size(  ) - 1;
  for( ; gain >= 1; --gain ) {
    if( PHtop.at( gain ) > 233 )
      break;
    if( PHbot.at( gain ) < 22 )
      break;
  }
  cout << "top PH " << PHtop.at( gain )
    << " at gain " << gain
    << " for Vcal 255" << " for pixel " << col << " " << row << endl;
  cout << "bot PH " << PHbot.at( gain )
    << " at gain " << gain
    << " for Vcal " << minVcal << " for pixel " << col << " " << row << endl;

  tb.SetDAC( gaindac, gain );
  tb.Flush(  );

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // check for all pixels

  vector < int16_t > nResponses; // size 0
  vector < double >QHmax;
  vector < double >QHrms;
  nResponses.reserve( 4160 ); // size 0, capacity 4160
  QHmax.reserve( 4160 );
  QHrms.reserve( 4160 );

  tb.SetDAC( Vcal, 255 );
  tb.Flush(  );

  bool again = 0;

  int colmax = 0;
  int rowmax = 0;

  do { // no overflows

    GetRocData( nTrig, nResponses, QHmax, QHrms );

    size_t j = 0;
    double phmax = 0;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        double ph = QHmax.at( j );
        if( ph > phmax ) {
          phmax = ph;
          colmax = col;
          rowmax = row;
        }

        ++j;
        if( j == nReadouts.size(  ) )
          break;

      } // row

      if( j == nReadouts.size(  ) )
        break;

    } // col

    cout << "max PH " << phmax << " at " << colmax << " " << rowmax << endl;

    if( phmax > 252 && gain < 255 ) {

      gain += 1; // reduce gain
      tb.SetDAC( gaindac, gain );
      tb.Flush(  );

      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;

      again = 1;
      nResponses.clear(  );
      QHmax.clear(  );
      QHrms.clear(  );

    }
    else
      again = 0;

  } while( again );

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // check all pixels for underflow at minVcal:

  vector < double >QHmin;
  QHmin.reserve( 4160 );
  nResponses.clear(  );
  QHrms.clear(  );

  tb.SetDAC( Vcal, minVcal );
  tb.Flush(  );

  again = 0;

  int colmin = 0;
  int rowmin = 0;

  do { // no underflows

    GetRocData( nTrig, nResponses, QHmin, QHrms );

    size_t j = 0;
    double phmin = 255;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        if( nResponses.at( j ) < nTrig / 2 ) {
          cout << "pixel " << col << " " << row << " below threshold at Vcal "
            << minVcal << endl;
        }
        else {
          double ph = QHmin.at( j );
          if( ph < phmin ) {
            phmin = ph;
            colmin = col;
            rowmin = row;
          }
        }

        ++j;
        if( j == nReadouts.size(  ) )
          break;

      } // row

      if( j == nReadouts.size(  ) )
        break;

    } // col

    cout << "min PH " << phmin << " at " << colmin << " " << rowmin << endl;

    if( phmin < 3 && gain > 0 ) {

      gain += 1; // reduce gain
      tb.SetDAC( gaindac, gain );
      tb.Flush(  );

      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;

      again = 1;
      nResponses.clear(  );
      QHmin.clear(  );
      QHrms.clear(  );

    }
    else
      again = 0;

  } while( again );

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // we have min and max.
 // adjust gain and offset such that max-min = 200, mid = 132

  again = 0;

  do {

    int cnt;
    double rms;

    tb.SetDAC( Vcal, minVcal );
    tb.Flush(  );
    double phmin = 255;
    GetPixData( roc, colmin, rowmin, nTrig, cnt, phmin, rms );

    tb.SetDAC( Vcal, 255 );
    tb.Flush(  );
    double phmax = 0;
    GetPixData( roc, colmax, rowmax, nTrig, cnt, phmax, rms );

    again = 0;

    double phmid = 0.5 * ( phmax + phmin );

    cout << "PH mid " << phmid << endl;

    if( phmid > 132 + 3 && offs > 0 ) { // 3 is margin of convergence
      offs -= 1;
      again = 1;
    }
    else if( phmid < 132 - 3 && offs < 255 ) {
      offs += 1;
      again = 1;
    }
    if( again ) {
      tb.SetDAC( offsdac, offs );
      tb.Flush(  );
      cout << "offs dac " << dacName[offsdac] << " set to " << offs << endl;
    }

    double range = phmax - phmin;

    cout << "PH rng " << range << endl;

    if( range > 200 + 3 && gain < 255 ) { // 3 is margin of convergence
      gain += 1; // reduce gain
      again = 1;
    }
    else if( range < 200 - 3 && gain > 0 ) {
      gain -= 1; // more gain
      again = 1;
    }
    if( again ) {
      tb.SetDAC( gaindac, gain );
      tb.Flush(  );
      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;
    }
  } while( again );

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // done

  cout << "final offs dac " << dacName[offsdac] << " set to " << offs << endl;
  cout << "final gain dac " << dacName[gaindac] << " set to " << gain << endl;

  dacval[roc][offsdac] = offs;
  dacval[roc][gaindac] = gain;

  Log.printf( "[SETDAC] %i  %i\n", gaindac, gain );
  Log.printf( "[SETDAC] %i  %i\n", offsdac, offs );
  Log.flush(  );

  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.Flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( phmap ) // check gain tuning and calibration
{
  int nTrig;
  PAR_INT( nTrig, 1, 65000 );

  const int vctl = dacval[0][CtrlReg];
  const int vcal = dacval[0][Vcal];

  Log.section( "PHMAP", false );
  Log.printf( " CtrlReg %i Vcal %i nTrig %i\n", vctl, vcal, nTrig );
  cout << "PHmap with " << nTrig << " triggers"
    << " at Vcal " << vcal << ", CtrlReg " << vctl << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( nTrig, nReadouts, PHavg, PHrms );

 // analyze:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "ph_dist_Vcal%i_CR%i", vcal, vctl ),
          Form( "PH distribution at Vcal %i, CtrlReg %i;<PH> [ADC];pixels",
                vcal, vctl ), 256, -0.5, 255.5 );

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "vcal_dist_Vcal%i_CR%i", vcal, vctl ),
          Form( dacval[0][CtrlReg] == 0 ?
                "PH Vcal distribution at Vcal %i, CtrlReg %i;calibrated PH [small Vcal DAC];pixels"
                :
                "PH Vcal distribution at Vcal %i, CtrlReg %i;calibrated PH [large Vcal DAC];pixels",
                vcal, vctl ), 256, 0, 256 );

  if( h13 )
    delete h13;
  h13 = new
    TH1D( Form( "rms_dist_Vcal%i_CR%i", vcal, vctl ),
          Form
          ( "PH RMS distribution at Vcal %i, CtrlReg %i;PH RMS [ADC];pixels",
            vcal, vctl ), 100, 0, 2 );

  if( h21 )
    delete h21;
  h21 = new TH2D( Form( "PHmap_Vcal%i_CR%i", vcal, vctl ),
                  Form( "PH map at Vcal %i, CtrlReg %i;col;row;<PH> [ADC]",
                        vcal, vctl ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h22 )
    delete h22;
  h22 = new TH2D( Form( "VcalMap_Vcal%i_CR%i", vcal, vctl ),
                  Form( dacval[0][CtrlReg] == 0 ?
                        "PH Vcal map at Vcal %i, CtrlReg %i;col;row;calibrated <PH> [small Vcal DAC]"
                        :
                        "PH Vcal map at Vcal %i, CtrlReg %i;col;row;calibrated <PH> [large Vcal DAC]",
                        vcal, vctl ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h23 )
    delete h23;
  h23 = new TH2D( Form( "RMSmap_Vcal%i_CR%i", vcal, vctl ),
                  Form( "RMS map at Vcal %i, CtrlReg %i;col;row;PH RMS [ADC]",
                        vcal, vctl ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  int nok = 0;
  double sum = 0;
  double su2 = 0;
  double phmin = 256;
  double phmax = -1;

  size_t j = 0;

  for( int col = 0; col < 52; ++col ) {

    cout << endl << setw( 2 ) << col << " ";

    for( int row = 0; row < 80; ++row ) {

      int cnt = nReadouts.at( j );
      double ph = PHavg.at( j );
      double rms = PHrms.at( j );

      cout << setw( 4 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
        <<"(" << setw( 3 ) << cnt << ")";
      if( row % 10 == 9 )
        cout << endl << "   ";

      Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );

      if( cnt > 0 ) {
        nok++;
        sum += ph;
        su2 += ph * ph;
        if( ph < phmin )
          phmin = ph;
        if( ph > phmax )
          phmax = ph;
        h11->Fill( ph );
        double vc = PHtoVcal( ph, col, row );
        h12->Fill( vc );
        h13->Fill( rms );
        h21->Fill( col, row, ph );
        h22->Fill( col, row, vc );
        h23->Fill( col, row, rms );
      }

      ++j;
      if( j == nReadouts.size(  ) )
        break;

    } // row

    Log.printf( "\n" );
    if( j == nReadouts.size(  ) )
      break;

  } // col

  cout << endl;

  h11->Write(  );
  h12->Write(  );
  h13->Write(  );
  h21->Write(  );
  h22->Write(  );
  h23->Write(  );
  cout << "histos 11, 12, 13, 21, 22" << endl;

  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw(  );
  h12->SetLineColor( 2 );
  h12->SetMarkerColor( 2 );
  h12->Draw( "same" );
  c1->Update(  );

  cout << endl;
  cout << "CtrlReg " << dacval[0][CtrlReg]
    << ", Vcal " << dacval[0][Vcal]
    << endl;

  cout << "Responding pixels " << nok << endl;
  if( nok > 0 ) {
    cout << "PH min " << phmin << ", max " << phmax << endl;
    double mid = sum / nok;
    double rms = sqrt( su2 / nok - mid * mid );
    cout << "PH mean " << mid << ", rms " << rms << endl;
  }
  Log.flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( calsmap ) // test PH map through sensor
{
  int nTrig;
  PAR_INT( nTrig, 1, 65000 );

  Log.section( "CALSMAP", false );
  Log.printf( " CtrlReg %i Vcal %i nTrig %i\n",
              dacval[0][CtrlReg], dacval[0][Vcal], nTrig );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( -nTrig, nReadouts, PHavg, PHrms ); // -nTRig = CALS flag

 // analyze:

  if( h11 )
    delete h11;
  h11 = new TH1D( "phroc", "PH distribution;<PH> [ADC];pixels", 256, 0, 256 );

  int nok = 0;
  double sum = 0;
  double su2 = 0;
  double phmin = 256;
  double phmax = -1;

  size_t j = 0;

  for( int col = 0; col < 52; ++col ) {
    cout << endl << setw( 2 ) << col << " ";
    for( int row = 0; row < 80 && j < nReadouts.size(  ); ++row ) {
      int cnt = nReadouts.at( j );
      double ph = PHavg.at( j );
      cout << setw( 4 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
        <<"(" << setw( 3 ) << cnt << ")";
      if( row % 10 == 9 )
        cout << endl << "   ";
      Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );
      if( cnt > 0 ) {
        nok++;
        sum += ph;
        su2 += ph * ph;
        if( ph < phmin )
          phmin = ph;
        if( ph > phmax )
          phmax = ph;
        h11->Fill( ph );
      }
      ++j;
    } // row
    Log.printf( "\n" );
  } // col

  h11->Write(  );
  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11" << endl;

  cout << endl;
  cout << "CtrlReg " << dacval[0][CtrlReg]
    << ", Vcal " << dacval[0][Vcal]
    << endl;

  cout << "Responding pixels " << nok << endl;
  if( nok > 0 ) {
    cout << "PH min " << phmin << ", max " << phmax << endl;
    double mid = sum / nok;
    double rms = sqrt( su2 / nok - mid * mid );
    cout << "PH mean " << mid << ", rms " << rms << endl;
  }
  Log.flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( bbtest ) // bump bond test
{
  int nTrig;
  PAR_INT( nTrig, 1, 65000 );

  Log.section( "BBTEST", false );
  Log.printf( " nTrig %i\n", nTrig );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  const int vctl = dacval[0][CtrlReg];
  tb.SetDAC( CtrlReg, 4 ); // large Vcal

  const int vcal = dacval[0][Vcal];
  tb.SetDAC( Vcal, 255 ); // max Vcal

  tb.Flush(  );

 // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( -nTrig, nReadouts, PHavg, PHrms ); // -nTRig = CALS flag

  tb.SetDAC( Vcal, vcal ); // restore
  tb.SetDAC( CtrlReg, vctl );
  tb.Flush(  );

 // analyze:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "calsPH",
          "Cals PH distribution;Cals PH [ADC];pixels", 256, 0, 256 );

  if( h21 )
    delete h21;
  h21 = new TH2D( "BBTest",
                  "Cals response map;col;row;responses / pixel",
                  52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h22 )
    delete h22;
  h22 = new TH2D( "CalsPHmap",
                  "Cals PH map;col;row;Cals PH [ADC]",
                  52, -0.5, 51.5, 80, -0.5, 79.5 );

  int nok = 0;
  int nActive = 0;
  int nPerfect = 0;

  double sum = 0;
  double su2 = 0;
  double phmin = 256;
  double phmax = -1;

  size_t j = 0;                 // index in data vectors

  for( int col = 0; col < 52; ++col ) {
    cout << endl << setw( 2 ) << col << " ";
    for( int row = 0; row < 80 && j < nReadouts.size(  ); ++row ) {
      int cnt = nReadouts.at( j );
      double ph = PHavg.at( j );
      cout << setw( 3 ) << cnt
        << "(" << setw( 3 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
        <<")";
      if( row % 10 == 9 )
        cout << endl << "   ";

     //Log.printf( " %i", (ph > -0.1 ) ? int(ph+0.5) : -1 );
      Log.printf( " %i", cnt );

      if( cnt > 0 ) {
        nok++;
        if( cnt >= nTrig / 2 )
          nActive++;
        if( cnt == nTrig )
          nPerfect++;
        sum += ph;
        su2 += ph * ph;
        if( ph < phmin )
          phmin = ph;
        if( ph > phmax )
          phmax = ph;
        h11->Fill( ph );
        h21->Fill( col, row, cnt );
        h22->Fill( col, row, ph );
      }
      else {
        cout << "no response from " << setw( 2 ) << col
          << setw( 3 ) << row << endl;
      }
      ++j;
    } // row
    Log.printf( "\n" );
  } // col
  Log.flush(  );

  h11->Write(  );
  h21->Write(  );
  h22->Write(  );
  cout << "histos 11, 21, 22" << endl;

  h21->SetStats( 0 );
  h21->GetYaxis(  )->SetTitleOffset( 1.3 );
  h21->SetMinimum( 0 );
  h21->SetMaximum( nTrig );
  h21->Draw( "colz" );
  c1->Update(  );

  cout << endl;
  cout << "Cals PH test: " << endl;
  cout << " >0% pixels: " << nok << endl;
  cout << ">50% pixels: " << nActive << endl;
  cout << "100% pixels: " << nPerfect << endl;

  Log.printf( "[Bump Statistic] \n" );
  Log.printf( " >0 pixels %i \n", nok );
  Log.printf( " >50 pixels %i \n", nActive );
  Log.printf( " >100 pixels %i \n", nPerfect );

  if( nok > 0 ) {
    cout << "PH min " << phmin << ", max " << phmax << endl;
    double mid = sum / nok;
    double rms = sqrt( su2 / nok - mid * mid );
    cout << "PH mean " << mid << ", rms " << rms << endl;
  }

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
// utility function: loop over ROC 0 pixels, give triggers, count responses
// user: enable all

int GetEff( int &n01, int &n50, int &n99 )
{
  n01 = 0;
  n50 = 0;
  n99 = 0;

  uint16_t nTrig = 10;
  uint16_t flags = 0;           // normal CAL

 //flags = 0x0002; // FLAG_USE_CALS;
  if( flags > 0 )
    cout << "CALS used..." << endl;

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  try {
    tb.LoopSingleRocAllPixelsCalibrate( 0, nTrig, flags );
  }
  catch( CRpcError & e ) {
    e.What(  );
    return 0;
  }

 // header = 1 word
 // pixel = +2 words
 // size = 4160 * nTrig * 3 = 124'800 words

  tb.Daq_Stop(  );

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize(  ) );

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest );
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(  ), dataB.begin(  ), dataB.end(  ) );
      dataB.clear(  );
    }
  }
  catch( CRpcError & e ) {
    e.What(  );
    return 0;
  }

 // unpack data:

  PixelReadoutData pix;

  int pos = 0;
  int err = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      int cnt = 0;

      for( int k = 0; k < nTrig; ++k ) {

        err = DecodePixel( data, pos, pix ); // analyzer

        if( err )
          cout << "error " << err << " at trig " << k
            << ", pix " << col << " " << row << ", pos " << pos << endl;
        if( err )
          break;

        if( pix.n > 0 )
          if( pix.x == col && pix.y == row )
            cnt++;

      } // trig

      if( err )
        break;
      if( cnt > 0 )
        n01++;
      if( cnt >= nTrig / 2 )
        n50++;
      if( cnt == nTrig )
        n99++;

    } // row

    if( err )
      break;

  } // col

  return 1;
}

//------------------------------------------------------------------------------
CMD_PROC( caldelroc ) // scan and set CalDel using all pixel: 17 s
{
  Log.section( "CALDELROC", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // scan caldel:

  int val = dacval[0][CalDel];

  const int pln = 256;
  int plx[pln] = { 0 };
  int pl1[pln] = { 0 };
  int pl5[pln] = { 0 };
  int pl9[pln] = { 0 };

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "caldel_act", "CalDel ROC scan;CalDel [DAC];active pixels", 128, -1,
          255 );

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "caldel_per", "CalDel ROC scan;CalDel [DAC];perfect pixels", 128,
          -1, 255 );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize ); // 2^24
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );

 // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.uDelay( 1000 );
  tb.Flush(  );

  int n0 = 0;
  int idel = 10;
  int step = 10;

  do {

    tb.SetDAC( CalDel, idel );
    tb.Daq_Start(  );
    tb.uDelay( 1000 );
    tb.Flush(  );

    int n01, n50, n99;

    GetEff( n01, n50, n99 );

    plx[idel] = idel;
    pl1[idel] = n01;
    pl5[idel] = n50;
    pl9[idel] = n99;

    h11->Fill( idel, n50 );
    h12->Fill( idel, n99 );

    cout << setw( 3 ) << idel
      << setw( 6 ) << n01 << setw( 6 ) << n50 << setw( 6 ) << n99 << endl;
    Log.printf( "%i %i %i %i\n", idel, n01, n50, n99 );

    if( n50 > 0 ) {
      if( step == 10 ) {
        step = 4;
        idel -= 10; // go back and measure finer
        n0 = 0;
      }
      else if( step == 4 ) {
        step = 2;
        idel -= 4; // go back and measure finer
        n0 = 0;
      }
    }

    if( n01 < 4 && step == 2 )
      n0++; // count empty responses

    idel += step;

  }
  while( n0 < 4 && idel < 256 );

  cout << endl;
  Log.flush(  );

 // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask(  );
  tb.roc_ClrCal(  );
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

 // compress and sort:

  int j = 0;
  for( int idel = 0; idel < 256; ++idel ) {
    if( pl1[idel] == 0 )
      continue;
    plx[j] = idel;
    pl1[j] = pl1[idel];
    pl5[j] = pl5[idel];
    pl9[j] = pl9[idel];
    ++j;
  }

  int nstp = j;
  cout << j << " points" << endl;

  h11->Write(  );
  h12->Write(  );
  if( par.isInteractive(  ) ) {
    h11->SetStats( 0 );
    h12->SetStats( 0 );
    h12->Draw(  );
    c1->Update(  );
  }
  cout << "histos 11, 12" << endl;

 // analyze:

  int nmax = 0;
  int i0 = 0;
  int i9 = 0;

  for( int j = 0; j < nstp; ++j ) {

    int idel = plx[j];
   //int nn = pl5[j]; // 50% active
    int nn = pl9[j];            // 100% perfect
    if( nn > nmax ) {
      nmax = nn;
      i0 = idel; // begin
    }
    if( nn == nmax )
      i9 = idel; // plateau

  } // dac

  cout << "eff plateau from " << i0 << " to " << i9 << endl;

  if( nmax > 55 ) {
    int i2 = i0 + ( i9 - i0 ) / 4;
    tb.SetDAC( CalDel, i2 );
    DO_FLUSH;
    dacval[0][CalDel] = i2;
    printf( "set CalDel to %i\n", i2 );
    Log.printf( "[SETDAC] %i  %i\n", CalDel, i2 );
  }
  else
    tb.SetDAC( CalDel, val ); // back to default

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vanaroc ) // scan and set Vana using all pixel: 17 s
{
  Log.section( "VANAROC", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

 // scan vana:

  int val = dacval[0][Vana];

  const int pln = 256;
  int plx[pln] = { 0 };
  int pl1[pln] = { 0 };
  int pl5[pln] = { 0 };
  int pl9[pln] = { 0 };

  if( h10 )
    delete h10;
  h10 = new
    TProfile( "ia_vs_vana", "Ia vs Vana;Vana [DAC];Ia [mA]", 256, -0.5, 255.5,
              0, 999 );

  if( h11 )
    delete h11;
  h11 = new
    TProfile( "active_vs_vana", "Vana ROC scan;Vana [DAC];active pixels", 256,
              -0.5, 255.5, -1, 9999 );

  if( h12 )
    delete h12;
  h12 = new
    TProfile( "perfect_vs_vana", "Vana ROC scan;Vana [DAC];perfect pixels",
              256, -0.5, 255.5, -1, 9999 );

  if( h13 )
    delete h13;
  h13 = new
    TProfile( "perfect_vs_ia", "efficiency vs Ia;IA [mA];perfect pixels", 800,
              0, 80, -1, 9999 );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize ); // 2^24
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );

 // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.uDelay( 1000 );
  tb.Flush(  );

  int idac = 10;
  int step = 10;
  int nmax = 0;
  int kmax = 0;

  do {

    tb.SetDAC( Vana, idac );
    tb.Daq_Start(  );
    tb.uDelay( 1000 );
    tb.Flush(  );

    int n01, n50, n99;

    GetEff( n01, n50, n99 );

    plx[idac] = idac;
    pl1[idac] = n01;
    pl5[idac] = n50;
    pl9[idac] = n99;

    if( n99 > nmax ) {
      nmax = n99;
      kmax = 0;
    }
    if( n99 == nmax )
      kmax++;

    double ia = tb.GetIA(  ) * 1E3;

    h10->Fill( idac, ia );
    h11->Fill( idac, n50 );
    h12->Fill( idac, n99 );
    h13->Fill( ia, n99 );

    cout << setw( 3 ) << idac
      << setw( 6 ) << ia
      << setw( 6 ) << n01 << setw( 6 ) << n50 << setw( 6 ) << n99 << endl;
    Log.printf( "%i %f %i %i %i\n", idac, ia, n01, n50, n99 );

    if( kmax < 4 && n50 > 0 ) {
      if( step == 10 ) {
        step = 4;
        idac -= 10; // go back and measure finer
        kmax = 0;
      }
      else if( step == 4 ) {
        step = 2;
        idac -= 4; // go back and measure finer
        kmax = 0;
      }
      else if( step == 2 ) {
        step = 1;
        idac -= 2; // go back and measure finer
        kmax = 0;
      }
    }

    if( kmax == 4 )
      step = 2;
    if( kmax == 10 )
      step = 5;
    if( kmax == 14 )
      step = 10;

    idac += step;

  }
  while( idac < 256 );

  cout << endl;
  Log.flush(  );

 // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask(  );
  tb.roc_ClrCal(  );
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

 // compress and sort:

  int j = 0;
  for( int idac = 0; idac < 256; ++idac ) {
    if( pl1[idac] == 0 )
      continue;
    plx[j] = idac;
    pl1[j] = pl1[idac];
    pl5[j] = pl5[idac];
    pl9[j] = pl9[idac];
    ++j;
  }

  int nstp = j;
  cout << j << " points" << endl;

 // plot:

  h11->Write(  );
  h12->Write(  );
  h13->Write(  );
  if( par.isInteractive(  ) ) {
    h11->SetStats( 0 );
    h12->SetStats( 0 );
    h13->SetStats( 0 );
    h12->Draw(  );
    c1->Update(  );
  }
  cout << "histos 11, 12, 13" << endl;

 // analyze:

  nmax = 0;
  int i0 = 0;
  int i9 = 0;

  for( int j = 0; j < nstp; ++j ) {

    int idac = plx[j];
   //int nn = pl5[j]; // 50% active
    int nn = pl9[j];            // 100% perfect
    if( nn > nmax ) {
      nmax = nn;
      i0 = idac; // begin
    }
    if( nn == nmax )
      i9 = idac; // plateau

  } // dac

  cout << "eff plateau from " << i0 << " to " << i9 << endl;

  tb.SetDAC( Vana, val ); // back to default

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( gaindac ) // calibrated PH vs Vcal: check gain
{
  Log.section( "GAINDAC", false );
  Log.printf( " CtrlReg %i\n", dacval[0][CtrlReg] );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "vc_dac",
          dacval[0][CtrlReg] == 0 ?
          "mean Vcal PH vs Vcal;Vcal [DAC];calibrated PH [small Vcal DAC]" :
          "mean Vcal PH vs Vcal;Vcal [DAC];calibrated PH [large Vcal DAC]",
          256, -0.5, 255.5 );

  if( h14 )
    delete h14;
  h14 = new
    TH1D( "rms_dac",
          dacval[0][CtrlReg] == 0 ?
          "PH Vcal RMS vs Vcal;Vcal [DAC];calibrated PH RMS [small Vcal DAC]"
          :
          "PH Vcal RMS vs Vcal;Vcal [DAC];calibrated PH RMS [large Vcal DAC]",
          256, -0.5, 255.5 );

  TH1D hph[4160];
  size_t j = 0;
  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row ) {
      hph[j] = TH1D( Form( "PH_Vcal_c%02i_r%02i", col, row ),
                     Form( "PH vs Vcal col %i row %i;Vcal [DAC];<PH> [ADC]",
                           col, row ), 256, -0.5, 255.5 );
      ++j;
    }

 // Vcal loop:

  int32_t dacstrt = 0;
  int32_t dacstep = 1;
  int32_t dacstop = 255;
 //int32_t nstp = (dacstop-dacstrt) / dacstep + 1;

  for( int32_t ical = dacstrt; ical <= dacstop; ical += dacstep ) {

    tb.SetDAC( Vcal, ical );
    tb.Daq_Start(  );
    tb.uDelay( 1000 );
    tb.Flush(  );

   // measure:

    uint16_t nTrig = 10;
    vector < int16_t > nReadouts; // size 0
    vector < double >PHavg;
    vector < double >PHrms;
    nReadouts.reserve( 4160 ); // size 0, capacity 4160
    PHavg.reserve( 4160 );
    PHrms.reserve( 4160 );

    GetRocData( nTrig, nReadouts, PHavg, PHrms );

   // analyze:

    int nok = 0;
    double sum = 0;
    double su2 = 0;
    double vcmin = 256;
    double vcmax = -1;

    size_t j = 0;

    for( int col = 0; col < 52; ++col ) {
      for( int row = 0; row < 80 && j < nReadouts.size(  ); ++row ) {
        int cnt = nReadouts.at( j );
        double ph = PHavg.at( j );
        hph[j].Fill( ical, ph );
        double vc = PHtoVcal( ph, col, row );
        if( cnt > 0 && vc < 999 ) {
          nok++;
          sum += vc;
          su2 += vc * vc;
          if( vc < vcmin )
            vcmin = vc;
          if( vc > vcmax )
            vcmax = vc;
        }
        ++j;
      } // row
    } // col

    cout << setw( 3 ) << ical << ": pixels " << setw( 4 ) << nok;
    double mid = 0;
    double rms = 0;
    if( nok > 0 ) {
      cout << ", vc min " << vcmin << ", max " << vcmax;
      mid = sum / nok;
      rms = sqrt( su2 / nok - mid * mid );
      cout << ", mean " << mid << ", rms " << rms;
    }
    cout << endl;
    h11->Fill( ical, mid );
    h14->Fill( ical, rms );

  } // ical

  tb.SetDAC( Vcal, dacval[0][Vcal] ); // restore
  tb.Flush(  );

  j = 0;
  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row ) {
      hph[j].Write(  );
      ++j;
    }

  h14->Write(  );
  h11->Write(  );
  h11->SetStats( 0 );
  h11->Draw(  );
  c1->Update(  );
  cout << "histos 11, 14" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dacscanroc ) // LoopSingleRocAllPixelsDacScan: 72 s with nTrig 10
// PH vs Vcal = gain calibration
// N  vs Vcal = Scurves
// cals N  vs VthrComp = bump bond test (give negative ntrig)
{
  int dac;
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  int nTrig;                    // size = 4160 * 256 * nTrig * 3 words = 32 MW for 10 trig
  if( !PAR_IS_INT( nTrig, -1000, 10000 ) )
    nTrig = 10;

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  cout << "scan dac " << dacName[dac]
       << " at CtrlReg " << dacval[0][CtrlReg]
       << ", VthrComp " << dacval[0][VthrComp]
       << ", CalDel " << dacval[0][CalDel]
       << endl;

  Log.section( "DACSCANROC", false );
  Log.printf( " DAC %i\n", dac );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );
  tb.Daq_Start(  );
  tb.uDelay( 100 );

 // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.uDelay( 1000 );
  tb.Flush(  );

  uint16_t flags = 0;           // normal CAL
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS
  if( flags > 0 )
    cout << "CALS used..." << endl;
  uint16_t mTrig = abs( nTrig );

  if( dac == 11 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac == 12 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac == 25 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  int16_t dacLower1 = dacStrt( dac );
  int16_t dacUpper1 = dacStop( dac );
  int32_t nstp = dacUpper1 - dacLower1 + 1;

 // measure:

  cout << "pulsing 4160 pixels with " << mTrig << " triggers for "
    << nstp << " DAC steps may take " << int ( 4160 * mTrig * nstp * 6e-6 ) +
    1 << " s..." << endl;

 //tb.SetTimeout( 4 * mTrig * nstp * 6 * 2 ); // [ms]

 // header = 1 word
 // pixel = +2 words
 // size = 256 dacs * 4160 pix * mTrig * 3 words = 32 MW

  vector < uint16_t > data;
  data.reserve( nstp * mTrig * 4160 * 3 );

  bool done = 0;
  double tloop = 0;
  double tread = 0;

  while( done == 0 ) {

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec;        // seconds since 1.1.1970
    long u1 = tv.tv_usec;       // microseconds

    try {
      done =
        tb.LoopSingleRocAllPixelsDacScan( 0, mTrig, flags, dac, dacLower1,
                                          dacUpper1 );
    }
    catch( CRpcError & e ) {
      e.What(  );
      return 0;
    }

    cout << "waiting for FPGA..." << endl;
    int dSize = tb.Daq_GetSize(  );
    cout << "DAQ size " << dSize << " words" << endl;

   //tb.Daq_Stop(); // avoid extra (noise) data

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec;        // seconds since 1.1.1970
    long u2 = tv.tv_usec;       // microseconds
    double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;
    tloop += dt;
    cout << "LoopSingleRocAllPixelsDacScan takes " << dt << " s"
      << " = " << tloop / 4160 / mTrig / nstp * 1e6 << " us / pix" << endl;
    cout << ( done ? "done" : "not done" ) << endl;

    vector < uint16_t > dataB;
    dataB.reserve( Blocksize );

    try {
      uint32_t rest;
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(  ), dataB.begin(  ), dataB.end(  ) );
      cout << "data size " << data.size(  )
	   << ", remaining " << rest << endl;
      while( rest > 0 ) {
        dataB.clear(  );
        tb.Daq_Read( dataB, Blocksize, rest );
        data.insert( data.end(  ), dataB.begin(  ), dataB.end(  ) );
        cout << "data size " << data.size(  )
	     << ", remaining " << rest << endl;
      }
    }
    catch( CRpcError & e ) {
      e.What(  );
      return 0;
    }

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec;        // seconds since 1.1.1970
    long u3 = tv.tv_usec;       // microseconds
    double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
    tread += dtr;
    cout << "Daq_Read takes " << tread << " s"
      << " = " << 2 * dSize / tread / 1024 / 1024 << " MiB/s" << endl;

    if( !done )
      cout << "loop more..." << endl << flush;

  } // while not done

  tb.SetDAC( dac, dacval[0][dac] ); // restore

 // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask(  );
  tb.roc_ClrCal(  );
  tb.Flush(  );

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

  int ctl = dacval[0][CtrlReg];
  int cal = dacval[0][Vcal];
  if( h21 )
    delete h21;
  h21 = new TH2D( Form( "PH_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
                  Form( "PH vs %s CR %i Vcal %i map;pixel;%s [DAC];PH [ADC]",
                        dacName[dac].c_str(  ), ctl, cal,
                        dacName[dac].c_str(  ) ), 4160, -0.5, 4160 - 0.5,
                  nstp, -0.5, nstp - 0.5 );

  if( h22 )
    delete h22;
  h22 = new TH2D( Form( "N_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
                  Form( "N vs %s CR %i Vcal %i map;pixel;%s [DAC];readouts",
                        dacName[dac].c_str(  ), ctl, cal,
                        dacName[dac].c_str(  ) ), 4160, -0.5, 4160 - 0.5,
                  nstp, -0.5, nstp - 0.5 );

  if( h11 )
    delete h11;
  h11 = new TH1D( "Responses",
                  "Responses;max responses;pixels",
                  mTrig + 1, -0.5, mTrig + 0.5 );

  if( h12 )
    delete h12;
  h12 = new TH1D( "CalsVthrPlateauWidth",
                  "Width of VthrComp plateau for cals;width of VthrComp plateau for cals [DAC];pixels",
                  101, -0.5, 100.5 );

  if( h23 )
    delete h23;
  h23 = new TH2D( "ResponseMap",
                  "Response map;col;row;max responses",
                  52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h24 )
    delete h24;
  h24 = new TH2D( "BBtestMap",
                  "BBtest map;col;row;max responses",
                  52, -0.5, 51.5, 80, -0.5, 79.5 );

 // unpack data:
 // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
 // loop cols
 //   loop rows
 //     loop dacs
 //       loop trig

  PixelReadoutData pix;

  int pos = 0;
  int err = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

     //cout << endl << setw(2) << col << setw(3) << row << endl;

      int nmx = 0;

      for( int32_t j = 0; j < nstp; ++j ) { // DAC steps

        int cnt = 0;
        int phsum = 0;

        for( int k = 0; k < mTrig; ++k ) {

          err = DecodePixel( data, pos, pix ); // analyzer

          if( err )
            cout << "error " << err << " at trig " << k
              << ", stp " << j
              << ", pix " << col << " " << row << ", pos " << pos << endl;
          if( err )
            break;

          if( pix.n > 0 ) {
            if( pix.x == col && pix.y == row ) {
              cnt++;
              phsum += pix.p;
            }
          }

        } // trig

        if( err )
          break;

        double ph = -1;
        if( cnt > 0 ) {
          ph = ( double ) phsum / cnt;
        }
       /* huge
          cout << setw(4) << ((ph > -0.1 ) ? int(ph+0.5) : -1);
          if( row%20 == 19 ) cout << endl << "      ";
          Log.printf( "%i %f\n", cnt, ph );
        */
        uint32_t idc = dacLower1 + j;
        h21->Fill( 80 * col + row, idc, ph );
        h22->Fill( 80 * col + row, idc, cnt );
        if( cnt > nmx )
          nmx = cnt;

      } // dac

      if( err )
        break;

      h11->Fill( nmx );
      h23->Fill( col, row, nmx );

    } // row

    if( err )
      break;

  } // col

  h11->Write(  );
  h21->Write(  );
  h22->Write(  );
  h23->Write(  );

  if( nTrig < 0 ) { // BB test

   // Localize the missing Bump from h22

    int nbinx = h22->GetNbinsX(  );
    int nbiny = h22->GetNbinsY(  );

    int nActive;
    int nMissing;
    int nIneff;

    nActive = 0;
    nMissing = 0;
    nIneff = 0;

    int ibinCenter;

   // Find Plateau

    for( int ibin = 1; ibin <= nbinx; ibin++ ) {

      ibinCenter = h22->GetXaxis(  )->GetBinCenter( ibin );

     // Find first the maximum
      int imax;
      imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
        int cnt = h22->GetBinContent( ibin, j );
       // Find Maximum
        if( cnt > imax )
          imax = cnt;
      }

      if( imax < mTrig / 2 ) {
        ++nIneff;
        cout << "Dead pixel at raw col: " << ibinCenter % 80
          << " " << ibinCenter / 80 << endl;
      }
      else {

       // Search for the Plateau

        int iEnd = 0;
        int iBegin = 0;

       // use the maximum response to localize the Plateau
        for( int jbin = 0; jbin <= nbiny; jbin++ ) {
          int cnt = h22->GetBinContent( ibin, jbin );
         // Find Plateau
          if( cnt >= imax / 2 ) {
            iEnd = jbin; // end of plateau
            if( iBegin == 0 )
              iBegin = jbin; // begin of plateau
          }
        }

       // cout << "Bin: " << ibin << " Plateau Begins and End in  " << iBegin << " - " << iEnd << endl;
       // narrow plateau is from noise

        h12->Fill( iEnd - iBegin );

        if( iEnd - iBegin < 15 ) {

          nMissing++;
          cout << "[Missing Bump at raw col:] " << ibinCenter %
            80 << " " << ibinCenter / 80 << endl;
          Log.printf( "[Missing Bump at raw col:] %i %i\n", ibin % 80,
                      ibin / 80 );
          h24->Fill( ibinCenter / 80, ibinCenter % 80, 0 );
        }
        else {
          h24->Fill( ibinCenter / 80, ibinCenter % 80, imax );
          nActive++;
        } // plateau width
      } // active imax
    } // x-bins

    h12->Write(  );

   // save the map in the log file

    for( int ibin = 1; ibin <= h24->GetNbinsX(  ); ++ibin ) {
      for( int jbin = 1; jbin <= h24->GetNbinsY(  ); ++jbin ) {
        int c_val = h24->GetBinContent( ibin, jbin );
        Log.printf( " %i", c_val );
       //cout << ibin << " " << jbin << " " << c_val << endl;
      } //row
      Log.printf( "\n" );
    } // col
    Log.flush(  );

    Log.printf( "Number of Active bump bonds [above trig/2]: %i\n", nActive );
    Log.printf( "Number of Missing bump bonds: %i\n", nMissing );
    Log.printf( "Number of Dead pixel: %i\n", nIneff );

    cout << "Number of Active bump bonds: " << nActive << endl;
    cout << "Number of Missing bump bonds: " << nMissing << endl;
    cout << "Number of Dead pixel: " << nIneff << endl;

    h24->Write(  );
    h24->SetStats( 0 );
    h24->Draw( "colz" );
    c1->Update(  );
    cout << "histos 11, 12, 21, 22, 23, 24" << endl;

  } // BB test

  else {
    h21->SetStats( 0 );
    h21->Draw( "colz" );
    c1->Update(  );
    cout << "histos 11, 21, 22, 23" << endl;
  }

  Log.flush(  );

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dacdac ) // LoopSingleRocOnePixelDacDacScan: 
// N  vs CalDel and VthrComp or Vcal = Tornado, timewalk
{
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  int dac1;
  PAR_INT( dac1, 1, 32 ); // only DACs, not registers
  int dac2;
  PAR_INT( dac2, 1, 32 ); // only DACs, not registers

  if( dacval[0][dac1] == -1 ) {
    cout << "DAC " << dac1 << " not active" << endl;
    return false;
  }
  if( dacval[0][dac2] == -1 ) {
    cout << "DAC " << dac2 << " not active" << endl;
    return false;
  }

  Log.section( "DACDAC", false );
  Log.printf( " pixel %i %i DAC %i vs %i\n", col, row, dac1, dac2 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize ); // 2^24
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );
  tb.uDelay( 100 );
  tb.Daq_Start(  );
  tb.uDelay( 100 );

 // pixel on:

  tb.roc_Col_Enable( col, 1 );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.Flush(  );

  uint16_t nTrig = 10;          // size = 4160 * 256 * nTrig * 3 words = 32 MW for 10 trig

  uint16_t flags = 0;           // normal CAL

  flags = 0x0002; // FLAG_USE_CALS;
  if( flags > 0 )
    cout << "CALS used..." << endl;

  if( dac1 == 11 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac1 == 12 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac2 == 11 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac2 == 12 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy

 //flags |= 0x0020; // don't use DAC correction table

  cout << "flags hex " << hex << flags << dec << endl;

  int16_t dacLower1 = dacStrt( dac1 );
  int16_t dacUpper1 = dacStop( dac1 );
  int32_t nstp1 = dacUpper1 - dacLower1 + 1;

  int16_t dacLower2 = dacStrt( dac2 );
  int16_t dacUpper2 = dacStop( dac2 );
  int32_t nstp2 = dacUpper2 - dacLower2 + 1;

  gettimeofday( &tv, NULL );
  long s1 = tv.tv_sec;          // seconds since 1.1.1970
  long u1 = tv.tv_usec;         // microseconds

 // measure:

  cout << "pulsing " << nTrig << " triggers for "
    << nstp1 << " x " << nstp2 << " DAC steps may take "
    << int ( nTrig * nstp1 * nstp2 * 6e-6 ) + 1 << " s..." << endl;

  try {
    tb.LoopSingleRocOnePixelDacDacScan( 0, col, row, nTrig, flags,
                                        dac1, dacLower1, dacUpper1,
                                        dac2, dacLower2, dacUpper2 );
   // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
   // loop dac1
   //   loop dac2
   //     loop trig
  }
  catch( CRpcError & e ) {
    e.What(  );
    return 0;
  }

  tb.Daq_Stop(  ); // avoid extra (noise) data

  int dSize = tb.Daq_GetSize(  );

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec;          // seconds since 1.1.1970
  long u2 = tv.tv_usec;         // microseconds
  double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;

  cout << "LoopSingleRocOnePixelDacDacScan takes " << dt << " s"
    << " = " << dt / nTrig / nstp1 / nstp2 * 1e6 << " us / pix" << endl;

  tb.SetDAC( dac1, dacval[0][dac1] ); // restore
  tb.SetDAC( dac2, dacval[0][dac2] ); // restore

  tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask(  );
  tb.roc_ClrCal(  );
  tb.Flush(  );

 // header = 1 word
 // pixel = +2 words
 // size = 256 dacs * 4160 pix * nTrig * 3 words = 32 MW

  cout << "DAQ size " << dSize << endl;

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize(  ) );

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest );
    cout << "data size " << data.size(  )
	 << ", remaining " << rest << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(  ), dataB.begin(  ), dataB.end(  ) );
      cout << "data size " << data.size(  )
	   << ", remaining " << rest << endl;
      dataB.clear(  );
    }
  }
  catch( CRpcError & e ) {
    e.What(  );
    return 0;
  }

  gettimeofday( &tv, NULL );
  long s3 = tv.tv_sec;          // seconds since 1.1.1970
  long u3 = tv.tv_usec;         // microseconds
  double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
  cout << "Daq_Read takes " << dtr << " s"
    << " = " << 2 * dSize / dtr / 1024 / 1024 << " MiB/s" << endl;

#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
 //tb.Daq_DeselectAll();
  tb.Flush(  );
#endif

  if( h21 )
    delete h21;
  h21 = new
    TH2D( Form( "PH_DAC%02i_DAC%02i_col%02i_row%02i", dac1, dac2, col, row ),
          Form( "PH vs %s vs %s col %i row %i;%s [DAC];%s [DAC];PH [ADC]",
                dacName[dac1].c_str(  ), dacName[dac2].c_str(  ), col, row,
                dacName[dac1].c_str(  ), dacName[dac2].c_str(  ) ),
          nstp1, -0.5, nstp1 - 0.5, nstp2, -0.5, nstp2 - 0.5 );

  if( h22 )
    delete h22;
  h22 = new
    TH2D( Form( "N_DAC%02i_DAC%02i_col%02i_row%02i", dac1, dac2, col, row ),
          Form( "N vs %s vs %s col %i row %i;%s [DAC];%s [DAC];readouts",
                dacName[dac1].c_str(  ), dacName[dac2].c_str(  ), col, row,
                dacName[dac1].c_str(  ), dacName[dac2].c_str(  ) ),
          nstp1, -0.5, nstp1 - 0.5, nstp2, -0.5, nstp2 - 0.5 );

  if( h23 )
    delete h23;
  h23 = new
    TH2D( Form
          ( "N_Trig2DAC%02i_DAC%02i_col%02i_row%02i", dac1, dac2, col, row ),
          Form
          ( "N_Trig2 vs %s vs %s col %i row %i;%s [DAC];%s [DAC];readouts",
            dacName[dac1].c_str(  ), dacName[dac2].c_str(  ), col, row,
            dacName[dac1].c_str(  ), dacName[dac2].c_str(  ) ), nstp1, -0.5,
          nstp1 - 0.5, nstp2, -0.5, nstp2 - 0.5 );

  TH1D *h_one =
    new TH1D( Form( "h_optimal_DAC%02i_col%02i_row%02i", dac2, col, row ),
              Form( "optimal DAC %i col %i row %i", dac2, col, row ),
              nstp1, -0.5, nstp1 - 0.5 );

 // unpack data:

  PixelReadoutData pix;

  int pos = 0;
  int err = 0;

  for( int32_t i = 0; i < nstp1; ++i ) { // DAC steps

    for( int32_t j = 0; j < nstp2; ++j ) { // DAC steps

      int cnt = 0;
      int phsum = 0;

      for( int k = 0; k < nTrig; ++k ) {

        err = DecodePixel( data, pos, pix ); // analyzer

        if( err )
          cout << "error " << err << " at trig " << k
            << ", stp " << j << " " << i << ", pos " << pos << endl;
        if( err )
          break;

        if( pix.n > 0 ) {
          if( pix.x == col && pix.y == row ) {
            cnt++;
            phsum += pix.p;
          }
        }

      } // trig

      if( err )
        break;

      double ph = -1;
      if( cnt > 0 ) {
        ph = ( double ) phsum / cnt;
      }
      uint32_t idac = dacLower1 + i;
      uint32_t jdac = dacLower2 + j;
      h21->Fill( idac, jdac, ph );
      h22->Fill( idac, jdac, cnt );

      if( cnt > nTrig / 2 ) {
        h23->Fill( idac, jdac, cnt );
      }

    } // dac

    if( err )
      break;

  } // dac

  Log.flush(  );

  h21->Write(  );
  h22->Write(  );
  h23->Write(  );

  h23->SetStats( 0 );
  h23->Draw( "colz" );
  c1->Update(  );
  cout << "histos 21, 22, 23" << endl;

  if( dac1 == 26 && dac2 == 12 ) { // Tornado plot: VthrComp vs CalDel

    int dac1Mean = int ( h23->GetMean( 1 ) );

    int i1 = h23->FindBin( dac1Mean, 1 );

    cout << "dac " << dac1 << " mean " << dac1Mean
	 << " at bin " << i1 << endl;

   // find efficiency plateau of dac2 at dac1Mean:

    int nm = 0;
    int i0 = 0;
    int i9 = 0;

    for( int j = 0; j < nstp1; ++j ) {

      int cnt = h23->GetBinContent( i1, j+1 ); // ROOT bins start at 1

      h_one->Fill( j, cnt );

      //cout << " " << cnt;
      //Log.printf( "%i %i\n", j, cnt );

      if( cnt > nm ) {
	nm = cnt;
	i0 = j; // begin of plateau
      }
      if( cnt >= nm ) {
	i9 = j; // end of plateau
      }
    }

    cout << "dac " << dac2 << " plateau from " << i0 << " to " << i9 << endl;

    int dac2Best = i0 + ( i9 - i0 ) / 4;

    cout << "set dac " << dac1 << " " << dacName[dac1]
      << " to mean " << dac1Mean << endl;

    cout << "set dac " << dac2 << " " << dacName[dac2]
      << " to best " << dac2Best << endl;

    tb.SetDAC( dac1, dac1Mean );
    tb.SetDAC( dac2, dac2Best );
    dacval[0][dac1] = dac1Mean;
    dacval[0][dac2] = dac2Best;

    Log.printf( "[SETDAC] %i  %i\n", dac1, dac1Mean );
    Log.printf( "[SETDAC] %i  %i\n", dac2, dac2Best );

    Log.printf( "dac2 %i Plateau begin %i end %i\n", dac2, i0, i9 );

    h_one->Write(  );
    delete h_one;

  } // Tornado

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
/* read back reg 255:
  0000 I2C data
  0001 I2C address
  0010 I2C pixel column
  0011 I2C pixel row

  1000 VD unreg
  1001 VA unreg
  1010 VA reg
  1100 IA
*/
CMD_PROC( readback )
{
  int i;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Pg_Stop(  );
  tb.Pg_SetCmd( 0, PG_TOK );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase(  ) );

 // take data:

  tb.Daq_Start(  );
  for( i = 0; i < 36; ++i ) {
    tb.Pg_Single(  );
    tb.uDelay( 20 );
  }
 //tb.Daq_Stop();

 // read out data
  vector < uint16_t > data;
  tb.Daq_Read( data, 40 );
#ifdef DAQOPENCLOSE
  tb.Daq_Close(  );
#endif

 //analyze data
  int pos = 0;
  PixelReadoutData pix;
  int err = 0;

  unsigned int value = 0;

 // find start bit
  do {
    err = DecodePixel( data, pos, pix );
    if( err )
      break;
  } while( ( pix.hdr & 2 ) == 0 );

 // read data
  for( i = 0; i < 16; ++i ) {
    err = DecodePixel( data, pos, pix );
    if( err )
      break;
    value = ( value << 1 ) + ( pix.hdr & 1 );
  }

  int roc = ( value >> 12 ) & 0xf;
  int cmd = ( value >> 8 ) & 0xf;
  int x = value & 0xff;

  printf( "%04X: roc[%i].", value, roc );
  switch ( cmd ) {
  case 0:
    printf( "I2C_data = %02X\n", x );
    break;
  case 1:
    printf( "I2C_addr = %02X\n", x );
    break;
  case 2:
    printf( "col = %02X\n", x );
    break;
  case 3:
    printf( "row = %02X\n", x );
    break;
  case 8:
    printf( "VD_unreg = %02X\n", x );
    break;
  case 9:
    printf( "VA_unreg = %02X\n", x );
    break;
  case 10:
    printf( "VA_reg = %02X\n", x );
    break;
  case 12:
    printf( "IA = %02X\n", x );
    break;
  default:
    printf( "? = %04X\n", value );
    break;
  }

  return true;
}

//------------------------------------------------------------------------------
void cmdHelp(  )
{
  fputs( "+-cmd commands ----------------------+\n"
         "| help      list of commands         |\n"
         "| exit      exit commander           |\n"
         "| quit      exit commander           |\n"
         "+------------------------------------+\n", stdout );
}

//------------------------------------------------------------------------------
CMD_PROC( h )
{
  cmdHelp(  );
  return true;
}

//------------------------------------------------------------------------------
void cmd(  )                    // called once from psi46test
{
  CMD_REG( showtb, "showtb                        print DTB state" );
  CMD_REG( showhv, "show hv                       status of iseg HV supply" );
  CMD_REG( scan, "scan                          enumerate USB devices" );
  CMD_REG( open,
           "open <name>                   open connection to testboard" );
  CMD_REG( close,
           "close                         close connection to testboard" );
  CMD_REG( welcome, "welcome                       blinks with LEDs" );
  CMD_REG( setled, "setled <bits>                 set atb LEDs" );
  CMD_REG( log, "log <text>                    writes text to log file" );
  CMD_REG( upgrade, "upgrade <filename>            upgrade DTB" );
  CMD_REG( rpcinfo, "rpcinfo                       lists DTB functions" );
  CMD_REG( version,
           "version                       shows DTB software version" );
  CMD_REG( info, "info                          shows detailed DTB info" );
  CMD_REG( boardid, "boardid                       get board id" );
  CMD_REG( init, "init                          inits the testboard" );
  CMD_REG( flush, "flush                         flushes usb buffer" );
  CMD_REG( clear, "clear                         clears usb data buffer" );

  CMD_REG( udelay, "udelay <us>                   waits <us> microseconds" );
  CMD_REG( mdelay, "mdelay <ms>                   waits <ms> milliseconds" );

  CMD_REG( d1, "d1 <signal>                   assign signal to D1 output" );
  CMD_REG( d2, "d2 <signal>                   assign signal to D2 outout" );
  CMD_REG( a1,
           "a1 <signal>                   assign analog signal to A1 output" );
  CMD_REG( a2,
           "a2 <signal>                   assign analog signal to A2 outout" );
  CMD_REG( probeadc,
           "probeadc <signal>             assign analog signal to ADC" );

  CMD_REG( clksrc, "clksrc <source>               Select clock source" );
  CMD_REG( clkok,
           "clkok                         Check if ext clock is present" );
  CMD_REG( fsel, "fsel <freqdiv>                clock frequency select" );
  CMD_REG( stretch, "stretch <src> <delay> <width> stretch clock" );

  CMD_REG( clk, "clk <delay>                   clk delay" );
  CMD_REG( ctr, "ctr <delay>                   ctr delay" );
  CMD_REG( sda, "sda <delay>                   sda delay" );
  CMD_REG( tin, "tin <delay>                   tin delay" );
  CMD_REG( clklvl, "clklvl <level>                clk signal level" );
  CMD_REG( ctrlvl, "ctrlvl <level>                ctr signel level" );
  CMD_REG( sdalvl, "sdalvl <level>                sda signel level" );
  CMD_REG( tinlvl, "tinlvl <level>                tin signel level" );
  CMD_REG( clkmode, "clkmode <mode>                clk mode" );
  CMD_REG( ctrmode, "ctrmode <mode>                ctr mode" );
  CMD_REG( sdamode, "sdamode <mode>                sda mode" );
  CMD_REG( tinmode, "tinmode <mode>                tin mode" );

  CMD_REG( sigoffset, "sigoffset <offset>            output signal offset" );
  CMD_REG( lvds, "lvds                          LVDS inputs" );
  CMD_REG( lcds, "lcds                          LCDS inputs" );

  CMD_REG( pon, "pon                           switch ROC power on" );
  CMD_REG( poff, "poff                          switch ROC power off" );
  CMD_REG( va, "va <mV>                       set VA supply in mV" );
  CMD_REG( vd, "vd <mV>                       set VD supply in mV" );
  CMD_REG( ia, "ia <mA>                       set IA limit in mA" );
  CMD_REG( id, "id <mA>                       set ID limit in mA" );

  CMD_REG( getva, "getva                         set VA in V" );
  CMD_REG( getvd, "getvd                         set VD in V" );
  CMD_REG( getia, "getia                         set IA in mA" );
  CMD_REG( getid, "getid                         set ID in mA" );
  CMD_REG( optia,
           "optia <ia>                    set Vana to desired ROC Ia [mA]" );

  CMD_REG( hvon, "hvon                          switch HV on" );
  CMD_REG( hvoff, "hvoff                         switch HV off" );
  CMD_REG( vb, "vb <V>                        set -Vbias in V" );

  CMD_REG( reson, "reson                         activate reset" );
  CMD_REG( resoff, "resoff                        deactivate reset" );
  CMD_REG( rocaddr, "rocaddr                       set ROC address" );
  CMD_REG( rowinvert,
           "rowinvert                     invert row address psi46dig" );
  CMD_REG( chip, "chip num                      set chip number" );

  CMD_REG( pgset,
           "pgset <addr> <bits> <delay>   set pattern generator entry" );
  CMD_REG( pgstop, "pgstop                        stops pattern generator" );
  CMD_REG( pgsingle, "pgsingle                      send single pattern" );
  CMD_REG( pgtrig,
           "pgtrig                        enable external pattern trigger" );
  CMD_REG( pgloop,
           "pgloop <period>               start patterngenerator in loop mode" );
  CMD_REG( trigdel,
           "trigdel <delay>               delay in trigger loop [BC]" );

  CMD_REG( upd, "upd <histo>                  re-draw ROOT histo in canvas" );

  CMD_REG( dopen,
           "dopen <buffer size> [<ch>]    Open DAQ and allocate memory" );
  CMD_REG( dsel,
           "dsel <MHz>                    select deserializer 160 or 400 MHz" );
  CMD_REG( dreset,
           "dreset <reset>                DESER400 reset 1, 2, or 3" );
  CMD_REG( dclose, "dclose [<channel>]            Close DAQ" );
  CMD_REG( dstart, "dstart [<channel>]            Enable DAQ" );
  CMD_REG( dstop, "dstop [<channel>]             Disable DAQ" );
  CMD_REG( dsize,
           "dsize [<channel>]             Show DAQ buffer fill state" );
  CMD_REG( dread,
           "dread                         Read Daq buffer and show as ROC data" );
  CMD_REG( dreadm,
           "dreadm [channel]              Read Daq buffer and show as module data" );

  CMD_REG( showclk, "showclk                       show CLK signal" );
  CMD_REG( showctr, "showctr                       show CTR signal" );
  CMD_REG( showsda, "showsda                       show SDA signal" );

  CMD_REG( tbmdis, "tbmdis                        disable TBM" );
  CMD_REG( tbmsel,
           "tbmsel <hub> <port>           set hub and port address, port 6=all" );
  CMD_REG( modsel,
           "modsel <hub>                  set hub address for module" );
  CMD_REG( tbmset, "tbmset <reg> <value>          set TBM register" );

  CMD_REG( readback, "readback                      read out ROC data" );

  CMD_REG( dselmod,
           "dselmod                       select deser400 for DAQ channel 0" );
  CMD_REG( dmodres, "dmodres                       reset all deser400" );
  CMD_REG( dselroca,
           "dselroca <value>              select adc for channel 0" );
  CMD_REG( dseloff, "dseloff                       deselect all" );
  CMD_REG( deser160, "deser160                      allign deser160" );
  CMD_REG( deser, "deser <value>                 controls deser160" );

  CMD_REG( select, "select <addr range>           set i2c address" );
  CMD_REG( dac, "dac <address> <value>         set DAC" );
  CMD_REG( vana, "vana <value>                  set Vana" );
  CMD_REG( vtrim, "vtrim <value>                 set Vtrim" );
  CMD_REG( vthr, "vthr <value>                  set VthrComp" );
  CMD_REG( vcal, "vcal <value>                  set Vcal" );
  CMD_REG( ctl, "ctl <value>                   set control register" );
  CMD_REG( wbc, "wbc <value>                   set WBC" );
  CMD_REG( show, "show [roc]                     print dacs" );
  CMD_REG( show1, "show1 dac                     print one dac for all ROCs" );
  CMD_REG( wdac,
           "wdac [description]            write dacParameters_chip_desc.dat" );
  CMD_REG( rddac,
           "rddac [description]           read dacParameters_chip_desc.dat" );
  CMD_REG( wtrim,
           "wtrim [description]           write trimParameters_chip_desc.dat" );
  CMD_REG( rdtrim,
           "rdtrim [description]          read trimParameters_chip_desc.dat" );

  CMD_REG( cole, "cole <range>                  enable column" );
  CMD_REG( cold, "cold <range>                  disable columns" );
  CMD_REG( pixi, "pixi roc col row              show trim bits" );
  CMD_REG( pixt, "pixt roc col row trim         set trim bits" );
  CMD_REG( pixe, "pixe <range> <range> <value>  trim pixel" );
  CMD_REG( pixd, "pixd <range> <range>          mask pixel" );
  CMD_REG( cal, "cal <range> <range>           enable calibrate pixel" );
  CMD_REG( arm, "arm <range> <range>           activate pixel" );
  CMD_REG( cals, "cals <range> <range>          sensor calibrate pixel" );
  CMD_REG( cald, "cald                          clear calibrate" );
  CMD_REG( mask, "mask                          mask all pixel and cols" );
  CMD_REG( fire, "fire col row [nTrig]          single pulse and read" );
  CMD_REG( fire2, "fire col row [nTrig]          correlation" );

  CMD_REG( daci, "daci roc dac                  current vs dac" );
  CMD_REG( vanaroc,
           "vanaroc                       ROC efficiency scan vs Vana" );
  CMD_REG( vthrcompi, "vthrcompi                     Id vs VthrComp" );
  CMD_REG( caldel, "caldel col row                CalDel efficiency scan" );
  CMD_REG( caldelmap, "caldelmap                     map of CalDel range" );
  CMD_REG( caldelroc,
           "caldelroc                     ROC CalDel efficiency scan" );
  CMD_REG( modcaldel,
           "modcaldel col row             CalDel efficiency scan for module" );
  CMD_REG( modpixsc, "modpixsc col row ntrig        module pixel S-curve" );
  CMD_REG( modsc, "modsc ntrig [step]            module S-curves MultiRoc" );
  CMD_REG( modthrdac,
           "modthrdac col row dac         Threshold vs DAC one pixel" );
  CMD_REG( modvthrcomp,
           "modvthrcomp target         set VthrComp on each ROC" );
  CMD_REG( modtrim, "modtrim target                set Vtrim and trim bits" );
  CMD_REG( modmap, "modmap nTrig                  module map" );

  CMD_REG( takedata,
           "takedata period               readout 40 MHz/period (stop: s enter)" );
  CMD_REG( onevst,
           "onevst col row period         <PH> vs time (stop: s enter)" );
  CMD_REG( modtd,
           "modtd period                  module take data 40MHz/period (stop: s enter)" );

  CMD_REG( vthrcomp,
           "vthrcomp target [guess]       set VthrComp to target Vcal" );
  CMD_REG( trim, "trim target [guess]           set Vtrim and trim bits" );
  CMD_REG( trimbits,
           "trimbits                      set trim bits for efficiency" );
  CMD_REG( thrdac,
           "thrdac col row dac            Threshold vs DAC one pixel" );
  CMD_REG( thrmap, "thrmap guess                  threshold map trimmed" );
  CMD_REG( thrmapsc, "thrmapsc stop (4=cals)        threshold map" );

  CMD_REG( effdac,
           "effdac col row dac            Efficiency vs DAC one pixel" );
  CMD_REG( phdac, "phdac col row dac             PH vs DAC one pixel" );
  CMD_REG( gaindac, "gaindac                       calibrated PH vs Vcal" );
  CMD_REG( calsdac, "calsdac col row dac [nTrig]   cals vs DAC one pixel" );
  CMD_REG( dacdac, "dacdac col row dacx dacy      DAC DAC scan" );

  CMD_REG( dacscanroc, "dacscanroc dac [nTrig]       PH vs DAC, all pixels" );

  CMD_REG( tune, "tune col row                  tune gain and offset" );
  CMD_REG( phmap, "phmap nTrig                   ROC PH map" );
  CMD_REG( calsmap,
           "calsmap nTrig                 CALS map = bump bond test" );
  CMD_REG( effmap, "effmap nTrig                  pixel alive map" );
  CMD_REG( bbtest,
           "bbtest nTrig                  CALS map = bump bond test" );

  cmdHelp(  );

  cmd_intp.SetScriptPath( settings.path );

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  for( size_t iroc = 0; iroc < 16; ++iroc )
    for( size_t idac = 0; idac < 256; ++idac )
      dacval[iroc][idac] = -1;

 // init globals (to something other than the default zero):

  for( int roc = 0; roc < 16; ++roc )
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        modthr[roc][col][row] = 255;
        modtrm[roc][col][row] = 15;
      }

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // set ROOT styles:

  gStyle->SetTextFont( 62 ); // 62 = Helvetica bold
  gStyle->SetTextAlign( 11 );

  gStyle->SetTickLength( -0.02, "x" ); // tick marks outside
  gStyle->SetTickLength( -0.02, "y" );
  gStyle->SetTickLength( -0.01, "z" );

  gStyle->SetLabelOffset( 0.022, "x" );
  gStyle->SetLabelOffset( 0.022, "y" );
  gStyle->SetLabelOffset( 0.022, "z" );

  gStyle->SetTitleOffset( 1.3, "x" );
  gStyle->SetTitleOffset( 1.6, "y" );
  gStyle->SetTitleOffset( 1.7, "z" );

  gStyle->SetLabelFont( 62, "X" );
  gStyle->SetLabelFont( 62, "Y" );
  gStyle->SetLabelFont( 62, "z" );

  gStyle->SetTitleFont( 62, "X" );
  gStyle->SetTitleFont( 62, "Y" );
  gStyle->SetTitleFont( 62, "z" );

  gStyle->SetTitleBorderSize( 0 ); // no frame around global title
  gStyle->SetTitleAlign( 13 ); // 13 = left top align
  gStyle->SetTitleX( 0.12 ); // global title
  gStyle->SetTitleY( 0.98 ); // global title

  gStyle->SetLineWidth( 1 ); // frames
  gStyle->SetHistLineColor( 4 ); // 4=blau
  gStyle->SetHistLineWidth( 3 );
  gStyle->SetHistFillColor( 5 ); // 5=gelb
 //  gStyle->SetHistFillStyle(4050); // 4050 = half transparent
  gStyle->SetHistFillStyle( 1001 ); // 1001 = solid

  gStyle->SetFrameLineWidth( 2 );

 // statistics box:

  gStyle->SetOptStat( 111111 );
  gStyle->SetStatFormat( "8.6g" ); // more digits, default is 6.4g
  gStyle->SetStatFont( 42 ); // 42 = Helvetica normal
 //  gStyle->SetStatFont(62); // 62 = Helvetica bold
  gStyle->SetStatBorderSize( 1 ); // no 'shadow'

  gStyle->SetStatX( 0.99 );
  gStyle->SetStatY( 0.60 );

  gStyle->SetPalette( 1 ); // rainbow colors

  gStyle->SetHistMinimumZero(  ); // no zero suppression

  gStyle->SetOptDate(  );

  cout << "open ROOT window..." << endl;
  MyMainFrame *myMF = new MyMainFrame( gClient->GetRoot(  ), 800, 600 );

  myMF->SetWMPosition( 99, 0 );

  cout << "open Canvas..." << endl;
  c1 = myMF->GetCanvas(  );

  c1->SetBottomMargin( 0.15 );
  c1->SetLeftMargin( 0.15 );
  c1->SetRightMargin( 0.18 );

  gPad->Update(  ); // required

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // command loop:

  while( true ) {

    try {

      CMD_RUN( stdin );

      delete myMF;
      return;
    }
    catch( CRpcError e ) {
      cout << "cmd gets CRpcError" << endl;
     //e.What(); crashes
     /*
        in __strlen_sse42 () from /lib64/libc.so.6
        std::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string(char const*, std::allocator<char> const&) () from /usr/lib64/libstdc++.so.6
        in CRpcError::What (this=0x7fff6c8dfc30) at rpc_error.cpp:36
        in cmd () at cmd.cpp:7714
      */
    }
  }

}
