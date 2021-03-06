
// test board, ROC, and Module commands

// Beat Meier, PSI, 31.8.2007 for Chip/Wafer tester
// Daniel Pitzl, DESY, 18.3.2014
// Claudia Seitz, DESY, 2014
// Daniel Pitzl, DESY, 5.7.2016

#include <cstdlib> // abs
#include <math.h>
#include <time.h> // clock
#include <sys/time.h> // gettimeofday, timeval
#include <fstream> // gainfile, dacPar
#include <iostream> // cout
#include <iomanip> // setw
#include <sstream> // stringstream
#include <utility>
#include <set>

#include "TROOT.h" // gROOT
#include "TFile.h" //
#include "TCanvas.h"
#include <TStyle.h>
#include <TF1.h>
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
const uint32_t Blocksize = 4 * 1024 * 1024;
//const uint32_t Blocksize = 16*1024*1024; // ERROR
//const uint32_t Blocksize = 64*1024*1024; // ERROR

int dacval[16][256];
string dacName[256];

int Chip = 599;
int Module = 4001;

bool HV = 0;

int ierror = 0;

int roclist[16] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
vector<int> enabledrocslist;
vector<int> ignorerocslist;

int nrocsa;
int nrocsb;

int modthr[16][52][80];
int modtrm[16][52][80];
int modcnt[16][52][80]; // responses
int modamp[16][52][80]; // amplitudes
bool hot[16][52][80] = { {{0}} }; // hot trim bits

// gain:

bool haveGain = 0;

double p0[16][52][80];
double p1[16][52][80];
double p2[16][52][80];
double p3[16][52][80];
double p4[16][52][80];
double p5[16][52][80];

TCanvas *c1 = NULL;

TH1D *h10 = NULL;
TH1D *h11 = NULL;
TH1D *h12 = NULL;
TH1D *h13 = NULL;
TH1D *h14 = NULL;
TH1D *h15 = NULL;
TH1D *h16 = NULL;
TH1D *h17 = NULL;

TH2D *h20 = NULL;
TH2D *h21 = NULL;
TH2D *h22 = NULL;
TH2D *h23 = NULL;
TH2D *h24 = NULL;
TH2D *h25 = NULL;

Iseg iseg;

//------------------------------------------------------------------------------
class TBstate
{
  bool daqOpen;
  uint32_t daqSize;
  int clockPhase;
  int deserPhase;

public:
  TBstate():daqOpen( 0 ), daqSize( 0 ), clockPhase( 4 ), deserPhase( 4 )
  {
  }
  ~TBstate()
  {
  }

  void SetDaqOpen( const bool open )
  {
    daqOpen = open;
  }
  bool GetDaqOpen()
  {
    return daqOpen;
  }

  void SetDaqSize( const uint32_t size )
  {
    daqSize = size;
  }
  uint32_t GetDaqSize()
  {
    return daqSize;
  }

  void SetClockPhase( const uint32_t phase )
  {
    clockPhase = phase;
  }
  uint32_t GetClockPhase()
  {
    return clockPhase;
  }

  void SetDeserPhase( const uint32_t phase160 )
  {
    deserPhase = phase160;
  }
  uint32_t GetDeserPhase()
  {
    return deserPhase;
  }
};

TBstate tbState;

//------------------------------------------------------------------------------
CMD_PROC( showtb ) // test board state
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;

  Log.section( "TBSTATE", true );

  cout << " 40 MHz clock phase " << tbState.GetClockPhase() << " ns" << endl;
  cout << "160 MHz deser phase " << tbState.GetDeserPhase() << " ns" << endl;
  if( tbState.GetDaqOpen() )
    cout << "DAQ memeory size " << tbState.GetDaqSize() << " words" << endl;
  else
    cout << "no DAQ open" << endl;

  uint8_t status = tb.GetStatus();
  printf( "SD card detect: %c\n", ( status & 8 ) ? '1' : '0' );
  printf( "CRC error:      %c\n", ( status & 4 ) ? '1' : '0' );
  printf( "Clock good:     %c\n", ( status & 2 ) ? '1' : '0' );
  printf( "CLock present:  %c\n", ( status & 1 ) ? '1' : '0' );

  Log.printf( "Clock phase %i\n", tbState.GetClockPhase() );
  Log.printf( "Deser phase %i\n", tbState.GetDeserPhase() );
  if( tbState.GetDaqOpen() )
    Log.printf( "DAQ memory size %i\n", tbState.GetDaqSize() );
  else
    Log.printf( "DAQ not open\n" );

  cout << "PixelAddressInverted " << tb.GetPixelAddressInverted() << endl;
  cout << "PixelAddressInverted " << tb.invertAddress << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( showhv ) // iseg HV status
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  iseg.status();
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( scan ) // scan for DTB on USB
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  CTestboard *tb = new CTestboard;
  string name;
  vector < string > devList;
  unsigned int nDev, nr;

  try {
    if( !tb->EnumFirst( nDev ) )
      throw int ( 1 );
    for( nr = 0; nr < nDev; ++nr ) {
      if( !tb->EnumNext( name ) )
        throw int ( 2 );
      if( name.size() < 4 )
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

  if( devList.size() == 0 ) {
    printf( "no DTB connected\n" );
    return true;
  }

  for( nr = 0; nr < devList.size(); ++nr )
    try {
      printf( "%10s: ", devList[nr].c_str() );
      if( !tb->Open( devList[nr], false ) ) {
	printf( "DTB in use\n" );
	continue;
      }
      unsigned int bid = tb->GetBoardId();
      printf( "DTB Id %u\n", bid );
      tb->Close();
    }
    catch( ... ) {
      printf( "DTB not identifiable\n" );
      tb->Close();
    }

  delete tb; // not permanentely opened

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( open )
{
  if( tb.IsConnected() ) {
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
            tb.ConnectionError(), usbId.c_str() );
    return true;
  }

  printf( "DTB %s opened\n", usbId.c_str() );

  string info;
  tb.GetInfo( info );
  printf( "--- DTB info-------------------------------------\n"
          "%s"
          "-------------------------------------------------\n",
          info.c_str() );

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( close )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Close();
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( welcome )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Welcome();
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

  if( tb.UpgradeGetVersion() == 0x0100 ) {
    // open file
    src.open( filename );
    if( !src.is_open() ) {
      printf( "ERROR UPGRADE: Could not open \"%s\"!\n", filename );
      return false;
    }

    // check if upgrade is possible
    printf( "Start upgrading DTB.\n" );
    if( tb.UpgradeStart( 0x0100 ) != 0 ) {
      string msg;
      tb.UpgradeErrorMsg( msg );
      printf( "ERROR UPGRADE: %s!\n", msg.data() );
      return false;
    }

    // download data
    printf( "Download running ...\n" );
    string rec;
    uint16_t recordCount = 0;
    while( true ) {
      getline( src, rec );
      if( src.good() ) {
        if( rec.size() == 0 )
          continue;
        ++recordCount;
        if( tb.UpgradeData( rec ) != 0 ) {
          string msg;
          tb.UpgradeErrorMsg( msg );
          printf( "ERROR UPGRADE: %s!\n", msg.data() );
          return false;
        }
      }
      else if( src.eof() )
        break;
      else {
        printf( "ERROR UPGRADE: Error reading \"%s\"!\n", filename );
        return false;
      }
    }

    if( tb.UpgradeError() != 0 ) {
      string msg;
      tb.UpgradeErrorMsg( msg );
      printf( "ERROR UPGRADE: %s!\n", msg.data() );
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
    tb.Flush();
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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  string name, call, ts;

  tb.GetRpcTimestamp( ts );
  int version = tb.GetRpcVersion();
  int n = tb.GetRpcCallCount();

  printf( "--- DTB RPC info ----------------------------------------\n" );
  printf( "RPC version:     %i.%i\n", version / 256, version & 0xff );
  printf( "RPC timestamp:   %s\n", ts.c_str() );
  printf( "Number of calls: %i\n", n );
  printf( "Function calls:\n" );
  for( int i = 0; i < n; ++i ) {
    tb.GetRpcCallName( i, name );
    rpc_TranslateCallName( name, call );
    printf( "%5i: %s\n", i, call.c_str() );
  }
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( info )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  string s;
  tb.GetInfo( s );
  printf( "--- DTB info ------------------------------------\n%s"
          "-------------------------------------------------\n",
          s.c_str() );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( version )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  string hw;
  tb.GetHWVersion( hw );
  int fw = tb.GetFWVersion();
  int sw = tb.GetSWVersion();
  printf( "%s: FW=%i.%02i SW=%i.%02i\n", hw.c_str(), fw / 256, fw % 256,
          sw / 256, sw % 256 );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( boardid )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  int id = tb.GetBoardId();
  printf( "Board Id = %i\n", id );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( init )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Init(); // done at power up?
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( flush )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Flush(); // send buffer of USB commands to DTB
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clear )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Clear(); // rpc_io->Clear()
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
  PAR_INT( sig, 0, 131 );
  tb.SignalProbeD1( sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( d2 )
{
  int sig;
  PAR_INT( sig, 0, 131 );
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
  PAR_INT( source, 0, 1 ); // 1 = ext
  tb.SetClockSource( source );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clkok )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( tb.IsClockPresent() )
    cout << "clock OK" << endl;
  else
    cout << "clock missing" << endl;
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
  tb.Sig_SetDelay( SIG_TIN, ns +  5, duty );
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
CMD_PROC( rda )
{
  int ns;
  PAR_INT( ns, 0, 32 );
  tb.Sig_SetRdaToutDelay( ns ); // 3.5
  DO_FLUSH;
  return true;
}

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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Sig_SetLVDS();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( lcds )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Sig_SetLCDS();
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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Pon();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( poff )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Poff();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( va )
{
  int value;
  PAR_INT( value, 0, 3000 );
  tb._SetVA( value );
  dacval[0][VAx] = value;
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
  dacval[0][VDx] = value;
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
  ierror = 0; // clear error flag
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getva ) // measure analog supply voltage
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double v = tb.GetVA();
  printf( "VA %1.3f V\n", v );
  Log.printf( "VA %1.3f V\n", v );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getvd ) // measure digital supply voltage
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double v = tb.GetVD();
  printf( "VD %1.3f V\n", v );
  Log.printf( "VD %1.3f V\n", v );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getia ) // measure analog supply current
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double ia = tb.GetIA() * 1E3; // [mA]
  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;
  printf( "IA %4.1f mA for %i ROCs = %4.1f mA per ROC\n",
          ia, nrocs, ia / nrocs );
  Log.printf( "IA %4.1f mA for %i ROCs = %4.1f mA per ROC\n",
              ia, nrocs, ia / nrocs );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getid ) // measure digital supply current
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double id = tb.GetID() * 1E3; // [mA]
  printf( "ID %1.1f mA\n", id );
  Log.printf( "ID %1.1f mA\n", id );
  ierror = 0;
  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;
  if( nrocs == 1 && id > 600 ) { // current limit [mA]
    ierror = 1;
    cout << "Error: digital current too high" << endl;
    Log.printf( "ERROR\n" );
  }
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( hvon )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.HVon(); // close HV relais on DTB
  DO_FLUSH;
  Log.printf( "[HVON]" );
  HV = 1;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vb ) // set ISEG bias voltage
{
  int value;
  PAR_INT( value, 0, 2000 ); // software limit [V]
  iseg.setVoltage( value );
  Log.printf( "[SETVB] -%i\n", value );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getvb ) // measure back bias voltage
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double vb = iseg.getVoltage();
  cout << "bias voltage " << vb << " V" << endl;
  Log.printf( "VB %f V\n", vb );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getib ) // get bias current
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double ib = iseg.getCurrent();
  cout << "bias current " << ib*1E6 << " uA" << endl;
  Log.printf( "IB %f uA\n", ib*1E6 );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( scanvb ) // bias voltage scan
{
  int vmax;
  if( !PAR_IS_INT( vmax, 1, 400 ) )
    vmax = 150;
  int vstp;
  if( !PAR_IS_INT( vstp, 1, 100 ) )
    vstp = 5;

  int vmin = vstp;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int nstp = (vmax-vmin)/vstp + 1;

  Log.section( "SCANBIAS", true );

  if( h11 )
    delete h11;
  h11 = new
    TProfile( "bias_scan",
	      "bias scan;bias voltage [V];sensor bias current [uA]",
	      nstp, 0 - 0.5*vstp, vmax + 0.5*vstp );
  h11->SetStats( 10 );

  double wait = 2; // [s]

  for( int vb = vmin; vb <= vmax; vb += vstp ) {

    iseg.setVoltage( vb );

    double diff = vstp;
    int niter = 0;
    while( fabs( diff ) > 0.1 && niter < 11 ) {
      double vm = iseg.getVoltage(); // measure Vbias, negative!
      diff = fabs(vm) - vb;
      ++niter;
    }

    cout << "vbias " << setw(3) << vb << ": ";

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    double duration = 0;
    double uA = 0;

    while( duration < wait ) {

      uA = iseg.getCurrent() * 1E6; // [uA]

      gettimeofday( &tv, NULL );
      long s2 = tv.tv_sec; // seconds since 1.1.1970
      long u2 = tv.tv_usec; // microseconds
      duration = s2 - s1 + ( u2 - u1 ) * 1e-6;

      cout << " " << uA;

    }

    cout << endl;
    Log.printf( "%i %f\n", vb, uA );
    h11->Fill( vb, uA );

    h11->Draw( "hist" );
    c1->Update();

  } // bias scan

  iseg.setVoltage( vmin ); // for safety

  h11->Write();
  cout << "  histos 11" << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // scanvb

//------------------------------------------------------------------------------
CMD_PROC( ibvst ) // bias current vs time
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  Log.section( "IBVSTIME", true );

  if( h11 )
    delete h11;
  h11 = new
    TProfile( "bias_time",
	      "bias vs time;time [s];sensor bias current [uA]",
	      3600, 0, 3600, 0, 2000 );

  double wait = 1; // [s]

  double vb = iseg.getVoltage(); // measure Vbias, negative!

  cout << "vbias " << setw(3) << vb << endl;

  double runtime = 0;

  while( !keypressed() ) {

    double duration = 0;
    double uA = 0;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    while( duration < wait ) {

      uA = iseg.getCurrent() * 1E6; // [uA]

      gettimeofday( &tv, NULL );
      long s2 = tv.tv_sec; // seconds since 1.1.1970
      long u2 = tv.tv_usec; // microseconds
      duration = s2 - s1 + ( u2 - u1 ) * 1e-6;
      runtime = s2 - s0 + ( u2 - u0 ) * 1e-6;
      cout << " " << uA;

    }

    cout << endl;
    Log.printf( "%i %f\n", vb, uA );
    h11->Fill( runtime, uA );
    h11->Draw( "hist" );
    c1->Update();

  } // time

  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // ibvst

//------------------------------------------------------------------------------
CMD_PROC( hvoff )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  iseg.setVoltage( 0 );
  tb.HVoff(); // open HV relais on DTB
  DO_FLUSH;
  Log.printf( "[HVOFF]" );
  HV = 0;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( reson )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.ResetOn();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( resoff )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.ResetOff();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( select ) // define active ROCs
{
  //int rocmin, rocmax;
  //PAR_RANGE( rocmin, rocmax, 0, 15 );

  int enabledrocs;
  PAR_INT( enabledrocs, 0, 65535 );

  enabledrocslist.clear();
  nrocsa = 0; // global
  nrocsb = 0;
  int rocmin = -1;
  for( int i = 0; i < 16; ++i ) {
    roclist[i] = (enabledrocs>>i) & 1;
    if( enabledrocs >> i & 1 ) {
      enabledrocslist.push_back(i);
      if( i < 8 )
	++nrocsa;
      else
	++nrocsb;
    }
    if( roclist[i] == 1 && rocmin == -1 ) rocmin = i;
  } // i

  //tb.Daq_Deser400_OldFormat( true ); // PSI D0003 has TBM08
  tb.Daq_Deser400_OldFormat( false ); // DESY has TBM08b/a

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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
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
  if( Chip < 600 )
    tb.linearAddress = 0;
  else {
    tb.linearAddress = 1;
    cout << "PROC600 with linear address encoding" << endl;
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
  if( Chip == 505 )
    gainFileName = "c505-trim30-gaincal.dat";
  if( Chip == 501 )
    gainFileName = "c501-trim35-gaincal.dat";

  if( gainFileName.length() > 0 ) {

    ifstream gainFile( gainFileName.c_str() );

    if( gainFile ) {

      haveGain = 1;
      cout << "gainFile: " << gainFileName << endl;

      char ih[99];
      int icol;
      int irow;
      int roc = 0;

      while( gainFile >> ih ) {
	gainFile >> icol;
	gainFile >> irow;
	gainFile >> p0[roc][icol][irow];
	gainFile >> p1[roc][icol][irow];
	gainFile >> p2[roc][icol][irow];
	gainFile >> p3[roc][icol][irow];
	gainFile >> p4[roc][icol][irow];
	gainFile >> p5[roc][icol][irow];
      }

    } // gainFile open

  } // gainFileName

  return true;

} // chip

//------------------------------------------------------------------------------
CMD_PROC( module )
{
  int module;
  PAR_INT( module, 0, 9999 );

  Module = module;

  cout << "Module " << Module << endl;

  if( Module < 1000 ) {
    tb.Daq_Deser400_OldFormat( true ); // PSI D0003 has TBM08
    cout << "TBM08 with old format" << endl;
  }
  else {
    tb.Daq_Deser400_OldFormat( false ); // DESY has TBM08b/a
    cout << "TBM08abc with new format" << endl;
  }

  string gainFileName;

  if( Module == 1405 )
    gainFileName = "P1405-trim32-chiller15-gaincal.dat"; // PSI

  if( Module == 4016 )
    gainFileName = "D4016-trim32-chill17-gaincal.dat"; // E-lab
  if( Module == 4016 )
    gainFileName = "D4016-trim32UHH2-gaincal.dat"; // UHH X-rays

  if( Module == 4017 )
    //gainFileName = "D4017-ia24-trim32-chiller15-tuned-gaincal.dat";
    gainFileName = "D4017-ia24-trim32-chiller15-Vcomp_ADC1-gaincal.dat";

  if( Module == 4022 )
    gainFileName = "D4022-trim36-gaincal.dat"; // coldbox, chiller +15C

  if( Module == 4509 )
    gainFileName = "K4509_trim32_chiller15_gaincalib.dat"; // KIT

  if( gainFileName.length() > 0 ) {

    ifstream gainFile( gainFileName.c_str() );

    if( gainFile ) {

      haveGain = 1;
      cout << "gainFile: " << gainFileName << endl;

      int roc;
      int icol;
      int irow;

      while( gainFile >> roc ) {
	gainFile >> icol;
	gainFile >> irow;
	gainFile >> p0[roc][icol][irow];
	gainFile >> p1[roc][icol][irow];
	gainFile >> p2[roc][icol][irow];
	gainFile >> p3[roc][icol][irow];
	gainFile >> p4[roc][icol][irow];
	gainFile >> p5[roc][icol][irow];
      }

    } // gainFile open

  } // gainFileName

  return true;

} // module

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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Pg_Stop();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgsingle )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Pg_Single();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgtrig )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Pg_Trigger();
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
CMD_PROC( trgsel ) // 4.1.1, bitmask 16 = PG, 4 = PG_DIR, 256 = ASYNC, 2048=ASYNC_DIR
{
  int bitmask;
  PAR_INT( bitmask, 0, 4095 );
  tb.Trigger_Select(bitmask);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgdel) // 4.0
{
  int delay;
  PAR_INT( delay, 0, 255 );
  tb.Trigger_Delay(delay);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgtimeout) // 4.0
{
  int timeout;
  PAR_INT(timeout, 0, 65535);
  tb.Trigger_Timeout(timeout);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgper) // 4.0
{
  int period;
  PAR_INT(period, 0, 0x7fffffff);
  tb.Trigger_SetGenPeriodic(period);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgrand) // 4.0
{
  int rate;
  PAR_INT(rate, 0, 0x7fffffff);
  tb.Trigger_SetGenRandom(rate);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgsend) // 4.0
{
  int bitmask;
  PAR_INT(bitmask, 0, 31);
  tb.Trigger_Send(bitmask);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
// inverse decorrelated Weibull PH -> large Vcal DAC
double PHtoVcal( double ph, uint16_t roc, uint16_t col, uint16_t row )
{
  if( !haveGain )
    return ph;

  if( roc > 15 )
    return ph;

  if( col > 51 )
    return ph;

  if( row > 79 )
    return ph;

  // phcal2ps decorrelated: ph = p4 + p3*exp(-t^p2), t = p0 + q/p1
  // phroc2ps decorrelated: ph = p4 - p3*exp(-t^p2), t = p0 + q/p1

  double Ared = ph - p4[roc][col][row]; // p4 is asymptotic maximum

  if( Ared >= 0 ) {
    Ared = -0.1; // avoid overflow
  }

  double a3 = p3[roc][col][row]; // negative

  if( a3 > 0 )
    a3 = -a3; // sign changed

  // large Vcal = ( (-ln((A-p4)/p3))^1/p2 - p0 )*p1
  // phroc: q =  ( (-ln(-(A-p4)/p3))^1/p2 - p0 )*p1 // sign changed

  double vc = p1[roc][col][row] * ( pow( -log( Ared / a3 ), 1 / p2[roc][col][row] ) - p0[roc][col][row] ); // [large Vcal]

  if( vc > 999 )
    cout << "overflow " << vc << " at"
	 << setw( 3 ) << col
	 << setw( 3 ) << row << ", Ared " << Ared << ", a3 " << a3 << endl;

  if( dacval[roc][CtrlReg] == 0 )
    return vc * p5[roc][col][row]; // small Vcal

  return vc; // large Vcal
}

//------------------------------------------------------------------------------
CMD_PROC( upd ) // redraw ROOT canvas; only works for global histos
{
  int plot;
  if( !PAR_IS_INT( plot, 10, 29 ) ) {
    gPad->Modified();
    gPad->Update();
    //c1->Modified();
    //c1->Update();
  }
  else if( plot == 10 ) {
    gStyle->SetOptStat( 111111 );
    if( h10 != NULL )
      h10->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 11 ) {
    gStyle->SetOptStat( 111111 );
    if( h11 != NULL )
      h11->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 12 ) {
    gStyle->SetOptStat( 111111 );
    if( h12 != NULL )
      h12->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 13 ) {
    gStyle->SetOptStat( 111111 );
    if( h13 != NULL )
      h13->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 14 ) {
    gStyle->SetOptStat( 111111 );
    if( h14 != NULL )
      h14->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 15 ) {
    gStyle->SetOptStat( 111111 );
    if( h15 != NULL )
      h15->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 16 ) {
    gStyle->SetOptStat( 111111 );
    if( h16 != NULL )
      h16->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 17 ) {
    gStyle->SetOptStat( 111111 );
    if( h17 != NULL )
      h17->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 20 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h20 != NULL )
      h20->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 21 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h21 != NULL )
      h21->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 22 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h22 != NULL )
      h22->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 23 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h23 != NULL )
      h23->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 24 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h24 != NULL )
      h24->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 25 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h25 != NULL )
      h25->Draw( "colz" );
    c1->Update();
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
    tb.Daq_Select_Deser400();
  else
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dselmod )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Daq_Select_Deser400();
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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Daq_DeselectAll();
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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  //DP tb.Pg_SetCmd(20, PG_RESR);
  //DP tb.Pg_SetCmd(0, PG_TOK);

  vector < uint16_t > data;

  vector < std::pair < int, int > > goodvalues;

  int x, y;
  printf( "deser phs 0     1     2     3     4     5     6     7\n" );

  for( y = 0; y < 25; ++y ) { // clk

    printf( "clk %2i:", y );

    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );

    for( x = 0; x < 8; ++x ) { // deser160 phase

      tb.Daq_Select_Deser160( x );
      tb.uDelay( 10 );

      tb.Daq_Start();

      tb.uDelay( 10 );

      tb.Pg_Single();

      tb.uDelay( 10 );

      tb.Daq_Stop();

      data.resize( tb.Daq_GetSize(), 0 );

      tb.Daq_Read( data, 100 );

      if( data.size() ) {
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
  tb.Daq_Close();
#endif

  printf( "Old values: clk delay %i, deserPhase %i\n",
          tbState.GetClockPhase(), tbState.GetDeserPhase() );

  if( goodvalues.size() == 0 ) {

    printf
      ( "No value found where header could be read back - no adjustments made.\n" );
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() ); // back to default
    y = tbState.GetClockPhase(); // back to default
    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );
    return true;
  }
  printf( "Good values are:\n" );
  for( std::vector < std::pair < int, int > >::const_iterator it =
	 goodvalues.begin(); it != goodvalues.end(); ++it ) {
    printf( "%i %i\n", it->first, it->second );
  }
  const int select = floor( 0.5 * goodvalues.size() - 0.5 );
  tbState.SetClockPhase( goodvalues[select].first );
  tbState.SetDeserPhase( goodvalues[select].second );
  printf( "New values: clock delay %i, deserPhase %i\n",
          tbState.GetClockPhase(), tbState.GetDeserPhase() );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() ); // set new
  y = tbState.GetClockPhase();
  tb.Sig_SetDelay( SIG_CLK, y );
  tb.Sig_SetDelay( SIG_CTR, y );
  tb.Sig_SetDelay( SIG_SDA, y + 15 );
  tb.Sig_SetDelay( SIG_TIN, y + 5 );

  return true;

} // deser160

//------------------------------------------------------------------------------
CMD_PROC( deserext ) // scan DESER160 phase for header 7F8 with ext trig
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif

  vector < uint16_t > data;

  vector < std::pair < int, int > > goodvalues;

  tb.Trigger_Select(256); // ext TRG

  int x, y;
  printf( "deser phs 0     1     2     3     4     5     6     7\n" );

  for( y = 0; y < 25; ++y ) { // clk

    printf( "clk %2i:", y );

    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );

    for( x = 0; x < 8; ++x ) { // deser160 phase

      tb.Daq_Select_Deser160( x );
      tb.uDelay( 10 );

      tb.Daq_Start();

      tb.uDelay( 100 );

      tb.Daq_Stop();

      data.resize( tb.Daq_GetSize(), 0 );

      tb.Daq_Read( data, 100 );

      if( data.size() ) {
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
  tb.Daq_Close();
#endif

  printf( "Old values: clk delay %i, deserPhase %i\n",
          tbState.GetClockPhase(), tbState.GetDeserPhase() );

  if( goodvalues.size() == 0 ) {

    printf
      ( "No value found where header could be read back - no adjustments made.\n" );
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() ); // back to default
    y = tbState.GetClockPhase(); // back to default
    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );
    return true;
  }
  printf( "Good values are:\n" );
  for( std::vector < std::pair < int, int > >::const_iterator it =
	 goodvalues.begin(); it != goodvalues.end(); ++it ) {
    printf( "%i %i\n", it->first, it->second );
  }
  const int select = floor( 0.5 * goodvalues.size() - 0.5 );
  tbState.SetClockPhase( goodvalues[select].first );
  tbState.SetDeserPhase( goodvalues[select].second );
  printf( "New values: clock delay %i, deserPhase %i\n",
          tbState.GetClockPhase(), tbState.GetDeserPhase() );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() ); // set new
  y = tbState.GetClockPhase();
  tb.Sig_SetDelay( SIG_CLK, y );
  tb.Sig_SetDelay( SIG_CTR, y );
  tb.Sig_SetDelay( SIG_SDA, y + 15 );
  tb.Sig_SetDelay( SIG_TIN, y + 5 );

  return true;

} // deserext

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
void DecodeTbmHeader08( unsigned int raw )
{
  int evNr = raw >> 8;
  int stkCnt = raw & 6;
  printf( "  EV(%3i) STF(%c) PKR(%c) STKCNT(%2i)",
          evNr,
          ( raw & 0x0080 ) ? '1' : '0',
          ( raw & 0x0040 ) ? '1' : '0', stkCnt );
}

//------------------------------------------------------------------------------
void DecodeTbmTrailer08( unsigned int raw )
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
void DecodeTbmHeader( unsigned int raw ) // 08abc
{
  int data = raw & 0x3f;
  int dataId = ( raw >> 6 ) & 0x3;
  int evNr = raw >> 8;
  printf( "  EV(%3i) D%i(%2i)", evNr, dataId, data );
}

//------------------------------------------------------------------------------
void DecodeTbmTrailer( unsigned int raw ) // 08bc
{
  int stkCnt = raw & 0x3f; // 3f = 11'1111
  printf
    ( "  NTP(%c) RST(%c) RSR(%c) SYE(%c) SYT(%c) CTC(%c) CAL(%c) SF(%c) ARS(%c) PRS(%c) STKCNT(%2i)", // DP bug fix: ARS-PKR order
      ( raw & 0x8000 ) ? '1' : '0',
      ( raw & 0x4000 ) ? '1' : '0',
      ( raw & 0x2000 ) ? '1' : '0',
      ( raw & 0x1000 ) ? '1' : '0',
      ( raw & 0x0800 ) ? '1' : '0',
      ( raw & 0x0400 ) ? '1' : '0',
      ( raw & 0x0200 ) ? '1' : '0',
      ( raw & 0x0100 ) ? '1' : '0',
      ( raw & 0x0080 ) ? '1' : '0',
      ( raw & 0x0040 ) ? '1' : '0',
      stkCnt );
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
  Decoder():printEvery( 0 ), nReadout( 0 ), nPixel( 0 ), f( 0 ),
	      nSamples( 0 ), samples( 0 )
  {
  }
  ~Decoder()
  {
    Close();
  }
  bool Open( const char *filename );
  void Close()
  {
    if( f )
      fclose( f );
    f = 0;
    delete[]samples;
  }
  bool Sample( uint16_t sample );
  void DumpSamples( int n );
  void Translate( unsigned long raw );
  uint16_t GetX()
  {
    return x;
  }
  uint16_t GetY()
  {
    return y;
  }
  uint16_t GetPH()
  {
    return ph;
  }
  void AnalyzeSamples();
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
      AnalyzeSamples();
      printEvery = 0;
    }
    else
      ++printEvery;
    ++nReadout;
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

  y = 80;
  x = 0;

  if( Chip < 600 ) {

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
  } // psi46dig
  else {
    y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
    x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
  } // proc600

} // Translate

//------------------------------------------------------------------------------
void Decoder::AnalyzeSamples()
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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;
  cout << nrocs << " ROCs defined" << endl;
  int tbmData = 0;
  if( nrocs > 1 )
    tbmData = 2; // header and trailer words

  uint32_t words_remaining = 0;
  vector < uint16_t > data;

  tb.Daq_Read( data, Blocksize, words_remaining );

  int size = data.size();

  cout << "words read: " << size
       << " = " << ( size - nrocs - tbmData ) / 2 << " pixels"
       << endl;
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
  Log.flush();

  cout << "PixelAddressInverted " << tb.GetPixelAddressInverted() << endl;

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

  for( unsigned int i = 0; i < data.size(); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
      if( ldb )
        printf( "%X\n", data[i] );
      even = 0;
    }
    else if( even ) { // merge 2 words into one pixel int:

      if( i < data.size() - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        dec.Translate( raw );

        uint16_t ix = dec.GetX();
        uint16_t iy = dec.GetY();
        uint16_t ph = dec.GetPH();
        double vc = ph;
        if( ldb )
          vc = PHtoVcal( ph, 0, ix, iy );
        if( ldb )
          cout << setw( 3 ) << ix << setw( 3 ) << iy << setw( 4 ) << ph <<
            ",";
        if( ldb )
          cout << "(" << vc << ")";
        if( npx % 8 == 7 && ldb )
          cout << endl;
        if( ix < 52 && iy < 80 ) {
          ++nn[ix][iy]; // hit map
          if( iy > ymax )
            ymax = iy;
          h21->Fill( ix, iy );
        }
        else
          ++nrr; // error
        ++npx;
      }
    }
    even = 1 - even;
  } // data
  if( ldb )
    cout << endl;

  cout << npx << " pixels" << endl;
  cout << nrr << " address errors" << endl;

  h21->Write();
  gStyle->SetOptStat( 10 ); // entries
  gStyle->SetStatY( 0.95 );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 21" << endl;

  // hit map:
  /*
    if( npx > 0 ) {
    if( ymax < 79 ) ++ymax; // print one empty
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

} // dread

//------------------------------------------------------------------------------
CMD_PROC( dreadm ) // module
{
  int channel;
  if( !PAR_IS_INT( channel, 0, 1 ) )
    channel = 0;

  uint32_t words_remaining = 0;
  vector < uint16_t > data;

  tb.Daq_Read( data, Blocksize, words_remaining, channel );

  int size = data.size();
  printf( "#samples: %i  remaining: %u\n", size, words_remaining );

  unsigned int hdr = 0, trl = 0;
  unsigned int raw = 0;

  Decoder dec;

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleHitMap",
                  "Module hit map;col;row;hits",
                  8 * 52, -0.5, 8 * 52 - 0.5,
                  2 * 80, -0.5, 2 * 80 - 0.5 );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis()->SetTitleOffset( 1.3 );

  // int TBM_eventnr,TBM_stackinfo,ColAddr,RowAddr,PulseHeight,TBM_trailerBits,TBM_readbackData;

  int32_t kroc = -1; // will start at 0
  if( channel == 1 )
    kroc = 7; // will start at 8

  for( int i = 0; i < size; ++i ) {

    int d = data[i] & 0x0fff; // 12 data bits

    uint16_t v = ( data[i] >> 12 ) & 0xe; // e = 14 = 1110, flag

    switch ( v ) {

    case 10:
      printf( "TBM H12(%03X)", d );
      hdr = d;
      break;
    case 8:
      printf( "+H34(%03X) =", d );
      hdr = ( hdr << 8 ) + d;
      DecodeTbmHeader( hdr );
      kroc = -1; // will start at 0
      if( channel == 1 )
	kroc = 7; // will start at 8
      break;

    case 4:
      printf( "\nROC-HEADER(%03X): ", d );
      ++kroc; // start at 0 or 8
      break;

    case 0:
      printf( "\nR123(%03X)", d );
      raw = d;
      break;
    case 2:
      printf( "+R456(%03X)", d );
      raw = ( raw << 12 ) + d;
      DecodePixel( raw );
      dec.Translate( raw );
      {
	uint16_t ix = dec.GetX();
	uint16_t iy = dec.GetY();
	//uint16_t ph = dec.GetPH();
	int l = kroc % 8; // 0..7
	int xm = 52 * l + ix; // 0..415  rocs 0 1 2 3 4 5 6 7
	int ym = iy; // 0..79
	if( kroc > 7 ) {
	  xm = 415 - xm; // rocs 8 9 A B C D E F
	  ym = 159 - ym; // 80..159
	}
	h21->Fill( xm, ym );
      }
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
  printf( "\n" );
  /*
  for( int i = 0; i < size; ++i ) {
    int x = data[i] & 0xffff;
    Log.printf( " %04X", x );
    if( i % 100 == 99 )
      Log.printf( "\n" );
  }
  Log.printf( "\n" );
  Log.flush();
  */
  h21->Write();
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 21" << endl;

  return true;

} // dreadm

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 14.12.2015, scan TBM delays, PG trigger, until first error
CMD_PROC( tbmscan )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif

  tb.Pg_Stop(); // stop DTB RCTT pattern generator

  tb.Daq_Select_Deser400();
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint32_t remaining0 = 0;
  uint32_t remaining1 = 0;
  vector < uint16_t > data0;
  vector < uint16_t > data1;
  data0.reserve( Blocksize );
  data1.reserve( Blocksize );

  vector < int > errcnt(256);

  for( int itin = 0; itin < 2; ++itin )
    for( int itbm = 0; itbm < 2; ++itbm )
      for( int ipt1 = 0; ipt1 < 8; ++ipt1 )
	for( int ipt0 = 0; ipt0 < 8; ++ipt0 ) {

	  tb.Pg_Stop(); // quiet

	  tb.tbm_Set( 0xE4, 0xF4 ); // clear TBM, reset ROCs
	  tb.tbm_Set( 0xF4, 0xF4 ); // clear TBM, reset ROCs

	  int ibyte = 128*itin + 64*itbm + 8*ipt1 + ipt0;

	  tb.tbm_Set( 0xEA, ibyte );
	  tb.tbm_Set( 0xFA, ibyte );

	  cout << setw(3) << ibyte;

	  tb.Flush();

	  unsigned int nev[2] = { 0, 0 };
	  unsigned int nrr[2] = { 0, 0 };

	  uint32_t trl = 0; // need to remember from previous daq_read

	  bool ldb = 0;

	  tb.Daq_Deser400_Reset( 3 );
	  tb.uDelay( 100 );
	  tb.Daq_Start( 0 );
	  tb.Daq_Start( 1 );
	  tb.uDelay( 100 );

	  tb.Pg_Single();

	  tb.uDelay( 10 );
	  tb.Daq_Stop( 0 );
	  tb.Daq_Stop( 1 );

	  tb.Daq_Read( data0, Blocksize, remaining0, 0 );
	  tb.Daq_Read( data1, Blocksize, remaining1, 1 );

	  // decode data:

	  for( int ch = 0; ch < 2; ++ch ) {

	    int size = data0.size();
	    if( ch == 1 )
	      size = data1.size();
	    uint32_t raw = 0;
	    uint32_t hdr = 0;
	    int32_t kroc = -1; // will start at 0
	    if( ch == 1 )
	      kroc = 7; // will start at 8

	    for( int ii = 0; ii < size; ++ii ) {

	      int d = 0;
	      int v = 0;
	      if( ch == 0 ) {
		d = data0[ii] & 0xfff; // 12 bits data
		v = ( data0[ii] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110
	      }
	      else {
		d = data1[ii] & 0xfff; // 12 bits data
		v = ( data1[ii] >> 12 ) & 0xe; // 3 flag bits
	      }
	      int c = 0;
	      int r = 0;
	      int x = 0;
	      int y = 0;

	      switch ( v ) {

		// TBM header:
	      case 10:
		hdr = d;
		if( nev[ch] > 0 && trl == 0 ) {
		  cout << "TBM error: header without previous trailer in event "
		       << nev[ch]
		       << ", channel " << ch
		       << endl;
		  ++nrr[ch];
		}
		trl = 0;
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
		break;

		// ROC header data:
	      case 4:
		++kroc; // start at 0
		if( ldb ) {
		  if( kroc > 0 )
		    cout << endl;
		  cout << "ROC " << setw( 2 ) << kroc;
		}
		if( kroc > 15 ) {
		  cout << "Error kroc " << kroc << endl;
		  kroc = 15;
		  ++nrr[ch];
		}
		if( kroc == 0 && hdr == 0 ) {
		  cout << "TBM error: no header in event " << nev[ch]
		       << " ch " << ch
		       << endl;
		  ++nrr[ch];
		}
		hdr = 0;
		break;

		// pixel data:
	      case 0:
		raw = d;
		break;
	      case 2:
		raw = ( raw << 12 ) + d;
		raw >>= 9;
		c = ( raw >> 12 ) & 7;
		c = c * 6 + ( ( raw >> 9 ) & 7 );
		r = ( raw >> 6 ) & 7;
		r = r * 6 + ( ( raw >> 3 ) & 7 );
		r = r * 6 + ( raw & 7 );
		y = 80 - r / 2;
		x = 2 * c + ( r & 1 );
		if( ldb )
		  cout << " " << x << "." << y;
		if( kroc < 0 || ( ch == 1 && kroc < 8 ) || kroc > 15 ) {
		  cout << "ROC data with wrong ROC count " << kroc
		       << " in event " << nev[ch]
		       << endl;
		  ++nrr[ch];
		}
		else if( y > -1 && y < 80 && x > -1 && x < 52 ) {
		} // valid px
		else {
		  cout << "invalid col row " << setw(2) << x
		       << setw(3) << y
		       << " in ROC " << setw(2) << kroc
		       << endl;
		  ++nrr[ch];
		}
		break;

		// TBM trailer:
	      case 14:
		trl = d;
		if( ch == 0 && kroc != 7 ) {
		  cout
		    << "wrong ROC count " << kroc
		    << " in event " << nev[ch] << " ch 0" 
		    << endl;
		  ++nrr[ch];
		}
		else if( ch == 1 && kroc != 15 ) {
		  cout
		    << "wrong ROC count " << kroc
		    << " in event " << nev[ch] << " ch 1" 
		    << endl;
		  ++nrr[ch];
		}
		break;
	      case 12:
		trl = ( trl << 8 ) + d;
		if( ldb ) {
		  cout << endl;
		  cout << "TBM trailer";
		  DecodeTbmTrailer( trl );
		  cout << endl;
		}
		++nev[ch];
		trl = 1; // flag
		break;

	      default:
		printf( "\nunknown data: %X = %d", v, v );
		++nrr[ch];
		break;

	      } // switch

	    } // data

	  } // ch

	  if( ldb )
	    cout << endl;

	  cout << ": size " << data0.size() + data1.size()
	       << ", err " << nrr[0] + nrr[1]
	       << endl;

	  errcnt.at(ibyte) = -nrr[0] - nrr[1];

	} // tbmset

  // all off:

  tb.Daq_Stop( 0 );
  tb.Daq_Stop( 1 );
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  cout << endl;
  for( int itin = 0; itin < 2; ++itin )
    for( int itbm = 0; itbm < 2; ++itbm ) {
      cout << "TIN delay " << itin*6.25
	   << "ns, TBM header/trailer delay " << itbm*6.25
	   << "ns:" << endl;
      cout << "port:";
      for( int ipt0 = 0; ipt0 < 8; ++ipt0 )
	cout << setw(8) << ipt0;
      cout << endl;
      for( int ipt1 = 0; ipt1 < 8; ++ipt1 ) {
	cout << setw(4) << ipt1 <<":";
	for( int ipt0 = 0; ipt0 < 8; ++ipt0 ){
	  int ibyte = 128*itin + 64*itbm + 8*ipt1 + ipt0;
	  cout << setw(8) << errcnt.at(ibyte);
	}
	cout << endl;
      }
    }

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tbmscan

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 14.12.2015, scan TBM delays, random trigger, until first error
CMD_PROC( tbmscant )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // enable all pixels:

  int masked = 0;

  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.roc_ClrCal();
      for( int col = 0; col < 52; ++col ) {
        tb.roc_Col_Enable( col, 1 );
        for( int row = 0; row < 80; ++row ) {
          int trim = modtrm[roc][col][row];
	  if( trim > 15 ) {
	    tb.roc_Pix_Mask( col, row );
	    ++masked;
	    cout << "mask roc col row "
		 << setw(2) << roc
		 << setw(3) << col
		 << setw(3) << row
		 << endl;
	  }
	  else
          tb.roc_Pix_Trim( col, row, trim );
        }
      }
    }
  tb.Flush();

  uint32_t remaining0 = 0;
  uint32_t remaining1 = 0;
  vector < uint16_t > data0;
  vector < uint16_t > data1;
  data0.reserve( Blocksize );
  data1.reserve( Blocksize );

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleHitMap",
		  "Module hit map;col;row;hits",
		  8 * 52, -0.5, 8 * 52 - 0.5,
		  2 * 80, -0.5, 2 * 80 - 0.5 );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis()->SetTitleOffset( 1.3 );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif

  tb.Pg_Stop(); // stop DTB RCTT pattern generator

  tb.Daq_Select_Deser400();
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  tb.Daq_Start( 0 );
  tb.Daq_Start( 1 );
  tb.uDelay( 1000 );

  for( int itin = 0; itin < 2; ++itin )
    for( int itbm = 0; itbm < 2; ++itbm )
      for( int ipt1 = 0; ipt1 < 8; ++ipt1 )
	for( int ipt0 = 0; ipt0 < 8; ++ipt0 ) {

	  tb.Trigger_Select(0); // quiet

	  tb.tbm_Set( 0xE4, 0xF4 ); // clear TBM, reset ROCs
	  tb.tbm_Set( 0xF4, 0xF4 ); // clear TBM, reset ROCs

	  tb.tbm_Set( 0xEA, 128*itin + 64*itbm + 8*ipt1 + ipt0 );
	  tb.tbm_Set( 0xFA, 128*itin + 64*itbm + 8*ipt1 + ipt0 );

	  tb.Trigger_SetGenRandom(10*1000*1000); // 90 kHz randoms

	  tb.Trigger_Select(512); // start GEN_DIR trigger generator
	  tb.Flush();

	  unsigned int nev[2] = { 0, 0 };
	  unsigned int ndq = 0;
	  unsigned int got = 0;
	  unsigned int npx = 0;
	  unsigned int nrr[2] = { 0, 0 };

	  uint32_t trl = 0; // need to remember from previous daq_read

	  bool ldb = 0;

	  bool roerr = 0;

	  while( !roerr && nev[0] < 1000*1000 ) {

	    tb.mDelay( 200 ); // [ms]

	    tb.Daq_Read( data0, Blocksize, remaining0, 0 );
	    tb.Daq_Read( data1, Blocksize, remaining1, 1 );

	    ++ndq;
	    got += data0.size();
	    got += data1.size();

	    gettimeofday( &tv, NULL );
	    long s2 = tv.tv_sec; // seconds since 1.1.1970
	    long u2 = tv.tv_usec; // microseconds

	    // decode data:

	    for( int ch = 0; ch < 2; ++ch ) {

	      int size = data0.size();
	      if( ch == 1 )
		size = data1.size();
	      uint32_t raw = 0;
	      uint32_t hdr = 0;
	      int32_t kroc = -1; // will start at 0
	      if( ch == 1 )
		kroc = 7; // will start at 8
	      unsigned int npxev = 0;

	      for( int ii = 0; ii < size; ++ii ) {

		int d = 0;
		int v = 0;
		if( ch == 0 ) {
		  d = data0[ii] & 0xfff; // 12 bits data
		  v = ( data0[ii] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110
		}
		else {
		  d = data1[ii] & 0xfff; // 12 bits data
		  v = ( data1[ii] >> 12 ) & 0xe; // 3 flag bits
		}
		int c = 0;
		int r = 0;
		int x = 0;
		int y = 0;

		switch ( v ) {

		  // TBM header:
		case 10:
		  hdr = d;
		  npxev = 0;
		  if( nev[ch] > 0 && trl == 0 ) {
		    cout << "TBM error: header without previous trailer in event "
			 << nev[ch]
			 << ", channel " << ch
			 << endl;
		    ++nrr[ch];
		    roerr = 1;
		  }
		  trl = 0;
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
		  break;

		  // ROC header data:
		case 4:
		  ++kroc; // start at 0
		  if( ldb ) {
		    if( kroc > 0 )
		      cout << endl;
		    cout << "ROC " << setw( 2 ) << kroc;
		  }
		  if( kroc > 15 ) {
		    cout << "Error kroc " << kroc << endl;
		    kroc = 15;
		    ++nrr[ch];
		    roerr = 1;
		  }
		  if( kroc == 0 && hdr == 0 ) {
		    cout << "TBM error: no header in event " << nev[ch]
			 << " ch " << ch
			 << endl;
		    ++nrr[ch];
		    roerr = 1;
		  }
		  hdr = 0;
		  break;

		  // pixel data:
		case 0:
		  raw = d;
		  break;
		case 2:
		  raw = ( raw << 12 ) + d;
		  raw >>= 9;
		  c = ( raw >> 12 ) & 7;
		  c = c * 6 + ( ( raw >> 9 ) & 7 );
		  r = ( raw >> 6 ) & 7;
		  r = r * 6 + ( ( raw >> 3 ) & 7 );
		  r = r * 6 + ( raw & 7 );
		  y = 80 - r / 2;
		  x = 2 * c + ( r & 1 );
		  if( ldb )
		    cout << " " << x << "." << y;
		  if( kroc < 0 || ( ch == 1 && kroc < 8 ) || kroc > 15 ) {
		    cout << "ROC data with wrong ROC count " << kroc
			 << " in event " << nev[ch]
			 << endl;
		    ++nrr[ch];
		    roerr = 1;
		  }
		  else if( y > -1 && y < 80 && x > -1 && x < 52 ) {
		    ++npx;
		    ++npxev;
		    int l = kroc % 8; // 0..7
		    int xm = 52 * l + x; // 0..415  rocs 0 1 2 3 4 5 6 7
		    int ym = y; // 0..79
		    if( kroc > 7 ) {
		      xm = 415 - xm; // rocs 8 9 A B C D E F
		      ym = 159 - y; // 80..159
		    }
		    h21->Fill( xm, ym );
		  } // valid px
		  else {
		    cout << "invalid col row " << setw(2) << x
			 << setw(3) << y
			 << " in ROC " << setw(2) << kroc
			 << endl;
		    ++nrr[ch];
		    roerr = 1;
		  }
		  break;

		  // TBM trailer:
		case 14:
		  trl = d;
		  if( ch == 0 && kroc != 7 ) {
		    cout
		      << "wrong ROC count " << kroc
		      << " in event " << nev[ch] << " ch 0" 
		      << endl;
		    ++nrr[ch];
		    roerr = 1;
		  }
		  else if( ch == 1 && kroc != 15 ) {
		    cout
		      << "wrong ROC count " << kroc
		      << " in event " << nev[ch] << " ch 1" 
		      << endl;
		    ++nrr[ch];
		    roerr = 1;
		  }
		  break;
		case 12:
		  trl = ( trl << 8 ) + d;
		  if( ldb ) {
		    cout << endl;
		    cout << "TBM trailer";
		    DecodeTbmTrailer( trl );
		    cout << endl;
		  }
		  ++nev[ch];
		  trl = 1; // flag
		  break;

		default:
		  printf( "\nunknown data: %X = %d", v, v );
		  ++nrr[ch];
		  roerr = 1;
		  break;

		} // switch

	      } // data

	    } // ch

	    if( ldb )
	      cout << endl;

	    h21->Draw( "colz" );
	    c1->Update();

	    cout << s2 - s0 + ( u2 - u0 ) * 1e-6 << " s"
		 << ",  calls " << ndq
		 << ",  last " << data0.size() + data1.size()
		 << ",  rest " << remaining0 + remaining1
		 << ",  trig " << nev[0]
		 << ",  byte " << got
		 << ",  pix " << npx
		 << ",  err " << nrr[0] + nrr[1]
		 << endl;

	  } // while

	  // daq_read for remaining ?

	} // tbmset

  // all off:

  tb.Trigger_Select(4); // back to default PG trigger

  tb.Daq_Stop( 0 );
  tb.Daq_Stop( 1 );
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tbmscan

//------------------------------------------------------------------------------
CMD_PROC( takedata ) // takedata period (ROC, trigger f = 40 MHz / period)
// period = 1 = flag for external trigger

// ext trig: event rate = 0.5 trigger rate ? 15.5.2015 TB 22

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
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // run number from file:

  unsigned int run = 1;
  ifstream irunFile( "runNumber.dat" );
  irunFile >> run;
  irunFile.close();

  ++run;

  ofstream orunFile( "runNumber.dat" );
  orunFile << run;
  orunFile.close();

  // output file:

  ostringstream fname; // output string stream

  fname << "run" << run << ".out";

  ofstream outFile( fname.str().c_str() );

  if( h10 )
    delete h10;
  h10 = new
    TH1D( "pixels",
          "pixels per trigger;multiplicity [pixel];triggers",
          101, -0.5, 100.5 );
  h10->Sumw2();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "pixelPH", "pixel PH;pixel PH [ADC];pixels",
	  255, -0.5, 254.5 ); // 255 is overflow
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "pixel_charge",
          dacval[0][CtrlReg] == 0 ?
          "pixel charge;pixel charge [small Vcal DAC];pixels" :
          "pixel charge;pixel charge [large Vcal DAC];pixels",
          256, -0.5, 255.5 );
  h12->Sumw2();

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
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );

  if( period == 1 ) // flag for ext trig
    tb.Pg_Stop(); // stop triggers, necessary for clean data
  else
    tb.Pg_Loop( period ); // not with ext trig

  tb.SetTimeout( 2000 ); // [ms] USB
  tb.Flush();

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

  while( !keypressed() ) {

    tb.mDelay( 500 ); // once per second

    if( period > 1 ) // flag for ext trig
      tb.Pg_Stop(); // stop triggers, necessary for clean data

    tb.uDelay( 100 );

    if( period > 1 ) // flag for ext trig
      tb.Daq_Stop();
    tb.uDelay( 100 );
    //tb.uDelay(4000); // better
    //tb.uDelay(9000); // some overflow events (rest)

    tb.Daq_Read( data, Blocksize, remaining );

    ++ndq;

    //tb.uDelay(1000);
    if( period > 1 ) // flag for ext trig
      tb.Daq_Start();
    tb.uDelay( 100 );
    if( period > 1 ) // flag for ext trig
      tb.Pg_Loop( period ); // start triggers
    tb.Flush();

    got += data.size();
    rst += remaining;

    gettimeofday( &tv, NULL );
    long s9 = tv.tv_sec; // seconds since 1.1.1970
    long u9 = tv.tv_usec; // microseconds
    duration = s9 - s0 + ( u9 - u0 ) * 1e-6;

    if( duration - tprev > 0.999 ) {

      cout << duration << " s"
	   << ", last " << data.size()
	   << ",  rest " << remaining
	   << ",  daq " << ndq
	   << ",  trg " << nev
	   << ",  sum " << got
	   << ",  rest " << rst
	   << ",  pix " << npx
	   << ",  err " << nrr
	   << ", file " << outFile.tellp() // put pointer [bytes]
	   << endl;

      h21->Draw( "colz" );
      c1->Update();

      tprev = duration;

    }

    // decode data:

    int npxev = 0;
    bool even = 0;
    bool tbm = 0;
    int phase = 0;

    for( unsigned int i = 0; i < data.size(); ++i ) {

      // Simon mail 26.1.2015
      // ext trigger: soft TBM header and trailer

      bool tbmend = 0;

      if( (      data[i] & 0xff00 ) == 0xA000 ) { // TBM header
        even = 0;
	tbm = 1;
      }
      else if( ( data[i] & 0xff00 ) == 0x8000 ) { // TBM header
        even = 0;
	tbm = 1;
	phase = ( data[i] & 0xf ); // phase trig vs clk 0..9
      }
      else if( ( data[i] & 0xff00 ) == 0xE000 ) { // TBM trailer
        even = 0;
      }
      else if( ( data[i] & 0xff00 ) == 0xC000 ) { // TBM trailer
        even = 0;
	tbmend = 1;
      }
      else if( ( data[i] & 0xffc ) == 0x7f8 ) { // ROC header
        if( ldb && i > 0 )
          cout << endl;
        if( ldb )
          printf( "%X", data[i] );
        even = 0;
        npxev = 0;
        ++nev;
      }
      else if( even ) { // 24 data bits come in 2 words

        if( i < data.size() - 1 ) {

          unsigned long raw = ( data[i] & 0xfff ) << 12;
          raw += data[i + 1] & 0xfff; // even + odd

          ++npx;
          ++npxev;

          dec.Translate( raw );

          uint16_t ix = dec.GetX();
          uint16_t iy = dec.GetY();
          uint16_t ph = dec.GetPH();
	  if( npxev == 1 ) { // non-empty event
	    outFile << nev;
	    if( tbm )
	      outFile << " " << phase;
	  }
	  outFile << " " << ix << " " << iy << " " << ph;
          double vc = PHtoVcal( ph, 0, ix, iy );
          if( ldb )
            cout << " " << ix << "." << iy
		 << ":" << ph << "(" << ( int ) vc << ")";
          h11->Fill( ph );
          h12->Fill( vc );
          h21->Fill( ix, iy );
          h22->Fill( ix, iy, vc );

          if( ix < 52 && iy < 80 ) {
            ++NN[ix][iy]; // hit map
            if( ph > 0 && ph < 256 )
              ++PH[ph];
          }
          else
            ++nrr;
	}
        else { // truncated pixel
          ++nrr;
	  cout << " err at " << i << " in " << data.size() << endl;
        }

      } // even

      even = 1 - even;

      // $4000 = 0100'0000'0000'0000  FPGA end marker
      // $8000 = 1000'0000'0000'0000  soft TBM header
      // $A000 = 1010'0000'0000'0000  soft TBM header
      // $C000 = 1100'0000'0000'0000  soft TBM trailer
      // $E000 = 1110'0000'0000'0000  soft TBM trailer

      bool lend = 0;
      if( tbm ) {
	if( tbmend )
	  lend = 1;
      }
      else if( ( data[i] & 0x4000 ) == 0x4000 ) // FPGA end marker
	lend = 1;

      if( lend ) {
        h10->Fill( npxev );
	if( npxev > 0 ) outFile << endl;
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

  tb.Daq_Stop();
  if( period > 1 ) // flag for ext trig
    tb.Pg_Stop(); // stop triggers, necessary for clean re-start
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
#endif
  tb.SetTimeout( 200000 ); // restore [ms] USB
  tb.Flush();

  outFile.close(); // redundant

  uint32_t nmx = 0;
  if( npx > 0 ) {
    for( int row = 79; row >= 0; --row ) {
      cout << setw( 2 ) << row << ": ";
      for( int col = 0; col < 52; ++col ) {
	cout << " " << NN[col][row];
        if( NN[col][row] > nmx )
          nmx = NN[col][row];
      }
      cout << endl;
    }

    // increase trim bits:

    cout << endl;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        if( NN[col][row] > nev / 100 )
          cout << "pixt 0"
	       << setw(3) << col
	       << setw(3) << row
	       << setw(3) << modtrm[0][col][row] + 1
	       << setw(7) << NN[col][row]
	       << endl;
    cout << endl;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        if( NN[col][row] > nev / 1000 )
          cout << "pixt 0"
	       << setw(3) << col
	       << setw(3) << row
	       << setw(3) << modtrm[0][col][row] + 1
	       << setw(7) << NN[col][row]
	       << endl;
    cout << endl;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        if( NN[col][row] > nev / 10000 )
          cout << "pixt 0"
	       << setw(3) << col
	       << setw(3) << row
	       << setw(3) << modtrm[0][col][row] + 1
	       << setw(7) << NN[col][row]
	       << endl;
  }
  cout << "sum " << npx << ", max " << nmx << endl;

  h10->Write();
  h11->Write();
  h12->Write();
  h21->Write();
  h22->Write();
  gStyle->SetOptStat( 10 ); // entries
  gStyle->SetStatY( 0.95 );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 10, 11, 12, 21, 22" << endl;

  cout << endl;
  cout << fname.str() << endl;
  cout << "duration    " << duration << endl;
  cout << "triggers    " << nev << " = " << nev / duration << " Hz" << endl;
  cout << "DAQ calls   " << ndq << endl;
  cout << "words read  " << got << ", remaining " << rst << endl;
  cout << "data rate   " << 2 * got / duration << " bytes/s" << endl;
  cout << "pixels      " << npx << " = " << npx /
    ( double ) nev << "/ev " << endl;
  cout << "data errors " << nrr << endl;

  return true;

} // takedata

//------------------------------------------------------------------------------
CMD_PROC( tdscan ) // takedata vs VthrComp: X-ray threshold method

// allon
// stretch 1 8 999
// tdscan 50 150
{
  int vmin;
  PAR_INT( vmin, 0, 255 );
  int vmax;
  PAR_INT( vmax, vmin, 255 );
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 10000;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  if( h10 )
    delete h10;
  h10 = new
    TH1D( "pixels",
          "pixels per trigger;multiplicity [pixel];triggers",
          101, -0.5, 100.5 );
  h10->Sumw2();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "pixelPH", "pixel PH;pixel PH [ADC];pixels",
	  255, -0.5, 254.5 ); // 255 is overflow
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "pixel_charge",
          dacval[0][CtrlReg] == 0 ?
          "pixel charge;pixel charge [small Vcal DAC];pixels" :
          "pixel charge;pixel charge [large Vcal DAC];pixels",
          256, -0.5, 255.5 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( "flux",
	  "flux above threshold;VthrComp [DAC];flux above threshold",
	  vmax-vmin+1, vmin-0.5, vmax+0.5 );
  h13->Sumw2();

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
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.Pg_Stop(); // stop triggers (who knows what was going on before?)
  tb.SetTimeout( 2000 ); // [ms] USB
  tb.Flush();

  double dttrig = 0;
  double dtread = 0;

  vector < uint16_t > data;
  uint32_t remaining;
  Decoder dec;

  // threshold scan:

  for( int vthr = vmin; vthr <= vmax; ++vthr ) {

    tb.SetDAC( VthrComp, vthr );

    // set CalDel? not needed for random hits with clock stretch

    tb.uDelay( 10000 );
    tb.Flush();

    // take data:

    tb.Daq_Start();
    tb.uDelay( 1000 );

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    for( int k = 0; k < nTrig; ++k ) {
      tb.Pg_Single();
      tb.uDelay( 20 );
    }
    tb.Flush();

    tb.Daq_Stop();
    tb.uDelay( 100 );
    tb.Daq_GetSize(); // read statement ends trig block for timer

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;
    dttrig += s2 - s1 + ( u2 - u1 ) * 1e-6;

    tb.Daq_Read( data, Blocksize, remaining );

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

    // decode data:

    unsigned int nev = 0;
    unsigned int npx = 0;
    unsigned int nrr = 0;
    unsigned int npxev = 0;
    bool even = 1;

    for( unsigned int i = 0; i < data.size(); ++i ) {

      if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
        even = 0;
        npxev = 0;
        ++nev;
      }
      else if( even ) { // 24 data bits come in 2 words

        if( i < data.size() - 1 ) {

          ++npx;
          ++npxev;

          unsigned long raw = ( data[i] & 0xfff ) << 12;
          raw += data[i + 1] & 0xfff; // even + odd

          dec.Translate( raw );

          uint16_t ix = dec.GetX();
          uint16_t iy = dec.GetY();
          uint16_t ph = dec.GetPH();
          double vc = PHtoVcal( ph, 0, ix, iy );
          h11->Fill( ph );
          h12->Fill( vc );
          h13->Fill( vthr );
          h21->Fill( ix, iy );
          h22->Fill( ix, iy, vc );
          if( ix > 51 || iy > 79 || ph > 255 )
            ++nrr;
        }
        else {
          ++nrr;
        }

      } // even

      even = 1 - even;

      if( ( data[i] & 0x4000 ) == 0x4000 ) { // FPGA end marker
        h10->Fill( npxev );
      }

    } // data

    cout
      << "Vthr " << setw(3) << vthr
      << ", dur " << dt+dtr
      << ", size " << data.size()
      << ",  rest " << remaining
      << ",  events " << nev
      << ",  pix " << npx
      << ",  err " << nrr
      << endl;

    h21->Draw( "colz" );
    c1->Update();

  } // Vthr loop

  tb.Daq_Stop();
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
#endif
  tb.SetDAC( VthrComp, dacval[0][VthrComp] ); // restore
  tb.SetTimeout( 200000 ); // restore [ms] USB
  tb.Flush();

  h10->Write();
  h11->Write();
  h12->Write();
  h13->Write();
  h21->Write();
  h22->Write();
  cout << "  histos 10, 11, 12, 13, 21, 22" << endl;

  h13->Draw( "hist" );
  c1->Update();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tdscan

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
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

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
  h11->Sumw2();

  tb.roc_Col_Enable( col, true );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );
  tb.Pg_Loop( period );

  tb.SetTimeout( 2000 ); // [ms] USB
  tb.Flush();

  int nsum = 0;
  double phsum = 0;

  while( !keypressed() ) {

    tb.uDelay( 1000 ); // limit daq rate

    tb.Pg_Stop(); // stop triggers, necessary for clean data

    tb.uDelay( 100 );

    tb.Daq_Stop();
    tb.uDelay( 100 );

    tb.Daq_Read( data, Blocksize, remaining );

    ++ndq;
    got += data.size();
    rst += remaining;

    //tb.uDelay(1000);
    tb.Daq_Start();
    tb.uDelay( 100 );
    tb.Pg_Loop( period ); // start triggers
    tb.Flush();

    gettimeofday( &tv, NULL );
    long s9 = tv.tv_sec; // seconds since 1.1.1970
    long u9 = tv.tv_usec; // microseconds
    duration = s9 - s0 + ( u9 - u0 ) * 1e-6;

    if( duration - tprev > 0.999 ) {

      double phavg = 0;
      if( nsum > 0 )
        phavg = phsum / nsum;

      cout << duration << " s"
	   << ", last " << data.size()
	   << ",  rest " << remaining
	   << ",  calls " << ndq
	   << ",  events " << nev
	   << ",  got " << got
	   << ",  rest " << rst << ",  pix " << npx << ",  ph " << phavg << endl;

      h11->Draw( "hist" );
      c1->Update();

      nsum = 0;
      phsum = 0;

      tprev = duration;

    }

    // decode data:

    bool even = 1;

    for( unsigned int i = 0; i < data.size(); ++i ) {

      if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
        if( ldb && i > 0 )
          cout << endl;
        if( ldb )
          printf( "%X", data[i] );
        even = 0;
        ++nev;
      }
      else if( even ) { // 24 data bits come in 2 words

        if( i < data.size() - 1 ) {

          unsigned long raw = ( data[i] & 0xfff ) << 12;
          raw += data[i + 1] & 0xfff; // even + odd

          dec.Translate( raw );

          uint16_t ix = dec.GetX();
          uint16_t iy = dec.GetY();
          uint16_t ph = dec.GetPH();
          double vc = PHtoVcal( ph, 0, ix, iy );
          if( ldb )
            cout << " " << ix << "." << iy
		 << ":" << ph << "(" << ( int ) vc << ")";
          if( ix == col && iy == row ) {
            h11->Fill( duration, ph );
            ++nsum;
            phsum += ph;
          }
          else
            ++nrr;
          ++npx;
        }
        else {
          ++nrr;
        }

      } // even

      even = 1 - even;

    } // data

    if( ldb )
      cout << endl;

  } // while takedata

  tb.Daq_Stop();
  tb.Pg_Stop(); // stop triggers, necessary for clean re-start
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
#endif
  tb.SetTimeout( 200000 ); // restore [ms] USB

  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  h11->Write();
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  cout << endl;
  cout << "duration    " << duration << endl;
  cout << "triggers    " << nev << " = " << nev / duration << " Hz" << endl;
  cout << "DAQ calls   " << ndq << endl;
  cout << "words read  " << got << ", remaining " << rst << endl;
  cout << "data rate   " << 2 * got / duration << " bytes/s" << endl;
  cout << "pixels      " << npx << " = " << npx /
    ( double ) nev << "/ev " << endl;
  cout << "data errors " << nrr << endl;

  return true;

} // onevst

//------------------------------------------------------------------------------
CMD_PROC( modtd ) // module take data (trigger f = 40 MHz / period)
// period = 1 = external trigger = two streams
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
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // run number from file:

  unsigned int run = 1;
  ifstream irunFile( "runNumber.dat" );
  irunFile >> run;
  irunFile.close();

  ++run;

  ofstream orunFile( "runNumber.dat" );
  orunFile << run;
  orunFile.close();

  // output file:

  ostringstream fname; // output string stream

  fname << "run" << run << ".out";

  ofstream outFile( fname.str().c_str() );

  uint32_t remaining0 = 0;
  uint32_t remaining1 = 0;
  vector < uint16_t > data0;
  vector < uint16_t > data1;
  data0.reserve( Blocksize );
  data1.reserve( Blocksize );

  unsigned int nev[2] = { 0 };
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
  uint32_t PH[16][52][80] = { {{0}} };

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "pixels",
          "pixels per trigger;pixel hits;triggers", 65, -0.5, 64.5 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "ADC",
          "PH ADC spectrum;PH [ADC];hits", 255, -0.5, 255.5 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( "Vcal",
          "PH Vcal spectrum;PH [Vcal DAC];hits", 255, -0.5, 255.5 );
  h13->Sumw2();

  if( h21 )
    delete h21;
  //int n = 4; // fat pixels
  //int n = 2; // big pixels
  int n = 1; // pixels
  h21 = new TH2D( "ModuleHitMap",
                  "Module hit map;col;row;hits",
                  8 * 52 / n, -0.5 * n, 8 * 52 - 0.5 * n,
                  2 * 80 / n, -0.5 * n, 2 * 80 - 0.5 * n );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis()->SetTitleOffset( 1.3 );

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

  tb.Daq_Select_Deser400();
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  tb.Daq_Start( 0 );
  tb.Daq_Start( 1 );
  tb.uDelay( 1000 );

  tb.Flush();

  uint32_t trl = 0; // need to remember from previous daq_read
  int nsec = 0;

  while( !keypressed() ) {

    //tb.uDelay( 50000 ); // [us] limit daq rate, more efficient
    //tb.uDelay(  5000 ); // 2 kHz Sr 90 no Makrolon
    //tb.uDelay( 10000 ); // 4 kHz Sr 90 no Makrolon
    //tb.uDelay( 20000 ); // 5.1 kHz Sr 90 no Makrolon
    tb.uDelay( 40000 ); // 5.6 kHz Sr 90 no Makrolon

    tb.Pg_Stop(); // stop triggers, necessary for clean data

    tb.Daq_Stop( 0 );
    tb.Daq_Stop( 1 );

    tb.Daq_Read( data0, Blocksize, remaining0, 0 );
    tb.Daq_Read( data1, Blocksize, remaining1, 1 );

    ++ndq;
    got += data0.size();
    rst += remaining0;
    got += data1.size();
    rst += remaining1;

    tb.Daq_Deser400_Reset( 3 ); // more stable ?

    tb.Daq_Start( 0 );
    tb.Daq_Start( 1 );

    tb.Pg_Loop( period ); // start triggers
    tb.Flush();

    bool ldb = 0;
    //if( ndq == 1 ) ldb = 1;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    double dt = s2 - s0 + ( u2 - u0 ) * 1e-6;

    if( ldb || int ( dt ) > nsec ) {

      h21->Draw( "colz" );
      c1->Update();

      cout << s2 - s0 + ( u2 - u0 ) * 1e-6
	   << " s, last " << data0.size() + data1.size()
	   << ",  rest " << remaining0 + remaining1
	   << ",  calls " << ndq << ",  events " << nev[0]
	   << ",  got " << got
	   << ",  rest " << rst << ",  pix " << npx << ",  err " << nrr << endl;
      nsec = int ( dt );
    }

    // decode data:

    for( int ch = 0; ch < 2; ++ch ) {

      int size = data0.size();
      if( ch == 1 )
        size = data1.size();
      uint32_t raw = 0;
      uint32_t hdr = 0;

      int32_t iroc = nrocsa * ch - 1; // will start at 8
      int32_t kroc = enabledrocslist[0]; // will start at 0
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
          ++nth[ch];
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
          iroc = -1; // will start at 0
          if( ch == 1 )
            iroc = nrocsa - 1; // will start at 8
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
          ++iroc; // start at 0
          kroc = enabledrocslist[iroc];
          ++nrh[ch];
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
          ++npx;
          ++PX[kroc];
	  //DecodePixel(raw);
          ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
	  h12->Fill( ph );
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
	    ++npxev;
            NN[kroc][x][y] += 1;
            PH[kroc][x][y] += ph;
	    int l = kroc % 8; // 0..7
	    int xm = 52 * l + x; // 0..415  rocs 0 1 2 3 4 5 6 7
	    int ym = y; // 0..79
	    if( kroc > 7 ) {
	      xm = 415 - xm; // rocs 8 9 A B C D E F
	      ym = 159 - y; // 80..159
	    }
	    double vc = PHtoVcal( ph, kroc, x, y );
	    h13->Fill( vc );
	    h21->Fill( xm, ym );
	    h22->Fill( xm, ym, vc );
	    if( npxev == 1 ) outFile << nev[ch]; // non-empty event
	    outFile << " " << xm << " " << ym << " " << ph;
	  } // valid px
          break;

	  // TBM trailer:
        case 14:
          trl = d;
          break;
        case 12:
          trl = ( trl << 8 ) + d;
          if( ldb ) {
            cout << endl;
            cout << "TBM trailer";
            DecodeTbmTrailer( trl );
            cout << endl;
          }
          ++nev[ch];
          ++ntt[ch];
          h11->Fill( npxev );
	  if( npxev > 0 ) outFile << endl;
          break;

        default:
          printf( "\nunknown data: %X = %d", v, v );
          break;

        } // switch

      } // data

    } // ch

    if( ldb )
      cout << endl;

  } // while

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  double dt = s9 - s0 + ( u9 - u0 ) * 1e-6;

  tb.Pg_Stop(); // stop triggers, necessary for clean re-start
  //tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();

  outFile.close(); // redundant

  h11->Write();
  h12->Write();
  h13->Write();
  h21->Write();
  h22->Write();
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 11, 12, 13, 21, 22" << endl;

  cout << endl;
  for( size_t iroc = 0; iroc < enabledrocslist.size(); ++iroc ) {
    int roc = enabledrocslist[iroc];
    cout << "ROC " << setw( 2 ) << roc
	 << ", hits " << PX[roc] << endl;
  }
  cout << endl;
  cout << "run            " << run << endl;
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
  cout << "written to " << fname.str() << endl;

  return true;

} // modtd

//------------------------------------------------------------------------------
CMD_PROC( showclk )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  const unsigned int nSamples = 20;
  const int gain = 1;
  // PAR_INT( gain, 1, 4 );

  unsigned int i, k;
  vector < uint16_t > data[20];

  tb.Pg_Stop();
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
    tb.Daq_Start();
    tb.Pg_Single();
    tb.uDelay( 1000 );
    tb.Daq_Stop();
    tb.Daq_Read( data[i], 1024 );
    if( data[i].size() != nSamples ) {
      printf( "Data size %i: %i\n", i, int ( data[i].size() ) );
      return true;
    }
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif
  tb.Flush();

  int n = 20 * nSamples;
  vector < double > values( n );
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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  const unsigned int nSamples = 60;
  const int gain = 1;
  // PAR_INT( gain, 1, 4 );

  unsigned int i, k;
  vector < uint16_t > data[20];

  tb.Pg_Stop();
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
    tb.Daq_Start();
    tb.Pg_Single();
    tb.uDelay( 1000 );
    tb.Daq_Stop();
    tb.Daq_Read( data[i], 1024 );
    if( data[i].size() != nSamples ) {
      printf( "Data size %i: %i\n", i, int ( data[i].size() ) );
      return true;
    }
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif
  tb.Flush();

  int n = 20 * nSamples;
  vector < double > values( n );
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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
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
    tb.Daq_Start();
    tb.roc_Pix_Trim( 12, 34, 5 );
    tb.uDelay( 1000 );
    tb.Daq_Stop();
    tb.Daq_Read( data[i], 1024 );
    if( data[i].size() != nSamples ) {
      printf( "Data size %i: %i\n", i, int ( data[i].size() ) );
      return true;
    }
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif
  tb.Flush();

  int n = 20 * nSamples;
  vector < double > values( n );
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
  int size = data.size();
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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  unsigned short t;
  vector < uint16_t > data[16];
  tb.Pg_SetCmd( 0, PG_TOK + 0 );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 10 );
  Log.section( "decoding" );
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  for( t = 0; t < 44; ++t ) {
    tb.Sig_SetDelay( SIG_CLK, t );
    tb.Sig_SetDelay( SIG_TIN, t + 5 );
    tb.uDelay( 10 );
    for( int i = 0; i < 16; ++i ) {
      tb.Daq_Start();
      tb.Pg_Single();
      tb.uDelay( 200 );
      tb.Daq_Stop();
      tb.Daq_Read( data[i], 200 );
    }
    Log.printf( "%3i ", int ( t ) );
    decoding_show( data );
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif
  Log.flush();

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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.tbm_Enable( false );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tbmsel ) // normal: tbmsel 31 6
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
  //cout << "tbm_Set reg " << reg << " (" << int(reg) << ") with " << value << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tbmget )
{
  int reg;
  PAR_INT( reg, 0, 255 );

  cout << "tbm_Get reg " << reg << " (" << int(reg) << ")" << endl;
  unsigned char value;
  if( tb.tbm_Get( reg, value ) ) {
    printf( " reg 0x%02X = %3i (0x%02X)\n", reg, (int)value, (int)value);
  }
  else
    puts( " error\n" );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tbmgetraw )
{
  int reg;
  PAR_INT( reg, 0, 255 );

  uint32_t value;

  tb.Sig_SetLCDS(); // TBM sends RDA as LCDS, ROC sends token as LVDS

  for( int idel = 0; idel < 19; ++idel ) {
    tb.Sig_SetRdaToutDelay( idel ); // 0..19
    if( tb.tbm_GetRaw( reg, value ) ) {
      printf( "value = 0x%02X (Hub=%2i; Port=%i; Reg=0x%02X; inv=0x%X; stop=%c)\n",
	      value & 0xff, (value>>19) & 0x1f, (value>>16) & 0x07,
	      ( value>>8) & 0xff, (value>>25) & 0x1f, (value & 0x1000) ? '1' : '0' );
    }
    else
      puts( "error\n" );
  }

  return true;

} // tbmgetraw

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
      tb.SetDAC( addr, value ); // set corrected
      dacval[i][addr] = value;
      Log.printf( "[SETDAC] %i  %i\n", addr, value );
    }

  DO_FLUSH;
  return true;

} // dac

//------------------------------------------------------------------------------
CMD_PROC( vdig )
{
  int value;
  PAR_INT( value, 0, 15 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.SetDAC( Vdig, value );
      dacval[i][Vdig] = value;
      Log.printf( "[SETDAC] %i  %i\n", Vdig, value );
    }

  DO_FLUSH;
  return true;

} // vdig

//------------------------------------------------------------------------------
CMD_PROC( vana )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.SetDAC( Vana, value );
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
      tb.SetDAC( Vtrim, value );
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
      tb.SetDAC( VthrComp, value );
      dacval[i][VthrComp] = value;
      Log.printf( "[SETDAC] %i  %i\n", VthrComp, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( subvthr ) // subtract from VthrComp (make it harder)(or softer)
{
  int sub;
  PAR_INT( sub, -255, 255 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      int vthr = dacval[roc][VthrComp];
      cout << "ROC " << setw(2) << roc << " VthrComp from " << setw(3) << vthr;
      vthr -= sub;
      if( vthr < 0 )
	vthr = 0;
      else if( vthr > 255 )
	vthr = 255;
      tb.SetDAC( VthrComp, vthr );
      dacval[roc][VthrComp] = vthr;
      Log.printf( "[SETDAC] %i  %i\n", VthrComp, vthr );
      cout << " to " << setw(3) << vthr << endl;
    } // roc
  DO_FLUSH;
  return true;

} // subvthr

//------------------------------------------------------------------------------
CMD_PROC( vcal )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.SetDAC( Vcal, value );
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
      tb.SetDAC( CtrlReg, value );
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
      tb.SetDAC( WBC, value );
      dacval[i][WBC] = value;
      Log.printf( "[SETDAC] %i  %i\n", WBC, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
bool setia( int target )
{
  Log.section( "OPTIA", false );
  Log.printf( " Ia %i mA\n", target );
  cout << endl;
  cout << "  [OPTIA] target " << target << " mA" << endl;

  int val = dacval[0][Vana];

  if( val < 1 || val > 254 ) {
    val = 111;
    tb.SetDAC( Vana, val );
    dacval[0][Vana] = val;
    tb.mDelay( 200 );
  }

  double ia = tb.GetIA() * 1E3; // [mA]

  const double slope = 6; // 255 DACs / 40 mA
  const double eps = 0.5; // convergence, for DTB

  double tia = target + 0.5*eps; // besser zuviel als zuwenig

  double diff = ia - tia;

  int iter = 0;
  cout << "  " << iter << ". " << val << "  " << ia << "  " << diff << endl;

  while( fabs( diff ) > eps && iter < 11 && val > 0 && val < 255 ) {

    int stp = int ( fabs( slope * diff ) );
    if( stp == 0 )
      stp = 1;
    if( diff > 0 )
      stp = -stp;

    val += stp;
    if( val < 0 )
      val = 0;
    else if( val > 255 )
      val = 255;

    tb.SetDAC( Vana, val );
    dacval[0][Vana] = val;
    Log.printf( "[SETDAC] %i  %i\n", Vana, val );
    tb.mDelay( 200 );
    ia = tb.GetIA() * 1E3; // contains flush
    diff = ia - tia;
    ++iter;
    cout << "  " << iter << ". " << val << "  " << ia << "  " << diff << endl;
  }
  Log.flush();
  cout << "  set Vana to " << val << endl;

  if( fabs( diff ) < eps && iter < 11 && val > 0 && val < 255 )
    return true;
  else
    return false;
}

//------------------------------------------------------------------------------
CMD_PROC( optia ) // DP  optia ia [mA] for one ROC
{
  if( ierror ) return false;

  int target;
  PAR_INT( target, 5, 50 );

  return setia( target );

} // optia

//------------------------------------------------------------------------------
CMD_PROC( optiamod ) // CS  optiamod ia [mA] for a module
{
  if( ierror ) return false;
  int val[16];
  double iaroc[16];

  int target;
  PAR_INT( target, 10, 80 );

  // set all VA to zero:

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc ) {
    if( roclist[iroc] ) ++nrocs;
    val[iroc] = dacval[iroc][Vana];
    tb.roc_I2cAddr(iroc);
    tb.SetDAC( Vana, 0 );
  }
  tb.mDelay(300);
  // 1 roc optimized each time, reduce offset current by one
  double corr =  ( double( nrocs ) - 1.0) / double( nrocs );
  double IAoff = tb.GetIA() * 1000 * corr; // [mA]

  Log.section( "OPTIA", false );
  Log.printf( " Ia %i mA\n", target );

  const double slope = 6; // 255 DACs / 40 mA
  const double eps = 0.5; // DTB

  for( int iroc = 0; iroc < 16; ++iroc ) {

    if( !roclist[iroc] ) continue;
    tb.roc_I2cAddr( iroc );
    tb.SetDAC( Vana, val[iroc] );
    tb.mDelay( 300 );
    double ia = tb.GetIA() * 1E3 - IAoff; // [mA]
    double diff = target + 0.5*eps - ia; // besser zuviel als zuwenig
    int iter = 0;
    tb.roc_I2cAddr(iroc);
    //initial values
    cout << "ROC "<< iroc<<endl;
    cout << iter << ". " << val[iroc] << "  " << ia << "  " << diff << endl;
    //safe initial settings in case values are alrady goo
    dacval[iroc][Vana] = val[iroc];
    iaroc[iroc] = ia;

    while( fabs( diff ) > eps && iter < 11 &&
	   val[iroc] > 0 && val[iroc] < 255 ) {

      int stp = int ( fabs( slope * diff ) );
      if( stp == 0 )
	stp = 1;
      if( diff < 0 )
	stp = -stp;
      val[iroc] += stp;
      if( val[iroc] < 0 )
	val[iroc] = 0;
      else if( val[iroc] > 255 )
	val[iroc] = 255;
      tb.SetDAC( Vana, val[iroc] );
      tb.mDelay( 200 );
      ia = tb.GetIA() * 1E3 -  IAoff; // contains flush
      dacval[iroc][Vana] = val[iroc];
      Log.printf( "[SETDAC] %i  %i\n", Vana, val[iroc] );
      diff = target + 0.1 - ia;
      iaroc[iroc] = ia;
      ++iter;
      cout << iter << ". " << val[iroc] << "  " << ia << "  " << diff << endl;

    } // while

    Log.flush();
    tb.SetDAC( Vana, 0 );
    tb.mDelay( 200 );
    cout << "set Vana back to 0 for next ROC (save Vana = " << val[iroc]
	 << " ia " << iaroc[iroc]  <<  ") " << endl;

  } // rocs

  double sumia = 0;
  for( int iroc = 0; iroc < 16; ++iroc ) {
    if( !roclist[iroc] ) continue;
    tb.roc_I2cAddr(iroc);
    tb.SetDAC( Vana,  dacval[iroc][Vana]);
    tb.mDelay( 200 );
    cout << "ROC " << iroc << " set Vana to " << val[iroc]
	 << " ia " << iaroc[iroc]  << endl;
    sumia = sumia + iaroc[iroc];
  }
  cout<<"sum of all rocs " << sumia
      << " with average " << sumia / nrocs << " per roc"
      << endl;
  return true;

} // optiamod

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
	Log.printf( "%3i  %s %4i\n", j, dacName[j].c_str(),
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
bool writedac( string desc )
{
  //string globalName = gROOT->GetFile()->GetName(); 
  //int sizeName = globalName.size();
  //string prefixName = globalName.substr(0,sizeName-5);
  //cout << " Prefix to be used: " << prefixName.c_str() << endl;
  //string desc = prefixName;

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

  ostringstream fname; // output string stream

  if( nrocs == 1 )
    fname << "dacParameters_c" << Chip << "_" << desc.c_str() << ".dat";
  else
    fname << "dacParameters_D" << Module << "_" << desc.c_str() << ".dat";
  
  ofstream dacFile( fname.str().c_str() ); // love it!
  
  for( size_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      for( int idac = 1; idac < 256; ++idac )
        if( dacval[roc][idac] > -1 ) {
          dacFile << setw( 3 ) << idac
		  << "  " << dacName[idac] << "\t"
		  << setw( 5 ) << dacval[roc][idac] << endl;
        } // dac
    } // ROC
  cout << "DAC values written to " << fname.str() << endl;
  return true;

}

//------------------------------------------------------------------------------
CMD_PROC( wdac ) // write DACs to file
{
  char cdesc[80];
  PAR_STRING( cdesc, 80 );
  string desc = cdesc;
  writedac( cdesc );
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
      ++nrocs;

  ostringstream fname; // output string stream

  if( nrocs == 1 )
    fname << "dacParameters_c" << Chip << "_" << desc.c_str() << ".dat";
  else
    fname << "dacParameters_D" << Module << "_" << desc.c_str() << ".dat";

  ifstream dacFile( fname.str().c_str() );

  if( !dacFile ) {
    cout << "dac file " << fname.str() << " does not exist" << endl;
    return 0;
  }
  cout << "read dac values from " << fname.str() << endl;
  Log.printf( "[RDDAC] %s\n", fname.str().c_str() );

  int idac;
  string dacname;
  int vdac;
  int roc = -1;
  int croc = -1;

  while( !dacFile.eof() ) {
    dacFile >> idac >> dacname >> vdac;
    if( idac == 1 ) {
      ++croc;
      roc = enabledrocslist[croc];
      cout << "ROC " << roc << endl;
      tb.roc_I2cAddr( roc );
    }
    if( idac < 0 )
      cout << "illegal dac number " << idac;
    else if( idac > 255 )
      cout << "illegal dac number " << idac;
    else {
      tb.SetDAC( idac, vdac );
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
      ++nrocs;

  ostringstream fname; // output string stream

  if( nrocs == 1 )
    fname << "trimParameters_c" << Chip << "_" << desc.c_str() << ".dat";
  else
    fname << "trimParameters_D" << Module << "_" << desc.c_str() << ".dat";

  ofstream trimFile( fname.str().c_str() );

  for( size_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      for( int col = 0; col < 52; ++col )
        for( int row = 0; row < 80; ++row ) {
          trimFile << setw( 2 ) << modtrm[roc][col][row]
		   << "  Pix" << setw( 3 ) << col << setw( 3 ) << row << endl;
        }
    }
  cout << "trim values written to " << fname.str() << endl;
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
      ++nrocs;

  ostringstream fname; // output string stream

  if( nrocs == 1 )
    fname << "trimParameters_c" << Chip << "_" << desc.c_str() << ".dat";
  else
    fname << "trimParameters_D" << Module << "_" << desc.c_str() << ".dat";

  ifstream trimFile( fname.str().c_str() );

  if( !trimFile ) {
    cout << "trim file " << fname.str() << " does not exist" << endl;
    return 0;
  }
  cout << "read trim values from " << fname.str() << endl;
  Log.printf( "[RDTRIM] %s\n", fname.str().c_str() );

  string Pix; // dummy
  int icol;
  int irow;
  int itrm;
  vector < uint8_t > trimvalues( 4160 ); // 0..15
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
  if(      num == VDx )
    return 1800; // ROC looses settings at lower voltage
  else if( num == VAx )
    return  800; // [mV]
  else
    return 0;
}

//------------------------------------------------------------------------------
int32_t dacStop( int32_t num )  // max DAC value
{
  if(      num < 1 )
    return 0;
  else if( num > 255 )
    return 0;
  else if( num == Vdig || num == Vcomp || num == Vbias_sf )
    return 15; // 4-bit
  else if( num == CtrlReg )
    return 4;
  else if( num == VDx )
    return 3000;
  else if( num == VAx )
    return 2000;
  else
    return 255; // 8-bit
}

//------------------------------------------------------------------------------
int32_t dacStep( int32_t num )
{
  if(      num == VDx )
    return 10;
  else if( num == VAx )
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
  PAR_INT( roc, 0, 15 );
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
  PAR_INT( roc, 0, 15 );
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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_ClrCal();
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( mask )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  for( int i = 0; i < 16; ++i )
    if( roclist[i] ) {
      tb.roc_I2cAddr( i );
      tb.roc_Chip_Mask();
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
                dacName[dac].c_str(), dacName[dac].c_str() ),
	  nstp, dacstrt - 0.5, dacstop + 0.5 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "IA_DAC%02i", dac ),
          Form( "Analog current vs %s;%s [DAC];IA [mA]",
                dacName[dac].c_str(), dacName[dac].c_str() ),
	  nstp, dacstrt - 0.5, dacstop + 0.5 );
  h12->Sumw2();

  for( int32_t i = dacstrt; i <= dacstop; i += dacstep ) {

    tb.SetDAC( dac, i );
    tb.mDelay( 50 );

    double id = tb.GetID() * 1E3; // [mA]
    double ia = tb.GetIA() * 1E3;

    Log.printf( "%i %5.1f %5.1f\n", i, id, ia );
    printf( "%3i %5.1f %5.1f\n", i, id, ia );

    h11->Fill( i, id );
    h12->Fill( i, ia );
  }
  Log.flush();

  tb.SetDAC( dac, val ); // restore
  tb.Flush();

  h11->Write();
  h12->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 12" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vthrcompi ) // Id vs VthrComp: noise peak (amplitude depends on Temp?)
// peak position depends on Vana
{
  if( ierror ) return false;

  Log.section( "Id-vs-VthrComp", false );

  int roc = 0;
  PAR_INT( roc, 0, 15 );

  int32_t dacstop = dacStop( VthrComp );
  int32_t nstp = dacstop + 1;
  vector < double > yvec( nstp, 0.0 );
  double ymax = 0;
  int32_t imax = 0;

  Log.printf( " DAC %i: %i:%i on ROC %i\n", VthrComp, 0, dacstop, roc );

  //select the ROC:
  tb.roc_I2cAddr( roc );

  // all pix on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, true );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[roc][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.Flush();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "ID_VthrComp",
          "Digital current vs comparator threshold;VthrComp [DAC];ID [mA]",
          256, -0.5, 255.5 );
  h11->Sumw2();

  for( int32_t i = 0; i <= dacstop; ++i ) {

    tb.SetDAC( VthrComp, i );
    tb.mDelay( 20 );

    double id = tb.GetID() * 1000.0; // [mA]

    h11->Fill( i, id );

    Log.printf( "%3i %5.1f\n", i, id );
    if( roc == 0 ) printf( "%i %5.1f mA\n", i, id );

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
  tb.roc_ClrCal();
  tb.Flush();

  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

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

  int32_t val = maxi22 - 8; // safety for digV2.1
  val = maxi29 - 30; // safety for digV2
  if( val < 0 )
    val = 0;
  tb.SetDAC( VthrComp, val );
  tb.Flush();
  dacval[roc][VthrComp] = val;
  Log.printf( "[SETDAC] %i  %i\n", VthrComp, val );
  Log.flush();

  cout << "set VthrComp to " << val << endl;

  return true;
}

//------------------------------------------------------------------------------
// utility function for Pixel PH and cnt, ROC or mod
bool GetPixData( int roc, int col, int row, int nTrig,
                 int &nReadouts, double &PHavg, double &PHrms )
{
  bool ldb = 0;

  nReadouts = 0;
  PHavg = -1;
  PHrms = -1;

  tb.roc_I2cAddr( roc );
  tb.roc_Col_Enable( col, true );
  int trim = modtrm[roc][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

  vector < uint8_t > trimvalues( 4160 );
  for( int x = 0; x < 52; ++x ) {
    for( int y = 0; y < 80; ++y ) {
      int trim = modtrm[roc][x][y];
      int i = 80 * x + y;
      trimvalues[i] = trim;
    } // row y
  } // col x
  tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  int tbmch = roc / 8; // 0 or 1

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, tbmch );
#endif
  if( nrocs == 1 )
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  else {
    tb.Daq_Select_Deser400();
    tb.Daq_Deser400_Reset( 3 );
  }
  tb.uDelay( 100 );
  tb.Daq_Start( tbmch );
  tb.uDelay( 100 );
  tb.Flush();

  uint16_t flags = 0;
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  uint16_t mTrig = abs( nTrig );

  tb.LoopSingleRocOnePixelCalibrate( roc, col, row, mTrig, flags );

  tb.Daq_Stop( tbmch );

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize( tbmch ) );
  uint32_t rest;
  tb.Daq_Read( data, Blocksize, rest, tbmch );

#ifdef DAQOPENCLOSE
  tb.Daq_Close( tbmch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  // unpack data:

  int cnt = 0;
  int phsum = 0;
  int phsu2 = 0;

  if( nrocs == 1 ) {

    Decoder dec;
    bool even = 1;

    bool extra = 0;

    for( unsigned int i = 0; i < data.size(); ++i ) {

      if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
	even = 0;
      }
      else if( even ) { // merge 2 data words into one int:

	if( i < data.size() - 1 ) {

	  unsigned long raw = ( data[i] & 0xfff ) << 12;
	  raw += data[i + 1] & 0xfff;

	  dec.Translate( raw );

	  uint16_t ix = dec.GetX();
	  uint16_t iy = dec.GetY();
	  uint16_t ph = dec.GetPH();

	  if( ix == col && iy == row ) {
	    ++cnt;
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

  } // ROC

  else { // mod

    bool ldbm = 0;

    int event = 0;

    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t iroc = nrocsa * tbmch - 1; // will start at 8
    int32_t kroc = 0; // enabledrocslist[0];

    // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data.size(); ++i ) {

      int d = data[i] & 0xfff; // 12 bits data
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
        iroc = nrocsa * tbmch - 1; // will start at 8
        break;

	// ROC header data:
      case 4:
        ++iroc;
        kroc = enabledrocslist[iroc];
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
          ++cnt;
          phsum += ph;
	  phsu2 += ph * ph;
        }
        break;

	// TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
	//DecodeTbmTrailer(trl);
        if( ldbm )
          cout << endl;
        ++event;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    if( ldb )
      cout << "  events " << event
	   << " = " << event / mTrig << " dac values" << endl;

  } // mod

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
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );
  tb.Flush();

  for( int k = 0; k < abs( nTrig ); ++k ) {
    tb.Pg_Single();
    tb.uDelay( 20 );
  }
  tb.Flush();

  tb.Daq_Stop();
  cout << "  DAQ size " << tb.Daq_GetSize() << endl;

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );

  tb.Daq_Read( data, Blocksize );

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  // pixel off:

  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  cout << "  data size " << data.size();
  for( size_t i = 0; i < data.size(); ++i ) {
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

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "ph_distribution",
          Form( "ph col %i row %i;PH [ADC];triggers", col, row ),
          256, -0.5, 255.5 );
  h11->Sumw2();

  bool extra = 0;

  for( unsigned int i = 0; i < data.size(); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // ROC header
      even = 0;
      if( evt > 0 )
        cout << endl;
      ++evt;
      cout << "evt " << setw( 2 ) << evt;
    }
    else if( even ) { // merge 2 words into one pixel int:

      if( i < data.size() - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        dec.Translate( raw );

        uint16_t ix = dec.GetX();
        uint16_t iy = dec.GetY();
        uint16_t ph = dec.GetPH();
        double vc = PHtoVcal( ph, 0, ix, iy );

        cout << " pix " << setw( 2 ) << ix << setw( 3 ) << iy << setw( 4 ) << ph;

        if( ix == col && iy == row ) {
          ++cnt;
          phsum += ph;
          phsu2 += ph * ph;
          vcsum += vc;
	  h11->Fill( ph );
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

  h11->Write();
  h11->SetStats( 111111 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

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
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );

  for( int k = 0; k < nTrig; ++k ) {
    tb.Pg_Single();
    tb.uDelay( 200 );
  }

  tb.Daq_Stop();
  cout << "DAQ size " << tb.Daq_GetSize() << endl;

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );

  tb.Daq_Read( data, Blocksize );

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  cout << "data size " << data.size();
  for( size_t i = 0; i < data.size(); ++i ) {
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

  for( unsigned int i = 0; i < data.size(); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // ROC header
      even = 0;
      ++evt;
      pixels.clear();
      php[0] = -1;
      php[1] = -1;
    }
    else if( even ) { // merge 2 words into one pixel int:

      if( i < data.size() - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        CRocPixel pix;
        pix.raw = data[i] << 12;
        pix.raw += data[i + 1] & 0xfff;
        pix.DecodeRaw();
        pixels.push_back( pix );

        dec.Translate( raw );

        uint16_t ix = dec.GetX();
        uint16_t iy = dec.GetY();
        uint16_t ph = dec.GetPH();
        double vc = PHtoVcal( ph, 0, ix, iy );

        if( ix == col && iy == row ) {
          ++cnt[0];
          phsum[0] += ph;
          phsu2[0] += ph * ph;
          vcsum[0] += vc;
          php[0] = ph;
        }
        else if( ix == col && iy == row2 ) {
          ++cnt[1];
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
          ++n2;
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
  tb.roc_ClrCal();
  tb.Flush();

  return true;
}
//------------------------------------------------------------------------------
// utility function: single pixel dac scan, ROC or module
bool DacScanPix( const uint8_t roc, const uint8_t col, const uint8_t row,
                 const uint8_t dac, const uint8_t stp, const int16_t nTrig,
		 vector < int16_t > & nReadouts,
                 vector < double > & PHavg, vector < double > & PHrms )
{
  bool ldb = 0;

  tb.roc_I2cAddr( roc );
  tb.roc_Col_Enable( col, true );
  int trim = modtrm[roc][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

  vector < uint8_t > trimvalues( 4160 );
  for( int x = 0; x < 52; ++x ) {
    for( int y = 0; y < 80; ++y ) {
      int trim = modtrm[roc][x][y];
      int i = 80 * x + y;
      trimvalues[i] = trim;
    } // row y
  } // col x
  tb.SetTrimValues( roc, trimvalues ); // load into FPGA
  if( ldb ) cout << "[DacScanPix] loaded trim values to FPGA" << endl;

  int tbmch = roc / 8; // 0 or 1

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, tbmch );
#endif
  if( nrocs == 1 )
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  else {
    tb.Daq_Select_Deser400();
    tb.Daq_Deser400_Reset( 3 );
  }
  tb.uDelay( 100 );
  tb.Daq_Start( tbmch );
  tb.uDelay( 100 );
  tb.Flush();

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
      tb.LoopSingleRocOnePixelDacScan( roc, col, row, mTrig, flags,
				       dac, stp, dacstrt, dacstop );
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  // header = 1 word
  // pixel = +2 words
  // size = 256 * mTrig * (1 or 3) = 7680 for mTrig 10, all respond

  tb.Daq_Stop( tbmch ); // else ROC 0, 8 garbage (13.2.2015)

  if( ldb ) {
    cout << "  DAQ size " << tb.Daq_GetSize( tbmch ) << endl;
    cout << "  DAQ fill " << ( int ) tb.Daq_FillLevel( tbmch ) << endl;
    cout << ( done ? "done" : "not done" ) << endl;
  }

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize( tbmch ) );

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest, tbmch );
    if( ldb )
      cout << "  data size " << data.size()
	   << ", remaining " << rest << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest, tbmch );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      if( ldb )
	cout << "  data size " << data.size()
	     << ", remaining " << rest << endl;
      dataB.clear();
    }
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

#ifdef DAQOPENCLOSE
  tb.Daq_Close( tbmch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  tb.SetDAC( dac, vdac ); // restore dac value
  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  // unpack data:

  if( nrocs == 1 ) {

    int pos = 0;

    try {

      int32_t nstp = ( dacstop - dacstrt ) / stp + 1;

      if( ldb ) cout << "  expect " << nstp << " dac steps" << endl;

      int nmin = mTrig;
      int nmax = 0;

      for( int32_t j = 0; j < nstp; ++j ) { // DAC steps

        int cnt = 0;
        int phsum = 0;
        int phsu2 = 0;

        for( int k = 0; k < mTrig; ++k ) {

	  if( ldb ) cout << setw(5) << pos;
          int hdr;
          vector < PixelReadoutData > vpix = GetEvent( data, pos, hdr ); // analyzer
          for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
	    if( ldb ) cout << " " << hex << vpix[ipx].hdr
			   << dec << " " << vpix[ipx].n << endl;
            if( vpix[ipx].x == col && vpix[ipx].y == row ) {
              ++cnt;
              int ph = vpix[ipx].p;
              phsum += ph;
              phsu2 += ph * ph;
            }
	  }
        } // trig

        nReadouts.push_back( cnt );
        if( cnt > nmax )
          nmax = cnt;
        if( cnt < nmin )
          nmin = cnt;

        double ph = -1.0;
        double rms = 0.0;
        if( cnt > 0 ) {
          ph = ( double ) phsum / cnt;
          rms = sqrt( ( double ) phsu2 / cnt - ph * ph );
        }
        PHavg.push_back( ph );
        PHrms.push_back( rms );

      } // dacs

      if( ldb ) cout << "  final data pos " << pos << endl;
      if( ldb ) cout << "  nReadouts.size " << nReadouts.size() << endl;
      if( ldb ) cout << "  min max responses " << nmin << " " << nmax << endl;

      int thr = nmin + ( nmax - nmin ) / 2; // 50% level
      int thrup = dacstrt;
      int thrdn = dacstop;

      for( size_t i = 0; i < nReadouts.size() - 1; ++i ) {
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
	     << ", step below 50% at " << thrdn
	     << endl;

    } // try

    catch( int e ) {
      cout << "  Data error " << e << " at pos " << pos << endl;
      for( int i = pos - 8;
           i >= 0 && i < pos + 8 && i < int ( data.size() ); ++i )
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
    int phsu2 = 0;

    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t iroc = nrocsa * tbmch - 1; // will start at 8
    int32_t kroc = 0; // enabledrocslist[0];

    // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data.size(); ++i ) {

      int d = data[i] & 0xfff; // 12 bits data
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
        iroc = nrocsa * tbmch - 1; // will start at 8
        break;

	// ROC header data:
      case 4:
        ++iroc;
        kroc = enabledrocslist[iroc];
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
          ++cnt;
          phsum += ph;
	  phsu2 += ph * ph;
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
	  double ph = -1.0;
	  double rms = 0.0;
	  if( cnt > 0 ) {
	    ph = ( double ) phsum / cnt;
	    rms = sqrt( ( double ) phsu2 / cnt - ph * ph );
	  }
          PHavg.push_back( ph );
	  PHrms.push_back( rms );
          cnt = 0;
          phsum = 0;
          phsu2 = 0;
        }
        if( ldbm )
          cout << endl;
        ++event;
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

} // DacScanPix

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

  int nstp = nReadouts.size();

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
    else
      break; // beyond threshold 18.11.2014

  return thr;

} // ThrPix

//------------------------------------------------------------------------------
bool setcaldel( int col, int row, int nTrig )
{
  Log.section( "CALDEL", false );
  Log.printf( " pixel %i %i\n", col, row );
  cout << endl;
  cout << "  [CalDel] for pixel " << col << " " << row << endl;
  cout << "  CalDel   " << dacval[0][CalDel] << endl;
  cout << "  VthrComp " << dacval[0][VthrComp] << endl;
  cout << "  CtrlReg  " << dacval[0][CtrlReg] << endl;
  cout << "  Vcal     " << dacval[0][Vcal] << endl;

  uint8_t dac = CalDel;
  int val = dacval[0][CalDel];
  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( 0, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

  int nstp = nReadouts.size();
  cout << "  DAC steps " << nstp << ":" << endl;

  int wbc = dacval[0][WBC];

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "caldel_%02i_%02i_wbc%03i", col, row, wbc ),
          Form( "CalDel scan col %i row %i WBC %i;CalDel [DAC];responses",
                col, row, wbc ),
	  nstp, -0.5, nstp - 0.5 );
  h11->Sumw2();

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
  cout << "  eff plateau from " << i0 << " to " << i9 << endl;

  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  bool ok = 1;
  if( i9 > 0 ) {
    int i2 = i0 + ( i9 - i0 ) / 4;
    tb.SetDAC( CalDel, i2 );
    dacval[0][CalDel] = i2;
    printf( "  set CalDel to %i\n", i2 );
    Log.printf( "[SETDAC] %i  %i\n", CalDel, i2 );
  }
  else {
    tb.SetDAC( CalDel, val ); // back to default
    cout << "  leave CalDel at " << val << endl;
    ok = 0;
  }
  tb.Flush();

  Log.flush();

  return ok;

} // SetCalDel

//------------------------------------------------------------------------------
CMD_PROC( caldel ) // scan and set CalDel using one pixel
{
  if( ierror ) return false;

  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 100;

  return setcaldel( col, row, nTrig );

}

//------------------------------------------------------------------------------
CMD_PROC( caldelmap ) // map caldel left edge, 22 s (nTrig 10), 57 s (nTrig 99)
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  Log.section( "CALDELMAP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int vdac = dacval[0][CalDel];
  const int vctl = dacval[0][CtrlReg];
  const int vcal = dacval[0][Vcal];
  tb.SetDAC( CtrlReg, 4 );
  tb.SetDAC( Vcal, 255 ); // max pulse

  int nTrig = 16;
  //int start = dacval[0][CalDel] - 10;
  int step = 1;
  //int thrLevel = nTrig / 2;
  //bool xtalk = 0;
  //bool cals = 0;

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

      int thr = ThrPix( 0, col, row, Vcal, step, nTrig );
      /* before FW 4.4
      int thr = tb.PixelThreshold( col, row,
                                   start, step,
                                   thrLevel, nTrig,
                                   CalDel, xtalk, cals );
      // PixelThreshold ends with dclose
      */
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
      tb.roc_ClrCal();
      tb.Flush();

    } //rows

    Log.printf( "\n" );
    tb.roc_Col_Enable( col, 0 );
    tb.Flush();

  } // cols

  cout << endl;
  cout << "min thr" << setw( 4 ) << minthr << endl;
  cout << "max thr" << setw( 4 ) << maxthr << endl;

  tb.SetDAC( Vcal, vcal ); // restore dac value
  tb.SetDAC( CalDel, vdac ); // restore dac value
  tb.SetDAC( CtrlReg, vctl );

  tb.Flush();

  //double defaultLeftMargin = c1->GetLeftMargin();
  //double defaultRightMargin = c1->GetRightMargin();
  //c1->SetLeftMargin(0.10);
  //c1->SetRightMargin(0.18);
  h21->Write();
  h21->SetStats( 0 );
  h21->SetMinimum( minthr );
  h21->SetMaximum( maxthr );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 21" << endl;
  //c1->SetLeftMargin(defaultLeftMargin);
  //c1->SetRightMargin(defaultRightMargin);

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

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
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int mm[16];

  for( size_t iroc = 0; iroc < enabledrocslist.size(); ++iroc ) {

    int roc = enabledrocslist[iroc];
    if( roclist[roc] == 0 )
      continue;

    cout << endl << setw( 2 ) << "ROC " << roc << endl;

    // scan caldel:

    int nTrig = 10;
    uint8_t dac = CalDel;

    vector < int16_t > nReadouts; // size 0
    vector < double >PHavg;
    vector < double >PHrms;
    nReadouts.reserve( 256 ); // size 0, capacity 256
    PHavg.reserve( 256 );
    PHrms.reserve( 256 );

    DacScanPix( roc, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

    int nstp = nReadouts.size();
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
  for( size_t iroc = 0; iroc < enabledrocslist.size(); ++iroc ){
    int roc = enabledrocslist[iroc];
    cout << setw( 2 ) << roc << " CalDel " << setw( 3 ) << dacval[roc][CalDel]
	 << " plateau height " << mm[roc]
	 << endl;
  }
  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
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
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

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
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif
  tb.Daq_Select_Deser400();
  tb.uDelay( 100 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );
  tb.Daq_Start( 0 );
  tb.Daq_Start( 1 );

  uint16_t flags = 0; // or flags = FLAG_USE_CALS;
  //flags |= 0x0010; // FLAG_FORCE_MASK (needs trims)

  int dac = Vcal;
  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int nstp = dacstop - dacstrt + 1;

  gettimeofday( &tv, NULL );
  long s1 = tv.tv_sec; // seconds since 1.1.1970
  long u1 = tv.tv_usec; // microseconds

  try {
    tb.LoopMultiRocOnePixelDacScan( rocAddress, col, row, nTrig, flags, dac,
                                    dacstrt, dacstop );
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  int dSize[2];
  dSize[0] = tb.Daq_GetSize( 0 );
  dSize[1] = tb.Daq_GetSize( 1 );

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec; // seconds since 1.1.1970
  long u2 = tv.tv_usec; // microseconds
  double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;

  cout << "\tLoopMultiRocOnePixelDacScan takes " << dt << " s"
       << " = " << dt / 16 / nstp / nTrig * 1e6 << " us / pix" << endl;

  // read and unpack data:

  int cnt[16][256] = { {0} };

  // TBM channels:

  for( int tbmch = 0; tbmch < 2; ++tbmch ) {

    cout << "\tDAQ size channel " << tbmch
	 << " = " << dSize[tbmch] << " words " << endl;

    tb.Daq_Stop( tbmch );

    vector < uint16_t > data;
    data.reserve( dSize[tbmch] );

    try {
      uint32_t rest;
      tb.Daq_Read( data, Blocksize, rest, tbmch );
      cout << "\tdata size " << data.size()
	   << ", remaining " << rest << endl;
      while( rest > 0 ) {
        vector < uint16_t > dataB;
        dataB.reserve( Blocksize );
        tb.uDelay( 5000 );
        tb.Daq_Read( dataB, Blocksize, rest, tbmch );
        data.insert( data.end(), dataB.begin(), dataB.end() );
        cout << "\tdata size " << data.size()
	     << ", remaining " << rest << endl;
        dataB.clear();
      }
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    // decode:

    bool ldb = 0;

    uint32_t nev = 0;
    uint32_t nrh = 0; // ROC headers
    uint32_t npx = 0;
    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;

    int32_t iroc = nrocsa * tbmch - 1; // will start at 0 or 8
    int32_t kroc = 0;
    uint8_t idc = 0;

    // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data.size(); ++i ) {

      int d = data[i] & 0xfff; // 12 bits data
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
        iroc = nrocsa * tbmch - 1; // new event, will start at 0 or 8
        idc = nev / nTrig; // 0..255
        break;

	// ROC header data:
      case 4:
        ++iroc; // start at 0
        kroc = enabledrocslist[iroc];
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }
        ++nrh;
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
        ++npx;
        if( x == col && y == row && kroc >= 0 && kroc < 16 )
          ++cnt[kroc][idc];
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
        ++nev;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "\tTBM ch " << tbmch
	 << ": events " << nev
	 << " (expect " << nstp * nTrig << ")"
	 << ", ROC headers " << nrh
	 << " (expect " << nstp * nTrig * 8 << ")"
	 << ", pixels " << npx << endl;

  } // tbmch

  gettimeofday( &tv, NULL );
  long s3 = tv.tv_sec; // seconds since 1.1.1970
  long u3 = tv.tv_usec; // microseconds
  double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
  cout << "\tDaq_Read takes " << dtr << " s"
       << " = " << 2 * ( dSize[0] + dSize[1] ) / dtr / 1024 / 1024 << " MiB/s"
       << endl;

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << "\tROC " << setw( 2 ) << roc << ":" << endl;

    int nmx = 0;
    for( int idc = 0; idc < nstp; ++idc )
      if( cnt[roc][idc] > nmx )
	nmx = cnt[roc][idc];

    int i10 = 0;
    int i50 = 0;
    int i90 = 0;

    for( int idc = 0; idc < nstp; ++idc ) {

      int ni = cnt[roc][idc];
      cout << setw( 3 ) << ni;

      if( ni <= 0.1 * nmx )
	i10 = idc; // thr - 1.28155 * sigma
      if( ni <= 0.5 * nmx )
	i50 = idc; // thr
      if( ni <= 0.9 * nmx )
	i90 = idc; // thr + 1.28155 * sigma
      if( ni ==  nmx )
	break;
      //cout<< " idc:"<< idc << " i10: "<< i10<< " i50: "<< i50<< " i90: "<< i90<<endl;
    }
    cout << endl;
    cout << "thr " << i50 << ", rms " << (i90 - i10)/2.5631 << endl;
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
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modpixsc

//------------------------------------------------------------------------------
// utility function: loop over ROC 0 pixels, give triggers, count responses
// user: enable all

int GetEff( int &n01, int &n50, int &n99 )
{
  n01 = 0;
  n50 = 0;
  n99 = 0;

  uint16_t nTrig = 10;
  uint16_t flags = 0; // normal CAL

  //flags = 0x0002; // FLAG_USE_CALS;
  if( flags > 0 )
    cout << "CALS used..." << endl;

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  try {
    tb.LoopSingleRocAllPixelsCalibrate( 0, nTrig, flags );
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  // header = 1 word
  // pixel = +2 words
  // size = 4160 * nTrig * 3 = 124'800 words

  tb.Daq_Stop();

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest );
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      dataB.clear();
    }
  }
  catch( CRpcError & e ) {
    e.What();
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
            ++cnt;

      } // trig

      if( err )
        break;
      if( cnt > 0 )
        ++n01;
      if( cnt >= nTrig / 2 )
        ++n50;
      if( cnt == nTrig )
        ++n99;

    } // row

    if( err )
      break;

  } // col

  return 1;

} // GetEff

//------------------------------------------------------------------------------
CMD_PROC( dacscanmod ) // DAC scan for modules, all pix
// S-curve: dacscanmod 25 16 2 99 takes 124 s
// dacscanmod dac [[-]ntrig] [step] [stop]
{
  int dac;
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  int nTrig;
  PAR_INT( nTrig, -65000, 65000 );

  int step = 1;
  if( !PAR_IS_INT( step, 1, 255 ) )
    step = 1;

  int stop;
  if( !PAR_IS_INT( stop, 1, 255 ) )
    stop = dacStop( dac );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

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

    //tb.SetDAC( CtrlReg, 0 ); // small Vcal

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
  tb.Flush();

  cout << " = " << rocAddress.size() << " ROCs" << endl;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif
  tb.Daq_Select_Deser400();
  tb.uDelay( 100 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0;
  if( nTrig < 0 ) {
    flags = 0x0002; // FLAG_USE_CALS
    cout << "CALS used..." << endl;
  }
  // flags |= 0x0010; // FLAG_FORCE_MASK is FPGA default

  uint8_t dacstrt = dacStrt( dac );
  uint8_t dacstop = stop;

  int nstp = ( dacstop - dacstrt ) / step + 1;

  cout << "scan DAC " << int( dac )
       << " from " << int( dacstrt )
       << " to " << int( dacstop )
       << " in " << nstp << " steps of " << int( step )
       << endl;
  cout << "at CtrlReg " << dacval[0][CtrlReg] << endl;
  if( dac != 25 ) cout << "Vcal " << dacval[0][Vcal] << endl;

  Log.section( "DACSCANMOD", false );
  Log.printf( " DAC %i, step %i, nTrig %i\n", dac, step, nTrig );

  uint16_t mTrig = abs( nTrig );

  vector < uint16_t > data[2];
  data[0].reserve( nstp * mTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail
  data[1].reserve( nstp * mTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail

  bool done = 0;
  double dtloop = 0;
  double dtread = 0;
  int loop = 0;

  while( done == 0 ) { // FPGA sends blocks of data

    ++loop;
    cout << "\tloop " << loop << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    tb.Daq_Start( 0 );
    tb.Daq_Start( 1 );
    tb.uDelay( 100 );

    try {
      done =
        tb.LoopMultiRocAllPixelsDacScan( rocAddress, mTrig, flags,
                                         dac, step, dacstrt, dacstop );
      // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
      // loop cols
      //   loop rows
      //     activate this pix on all ROCs
      //     loop dacs
      //       loop trig
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    cout << ( done ? "\tdone" : "\tnot done" ) << endl;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    dtloop += s2 - s1 + ( u2 - u1 ) * 1e-6;

    int dSize[2];
    dSize[0] = tb.Daq_GetSize( 0 );
    dSize[1] = tb.Daq_GetSize( 1 );

    // read:

    for( int tbmch = 0; tbmch < 2; ++tbmch ) {

      cout << "\tDAQ size channel " << tbmch
	   << " = " << dSize[tbmch] << " words " << endl;

      tb.Daq_Stop( tbmch );

      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );

      try {
        uint32_t rest;
        tb.Daq_Read( dataB, Blocksize, rest, tbmch );
        data[tbmch].insert( data[tbmch].end(), dataB.begin(),
                            dataB.end() );
        cout << "\tdata[" << tbmch << "] size " << data[tbmch].size()
	  //<< ", remaining " << rest
	     << " of " << data[tbmch].capacity()
	     << endl;
        while( rest > 0 ) {
          dataB.clear();
          //tb.uDelay( 5000 );
          tb.Daq_Read( dataB, Blocksize, rest, tbmch );
          data[tbmch].insert( data[tbmch].end(), dataB.begin(),
                              dataB.end() );
          cout << "\tdata[" << tbmch << "] size " << data[tbmch].size()
	    //<< ", remaining " << rest
	       << " of " << data[tbmch].capacity()
	       << endl;
        }
      }
      catch( CRpcError & e ) {
        e.What();
        return 0;
      }

    } // TBM ch

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

  } // while FPGA not done

  cout << "\tLoopMultiRocAllPixelDacScan takes " << dtloop << " s" << endl;
  cout << "\tDaq_Read takes " << dtread << " s"
       << " = " << 2 * ( data[0].size() + data[1].size() ) /
    dtread / 1024 / 1024 << " MiB/s" << endl;

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
        for( int l = 0; l < nstp; ++l )
          cnt[i][j][k][l] = 0;

  int ****amp = new int ***[16];
  for( int i = 0; i < 16; ++i )
    amp[i] = new int **[52];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      amp[i][j] = new int *[80];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        amp[i][j][k] = new int[nstp];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        for( int l = 0; l < nstp; ++l )
          amp[i][j][k][l] = 0;

  bool ldb = 0;
  long countLoop = -1;
  for( int tbmch = 0; tbmch < 2; ++tbmch ) {
    countLoop = -1;
    cout << "\tDAQ size channel " << tbmch
	 << " = " << data[tbmch].size() << " words " << endl;

    uint32_t nev = 0;
    uint32_t nrh = 0; // ROC headers
    uint32_t npx = 0;
    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t iroc = nrocsa * tbmch -1; // will start at 0 or 8
    int32_t kroc = 0;//enabledrocslist[0];
    uint8_t idc = 0;

    // nDAC * nTrig * ( TBM header, 8 * ( ROC header, one pixel ), TBM trailer )

    for( size_t i = 0; i < data[tbmch].size(); ++i ) {

      int d = data[tbmch].at( i ) & 0xfff; // 12 bits data
      int v = ( data[tbmch].at( i ) >> 12 ) & 0xf; // 3 flag bits

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;
      bool addressmatch = true;
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
        iroc = nrocsa * tbmch - 1; // new event, will start at 0 or 8
        idc = ( nev / mTrig ) % nstp; // 0..nstp-1
	++countLoop;
        break;

	// ROC header data:
      case 4:
        ++iroc; // start at 0 or 8
        kroc = enabledrocslist[iroc];
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }
        ++nrh;
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
        ++npx;
	{
	  long xi = (countLoop / mTrig ) / nstp ;
	  int row = ( xi ) % 80;
	  int col = (xi - row) / 80;
	  
	  if( col != x or row != y ) {
	    addressmatch = false;
	    if( ldb ) {
	      cout << " pixel address mismatch " << endl;
	      cout << "from data " << " x " << x << " y " << y << " ph " << ph << endl;
	      cout << "from loop " << " col " << col << " row " << row << endl;
	    }
	  } // mismatch
	} // dummy scope

	if( addressmatch && kroc >= 0 && kroc < 16 ) {
          ++cnt[kroc][x][y][idc];
          amp[kroc][x][y][idc] += ph;
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
        ++nev;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "\tTBM ch " << tbmch
	 << ": events " << nev
	 << " (expect " << 4160 * nstp * mTrig << ")"
	 << ", ROC headers " << nrh
	 << " (expect " << 4160 * nstp * mTrig * 8 << ")"
	 << ", pixels " << npx << endl;

  } // tbmch

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "maxResponseDist",
          "maximum response distribution;maximum responses;pixels",
          mTrig + 1, -0.5, mTrig + 0.5 );
  h11->Sumw2();

  if( h14 )
    delete h14;
  h14 = new
    TH1D( "thrDist",
	  "Threshold distribution;threshold [small Vcal DAC];pixels",
	  255, -0.5, 254.5 );
  h14->Sumw2();

  if( h15 )
    delete h15;
  h15 = new
    TH1D( "noiseDist",
          "Noise distribution;width of threshold curve [small Vcal DAC];pixels",
          51, -0.5, 50.5 );
  h15->Sumw2();

  dacstop = dacstrt + (nstp-1)*step; // 255 or 254 (23.8.2014)
  int ctl = dacval[0][CtrlReg];
  int cal = dacval[0][Vcal];

  if( h21 )
    delete h21;
  if( nTrig < 0 ) // cals
    h21 = new TH2D( Form( "cals_PH_DAC%02i_map", dac ),
		    Form( "cals PH vs %s map;pixel;%s [DAC];cals PH [ADC]",
			  dacName[dac].c_str(),
			  dacName[dac].c_str() ),
		    16*4160, -0.5, 16*4160-0.5,
		    nstp, dacstrt - 0.5*step, dacstop + 0.5*step );
  else
    h21 = new TH2D( Form( "PH_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
		    Form( "PH vs %s CR %i Vcal %i map;pixel;%s [DAC];PH [ADC]",
			  dacName[dac].c_str(), ctl, cal,
			  dacName[dac].c_str() ),
		    16*4160, -0.5, 16*4160-0.5,
		    nstp, dacstrt - 0.5*step, dacstop + 0.5*step ); // 23.8.2014 * step

  if( h22 )
    delete h22;
  if( nTrig < 0 ) // cals
    h22 = new TH2D( Form( "cals_N_DAC%02i_map", dac ),
		    Form( "cals N vs %s map;pixel;%s [DAC];cals responses",
			  dacName[dac].c_str(),
			  dacName[dac].c_str() ),
		    16*4160, -0.5, 16*4160-0.5,
		    nstp, dacstrt - 0.5*step, dacstop + 0.5*step );
  else
    h22 = new TH2D( Form( "N_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
		    Form( "N vs %s CR %i Vcal %i map;pixel;%s [DAC];responses",
			  dacName[dac].c_str(), ctl, cal,
			  dacName[dac].c_str() ),
		    16*4160, -0.5, 16*4160-0.5,
		    nstp, dacstrt - 0.5*step, dacstop + 0.5*step ); // 23.8.2014 * step

  if( h23 )
    delete h23;
  h23 = new TH2D( "ModuleResponseMap",
                  "Module response map;col;row;responses",
                  8 * 52, -0.5, 8 * 52 - 0.5, 2 * 80, -0.5, 2 * 80 - 0.5 );
  h23->GetYaxis()->SetTitleOffset( 1.3 );

  if( h24 )
    delete h24;
  h24 = new TH2D( "ModuleThrMap",
                  "Module threshold map;col;row;threshold [small Vcal DAC]",
                  8 * 52, -0.5, 8 * 52 - 0.5, 2 * 80, -0.5, 2 * 80 - 0.5 );
  h24->GetYaxis()->SetTitleOffset( 1.3 );

  if( h25 )
    delete h25;
  h25 = new TH2D( "ModuleNoiseMap",
                  "Module noise map;col;row;width of threshold curve [small Vcal DAC]",
                  8 * 52, -0.5, 8 * 52 - 0.5, 2 * 80, -0.5, 2 * 80 - 0.5 );
  h25->GetYaxis()->SetTitleOffset( 1.3 );

  int iw = 3; // print format
  if( mTrig > 99 )
    iw = 4;

  // plot data:

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    if( ldb )
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
        if( col == 22 && row == -33 )
          ldb = 1; // one example pix

        for( int idc = 0; idc < nstp; ++idc ) {

          int ni = cnt[roc][col][row][idc];
          if( ldb )
            cout << setw( iw ) << ni;

	  uint32_t vdc = dacstrt + step*idc;

	  int ipx = 4160*roc + 80*col + row; // 0..66559

	  if( ni > 0 )
	    h21->Fill( ipx, vdc, amp[roc][col][row][idc] / ni );

	  h22->Fill( ipx, vdc, ni );

	  if( ni <= 0.1 * nmx )
	    i10 = vdc; // thr - 1.28155 * sigma
	  if( ni <= 0.5 * nmx )
	    i50 = vdc; // thr
	  if( ni <= 0.9 * nmx )
	    i90 = vdc; // thr + 1.28155 * sigma
	}

        if( ldb )
          cout << endl;
        if( ldb )
          cout << "thr " << i50
	       << ", rms " << (i90 - i10)/2.5631*step << endl;

        if( dac == 25 && nTrig > 0 && dacval[roc][CtrlReg] == 0 )
	  modthr[roc][col][row] = i50;

        h11->Fill( nmx );
        h14->Fill( i50 );
        h15->Fill( (i90 - i10)/2.5631*step );
        int l = roc % 8; // 0..7
        int m = roc / 8; // 0..1
        int xm = 52 * l + col; // 0..415  rocs 0 1 2 3 4 5 6 7
        if( m == 1 )
          xm = 415 - xm; // rocs 8 9 A B C D E F
        int ym = 159 * m + ( 1 - 2 * m ) * row; // 0..159
        h23->Fill( xm, ym, nmx );
        h24->Fill( xm, ym, i50 );
        h25->Fill( xm, ym, (i90 - i10)/2.5631*step );

      } // row

    } // col

  } // roc

   if( nTrig < 0 && dac == 12 ) { // BB test for module

    if( h12 )
      delete h12;
    h12 = new TH1D( "CalsVthrPlateauWidth",
		    "Width of VthrComp plateau for cals;width of VthrComp plateau for cals [DAC];pixels",
		    151, -0.5, 150.5 );
    h12->Sumw2();

    if( h24 )
      delete h24;
    h24 = new TH2D( "BBtestMap",
		    "BBtest map;col;row;cals VthrComp response plateau width",
		    8*52, -0.5, 8*52 - 0.5, 2*80, -0.5, 2*80 - 0.5 );

    if( h25 )
      delete h25;
    h25 = new TH2D( "BBqualityMap",
		    "BBtest quality map;col;row;dead good missing",
		    8*52, -0.5, 8*52 - 0.5, 2*80, -0.5, 2*80 - 0.5 );

    // Localize the missing Bump from h22

    int nbinx = h22->GetNbinsX(); // pix
    int nbiny = h22->GetNbinsY(); // dac

    int nActive;
    int nMissing;
    int nIneff;

    nActive = 0;
    nMissing = 0;
    nIneff = 0;

    int ipx;

    // pixels:

    // get width of Vthrcomp plateu distribution and fit it
    for( int ibin = 1; ibin <= nbinx; ++ibin ) {

      ipx = h22->GetXaxis()->GetBinCenter( ibin );
      int roc = ipx / ( 80 * 52 );
      ipx = ipx - roc * ( 80 * 52);
      // max response:

      int imax;
      imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = h22->GetBinContent( ibin, j );
	if( cnt > imax )
	  imax = cnt;
      }

      if( imax < mTrig / 2 ) {
	++nIneff;
      }
      else {
 
	// Search for the Plateau

	int iEnd = 0;
	int iBegin = 0;

	// search for DAC response plateau:

	for( int jbin = 0; jbin <= nbiny; ++jbin ) {

	  int idac = h22->GetYaxis()->GetBinCenter( jbin );

	  int cnt = h22->GetBinContent( ibin, jbin );

	  if( cnt >= imax / 2 ) {
	    iEnd = idac; // end of plateau
	    if( iBegin == 0 )
	      iBegin = idac; // begin of plateau
	  }
	}

	// cout << "Bin: " << ibin << " Plateau Begins and End in  " << iBegin << " - " << iEnd << endl;
	// narrow plateau is from noise

	h12->Fill( iEnd - iBegin );

      }
    }
    TF1 *gausFit = new TF1("gausFit","gaus", 30, 70);
    h12->Fit(gausFit,"qr");
    float cutval = gausFit->GetParameter(1) - 5*gausFit->GetParameter(2);

    cout << "cutoff for bad bumps (5 sigma away from mean of good bumps): " << cutval << endl;
    Log.printf( "cutoff for bad bumps (5 sigma away from mean of good bumps): %f", cutval );

    for( int ibin = 1; ibin <= nbinx; ++ibin ) {

      ipx = h22->GetXaxis()->GetBinCenter( ibin );
      int roc = ipx / ( 80 * 52 );
      ipx = ipx - roc * ( 80 * 52);
      // max response:

      int imax;
      imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = h22->GetBinContent( ibin, j );
	if( cnt > imax )
	  imax = cnt;
      }

      if( imax < mTrig / 2 ) {
	++nIneff;
	cout << "Dead pixel at roc row col: " << roc << " " << ipx % 80
	     << " " << ipx / 80 << endl;
      }
      else {
 
	// Search for the Plateau

	int iEnd = 0;
	int iBegin = 0;

	// search for DAC response plateau:

	for( int jbin = 0; jbin <= nbiny; ++jbin ) {

	  int idac = h22->GetYaxis()->GetBinCenter( jbin );

	  int cnt = h22->GetBinContent( ibin, jbin );

	  if( cnt >= imax / 2 ) {
	    iEnd = idac; // end of plateau
	    if( iBegin == 0 )
	      iBegin = idac; // begin of plateau
	  }
	}

	// cout << "Bin: " << ibin << " Plateau Begins and End in  " << iBegin << " - " << iEnd << endl;
	// narrow plateau is from noise

	int row = ipx % 80;
	int col = ipx / 80;
	int l = roc % 8; // 0..7
	int m = roc / 8; // 0..1
	int xm = 52 * l + col; // 0..415 rocs 0 1 2 3 4 5 6 7
	if( m == 1 )
	  xm = 415 - xm; // rocs 8 9 A B C D E F
	int ym = 159 * m + ( 1 - 2 * m ) * row; // 0..159

	h24->Fill(  xm, ym, iEnd - iBegin );

	if( iEnd - iBegin < cutval ) {

	  ++nMissing;
	  cout << "[Missing Bump at roc row col:] " << roc << " " << ipx % 80
	       << " " << ipx / 80 << endl;
	  Log.printf( "[Missing Bump at roc row col:] %i %i %i\n", roc , ipx % 80, ipx / 80 );
	  h25->Fill( xm, ym, 1 ); // magenta
	}
	else {
	  ++nActive;
	  h25->Fill( xm, ym, 2 ); // green
	} // plateau width

      } // active imax

    } // x-bins

    h12->Write();

    // save the map in the log file

    for( int ibin = 1; ibin <= h24->GetNbinsX(); ++ibin ) {
      for( int jbin = 1; jbin <= h24->GetNbinsY(); ++jbin ) {
	int c_val = h24->GetBinContent( ibin, jbin );
	Log.printf( " %i", c_val );
	//cout << ibin << " " << jbin << " " << c_val << endl;
      } //row
      Log.printf( "\n" );
    } // col
    Log.flush();

    Log.printf( "Number of Active bump bonds [above trig/2]: %i\n", nActive );
    Log.printf( "Number of Missing bump bonds: %i\n", nMissing );
    Log.printf( "Number of Dead pixel: %i\n", nIneff );

    cout << "Number of Active bump bonds: " << nActive << endl;
    cout << "Number of Missing bump bonds: " << nMissing << endl;
    cout << "Number of Dead pixel: " << nIneff << endl;

    h24->Write();

    h25->SetStats( 0 );
    h25->SetMinimum(0);
    h25->SetMaximum(2);
    h25->Draw( "colz" );
    c1->Update();
    cout << "  histos 11, 12, 21, 22, 23, 24, 25" << endl;

  } // BB test

  // De-allocate memory to prevent memory leak

  for( int i = 0; i < 16; ++i ) {
    for( int j = 0; j < 52; ++j ) {
      for( int k = 0; k < 80; ++k )
        delete[]cnt[i][j][k];
      delete[]cnt[i][j];
    }
    delete[]cnt[i];
  }
  delete[]cnt;

  for( int i = 0; i < 16; ++i ) {
    for( int j = 0; j < 52; ++j ) {
      for( int k = 0; k < 80; ++k )
        delete[]amp[i][j][k];
      delete[]amp[i][j];
    }
    delete[]amp[i];
  }
  delete[]amp;

  h11->Write();
  h14->Write();
  h15->Write();
  h21->Write();
  h22->Write();
  h23->Write();
  h24->Write();
  h25->Write();
  h23->Draw( "colz" );
  c1->Update();
  cout << "  histos 11, 14, 15, 21 big, 22 big, 23, 24, 25" << endl;

  // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    tb.SetDAC( Vcal, dacval[roc][dac] ); // restore
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // dacscanmod

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
        ++nok;
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

  Log.flush();

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

} // printThrMap

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
  tb.Flush();

  cout << " = " << rocAddress.size() << " ROCs" << endl;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif
  tb.Daq_Select_Deser400();
  tb.uDelay( 100 );
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
  int loop = 0;

  while( done == 0 ) {

    ++loop;
    cout << "\tloop " << loop << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    tb.Daq_Start( 0 );
    tb.Daq_Start( 1 );
    tb.uDelay( 100 );

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
      e.What();
      return;
    }

    cout << ( done ? "\tdone" : "\tnot done" ) << endl;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    dtloop += s2 - s1 + ( u2 - u1 ) * 1e-6;

    int dSize[2];
    dSize[0] = tb.Daq_GetSize( 0 );
    dSize[1] = tb.Daq_GetSize( 1 );

    // read:

    for( int tbmch = 0; tbmch < 2; ++tbmch ) {

      cout << "\tDAQ size channel " << tbmch
	   << " = " << dSize[tbmch] << " words " << endl;

      //cout << "\tDAQ_Stop..." << endl;
      tb.Daq_Stop( tbmch );

      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );

      try {
        uint32_t rest;
        tb.Daq_Read( dataB, Blocksize, rest, tbmch );
        data[tbmch].insert( data[tbmch].end(), dataB.begin(),
                            dataB.end() );
        cout << "\tdata[" << tbmch << "] size " << data[tbmch].size()
	  //<< ", remaining " << rest
	     << " of " << data[tbmch].capacity()
	     << endl;
        while( rest > 0 ) {
          dataB.clear();
          tb.uDelay( 5000 );
          tb.Daq_Read( dataB, Blocksize, rest, tbmch );
          data[tbmch].insert( data[tbmch].end(), dataB.begin(),
                              dataB.end() );
          cout << "\tdata[" << tbmch << "] size " << data[tbmch].size()
	    //<< ", remaining " << rest
	       << " of " << data[tbmch].capacity()
	       << endl;
        }
      }
      catch( CRpcError & e ) {
        e.What();
        return;
      }

    } // TBM ch

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

  } // while not done

  cout << "\tLoopMultiRocAllPixelDacScan takes " << dtloop << " s" << endl;
  cout << "\tDaq_Read takes " << dtread << " s"
       << " = " << 2 * ( data[0].size() +
			 data[1].size() ) / dtread / 1024 /
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
        for( int l = 0; l < nstp; ++l )
          cnt[i][j][k][l] = 0;

  bool ldb = 0;
  long countLoop = -1; 
  for( int tbmch = 0; tbmch < 2; ++tbmch ) {
    countLoop = -1; 
    cout << "\tDAQ size channel " << tbmch
	 << " = " << data[tbmch].size() << " words " << endl;

    uint32_t nev = 0;
    uint32_t nrh = 0; // ROC headers
    uint32_t npx = 0;
    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t iroc = nrocsa * tbmch - 1; // will start at 0 or 8
    int32_t kroc = 0;//enabledrocslist[0];
    uint8_t idc = 0; // 0..255

    // nDAC * nTrig * ( TBM header, 8 * ( ROC header, one pixel ), TBM trailer )

    for( size_t i = 0; i < data[tbmch].size(); ++i ) {

      int d = data[tbmch].at( i ) & 0xfff; // 12 bits data
      int v = ( data[tbmch].at( i ) >> 12 ) & 0xf; // 3 flag bits

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;
      bool addressmatch = true;
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
        iroc = nrocsa * tbmch - 1; 
	++countLoop;
        idc = ( nev / nTrig ) % nstp; // 0..nstp-1
        break;

	// ROC header data:
      case 4:
        ++iroc;
        kroc = enabledrocslist[iroc]; // start at 0 or 8
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }
        ++nrh;
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
        ++npx;
	{
	  long xi = (countLoop / nTrig ) / nstp ;
	  int row = ( xi ) % 80;
	  int col = (xi - row) / 80;	
	  
	  if( col != x or row != y ) {
	    addressmatch = false;
	    if( ldb ) {
	      cout << " pixel address mismatch " << endl;
	      cout << "from data " << " x " << x << " y " << y << " ph " << ph << endl;
	      cout << "from loop " << " col " << col << " row " << row << endl;
	    }
	  } // mismatch
	} // dummy scope

	if( addressmatch && kroc >= 0 && kroc < 16 )
          ++cnt[kroc][x][y][idc];
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
        ++nev;
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
        if( lex && col == 35 && row == 16 )
          ldb = 1; // one example pix

        for( int idc = 0; idc < nstp; ++idc ) {

          int ni = cnt[roc][col][row][idc];

          if( ldb )
            cout << setw( iw ) << ni;
	  if( ni <= 0.1 * nmx )
	    i10 = idc; // thr - 1.28155 * sigma
	  if( ni <= 0.5 * nmx )
	    i50 = idc; // thr
	  if( ni <= 0.9 * nmx )
	    i90 = idc; // thr + 1.28155 * sigma
	  if( ni == nmx )
            break;
        }

        if( ldb )
          cout << endl;
        if( ldb )
          cout << "\tthr " << i50 * step
	       << ", rms " << ( (i90 - i10)/2.5631 ) * step << endl;

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
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

} // ModThrMap

//------------------------------------------------------------------------------
CMD_PROC( modthrmap ) // modthrmap [new] [cut]
{
  Log.section( "MODTHRMAP", true );

  int New;
  if( !PAR_IS_INT( New, 0, 1 ) )
    New = 1; // measure new map

  int cut;
  if( !PAR_IS_INT( cut, 0, 255 ) )
    cut = 20; // for printout

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int strt = 0;
  int stop = 127; // Vcal scan range, 255 = overflow
  int step = 1; // fine
  int nstp = ( stop - strt ) / step + 1;

  const int nTrig = 10;
  const int xtlk = 0;
  const int cals = 0;

  cout << "measuring Vcal threshold map in range "
       << strt << " to " << stop
       << " in steps of " << step << " with " << nTrig << " triggers" << endl;

  if( New )
    ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // fills modthr

  if( h11 )
    delete h11;
  gStyle->SetOptStat( 1110 );
  h11 = new
    TH1D( Form( "mod_thr_dist" ),
	  Form( "Module threshold distribution;threshold [small Vcal DAC];pixels" ),
	  nstp, strt-0.5, stop+0.5 );
  h11->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleThresholdMap",
                  "Module threshold map;col;row;threshold [small Vcal DAC]",
                  8*52, -0.5, 8*52 - 0.5, 2*80, -0.5, 2*80 - 0.5 );
  h21->GetYaxis()->SetTitleOffset( 1.3 );

  int sum = 0;
  int nok = 0;
  int su2 = 0;
  int vmin = 255;
  int vmax = 0;
  int colmin = -1;
  int rowmin = -1;
  int rocmin = -1;
  int colmax = -1;
  int rowmax = -1;
  int rocmax = -1;

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int l = roc % 8; // 0..7
	int xm = 52 * l + col; // 0..415  rocs 0 1 2 3 4 5 6 7
	int ym = row; // 0..79
	if( roc > 7 ) {
	  xm = 415 - xm; // rocs 8 9 A B C D E F
	  ym = 159 - row; // 80..159
	}
	int thr = modthr[roc][col][row];
	h11->Fill( thr );
	h21->Fill( xm, ym, thr );
	if( thr > 0 && thr < 255 ) {
	  sum += thr;
	  su2 += thr * thr;
	  ++nok;
	}
	else
	  cout << "thr " << setw( 3 ) << thr
	       << " for ROC col row trim "
	       << setw( 2 ) << roc
	       << setw( 3 ) << col
	       << setw( 3 ) << row
	       << setw( 3 ) << modtrm[roc][col][row]
	       << endl;

	if( thr < cut )
	  cout << "thr " << setw( 3 ) << thr
	       << " for ROC col row trim "
	       << setw( 2 ) << roc
	       << setw( 3 ) << col
	       << setw( 3 ) << row
	       << setw( 3 ) << modtrm[roc][col][row]
	       << endl;

	if( thr > 0 && thr < vmin ) {
	  vmin = thr;
	  colmin = col;
	  rowmin = row;
	  rocmin = roc;
	}

	if( thr > vmax && thr < 255 ) {
	  vmax = thr;
	  colmax = col;
	  rowmax = row;
	  rocmax = roc;
	}

      } // rows

  } // rocs

  cout << "  valid thresholds " << nok << endl;
  if( nok > 0 ) {
    cout << "  min thr " << vmin
	 << " at ROC" << setw( 2 ) << rocmin
	 << " col " << setw( 2 ) << colmin
	 << " row " << setw( 2 ) << rowmin
	 << endl;
    cout << "  max thr " << vmax
	 << " at ROC" << setw( 2 ) << rocmax
	 << " col " << setw( 2 ) << colmax
	 << " row " << setw( 2 ) << rowmax
	 << endl;
    double mid = ( double ) sum / ( double ) nok;
    double rms = sqrt( ( double ) su2 / ( double ) nok - mid * mid );
    cout << "  mean thr " << mid << endl;
    cout << "  thr rms  " << rms << endl;
  }

  h11->Write();
  h21->Write();
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 21" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modthrmap

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 8.7.2014: set global threshold per ROC
CMD_PROC( modvthrcomp )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "MODVTHRCOMP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // take threshold map, find softest pixel

  int strt = 0;
  int stop = 255; // Vcal scan range
  int step = 4; // coarse

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
	if( thr < 11 ) continue; // ignore
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

    int vstp = -1; // towards harder threshold
    if( vmin > target )
      vstp = 1; // towards softer threshold

    int vthr = dacval[roc][VthrComp];

    for( ; vthr < 255 && vthr > 0; vthr += vstp ) {

      tb.SetDAC( VthrComp, vthr );
      tb.uDelay( 100 );
      tb.Flush();

      int thr = ThrPix( roc, colmin, rowmin, dac, step, nTrig );

      cout << "VthrComp " << setw( 3 ) << vthr << " thr " << setw( 3 ) << thr
	   << endl;
      if( vstp * thr <= vstp * target or thr > 240 ) // signed
        break;

    } // vthr

    vthr -= 1; // safety margin towards harder threshold
    tb.SetDAC( VthrComp, vthr );
    cout << "set VthrComp to " << vthr << endl;
    dacval[roc][VthrComp] = vthr;
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    tb.Flush();

  } // rocs

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << "ROC " << setw( 2 ) << roc
	 << " VthrComp " << setw( 3 ) << dacval[roc][VthrComp]
	 << endl;
  }

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modvthrcomp

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

} // ModTrimStep

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 8.7.2014: trim module
CMD_PROC( modtrim )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "TRIM", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

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
        int trim = 15; // no trim
        modtrm[roc][col][row] = trim;
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  } // rocs

  tb.uDelay( 1000 );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // take threshold map, find hardest pixel

  bool contTrim = true;
  int strt = 0;
  int stop = 255; // Vcal scan range
  int step = 4; // coarse

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

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "mod_thr_dist_bits_15",
	  "Module threshold distribution bits 15;threshold [small Vcal DAC];pixels",
	  64, -2, 254 ); // 255 = overflow
  h11->Sumw2();

  for( size_t roc = 0; roc < 16; ++roc )
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
	h11->Fill( modthr[roc][col][row] );

  h11->Draw( "hist" );
  c1->Update();
  h11->Write();

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
        if( thr < vmin && thr > 0 )
          vmin = thr;
      } // cols

    if( vmin < target ) {
      cout << "min threshold below target on ROC " << roc
	   << ": skipped (try lower VthrComp) will not trim module" << endl;
      contTrim = false;
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
      tb.uDelay( 120 );
      tb.Flush();

      int thr = ThrPix( roc, colmax, rowmax, dac, step, nTrig );

      cout << setw( 3 ) << itrim << "  " << setw( 3 ) << thr << endl;
      if( thr < target or thr > 254 ) // CS
	break; // done

    } // itrim

    itrim += 2; // margin
    // CS safety if something failed go to default value:
    if( itrim < 10 ) itrim = 180; 
    tb.SetDAC( Vtrim, itrim );
    cout << "set Vtrim to " << itrim << endl;
    dacval[roc][Vtrim] = itrim;

    tb.roc_Pix_Mask( colmax, rowmax );
    tb.roc_Col_Enable( colmax, 0 );
    tb.roc_ClrCal();
    tb.Flush();

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

  if( contTrim ) {

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

    if( h12 )
      delete h12;
    h12 = new
      TH1D( "mod_thr_dist_bits_7",
	    "Module threshold distribution bits 7;threshold [small Vcal DAC];pixels",
	    64, -2, 254 ); // 255 = overflow
    h12->Sumw2();

    for( size_t roc = 0; roc < 16; ++roc )
      for( int col = 0; col < 52; ++col )
	for( int row = 0; row < 80; ++row )
	  h12->Fill( modthr[roc][col][row] );

    h12->Draw( "hist" );
    c1->Update();
    h12->Write();

    int correction = 4;
    step = 4;

    ModTrimStep( target, correction, step, nTrig, xtlk, cals ); // fills modtrm, fills modthr

    //printThrMap( 0, roc, nok );

    if( h13 )
      delete h13;
    h13 = new
      TH1D( "mod_thr_dist_bits_7_4",
	    "Module threshold distribution bits 7 4;threshold [small Vcal DAC];pixels",
	    64, -2, 254 ); // 255 = overflow
    h13->Sumw2();

    for( size_t roc = 0; roc < 16; ++roc )
      for( int col = 0; col < 52; ++col )
	for( int row = 0; row < 80; ++row )
	  h13->Fill( modthr[roc][col][row] );

    h13->Draw( "hist" );
    c1->Update();
    h13->Write();

    correction = 2;
    step = 2;

    ModTrimStep( target, correction, step, nTrig, xtlk, cals ); // fills modtrm, fills modthr

    //printThrMap( 0, roc, nok );

    if( h14 )
      delete h14;
    h14 = new
      TH1D( "mod_thr_dist_bits_7_4_2",
	    "Module threshold distribution bits 7 4 2;threshold [small Vcal DAC];pixels",
	    127, -1, 253 ); // 255 = overflow
    h14->Sumw2();

    for( size_t roc = 0; roc < 16; ++roc )
      for( int col = 0; col < 52; ++col )
	for( int row = 0; row < 80; ++row )
	  h14->Fill( modthr[roc][col][row] );

    h14->Draw( "hist" );
    c1->Update();
    h14->Write();

    correction = 1;
    step = 1;

    ModTrimStep( target, correction, step, nTrig, xtlk, cals ); // fills modtrm, fills modthr

    //printThrMap( 0, roc, nok );

    if( h15 )
      delete h15;
    h15 = new
      TH1D( "mod_thr_dist_bits_7_4_2_1",
	    "Module threshold distribution bits 7 4 2 1;threshold [small Vcal DAC];pixels",
	    255, -0.5, 254.5 ); // 255 = overflow
    h15->Sumw2();

    for( size_t roc = 0; roc < 16; ++roc )
      for( int col = 0; col < 52; ++col )
	for( int row = 0; row < 80; ++row )
	  h15->Fill( modthr[roc][col][row] );

    h15->Draw( "hist" );
    c1->Update();
    h15->Write();

    correction = 1;
    step = 1;

    ModTrimStep( target, correction, step, nTrig, xtlk, cals ); // fills modtrm, fills modthr

    //printThrMap( 0, roc, nok );

    if( h16 )
      delete h16;
    h16 = new
      TH1D( "mod_thr_dist_bits_7_4_2_1_1",
	    "Module threshold distribution bits 7 4 2 1 1;threshold [small Vcal DAC];pixels",
	    255, -0.5, 254.5 ); // 255 = overflow
    h16->Sumw2();

    for( size_t roc = 0; roc < 16; ++roc )
      for( int col = 0; col < 52; ++col )
	for( int row = 0; row < 80; ++row )
	  h16->Fill( modthr[roc][col][row] );

    h16->Draw( "hist" );
    c1->Update();
    h16->Write();

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

  } // contTrim

  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modtrim

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 15.2.2015, adjust trimbits of weak pixels
CMD_PROC( modtrimbits )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  // analyze latest mod thr map:

  int sum = 0;
  int nok = 0;
  int su2 = 0;

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int thr = modthr[roc][col][row];
	if( thr > 0 && thr < 255 ) {
	  sum += thr;
	  su2 += thr * thr;
	  ++nok;
	}
	else
	  cout << "thr " << setw( 3 ) << thr
	       << " for ROC col row trim "
	       << setw( 2 ) << roc
	       << setw( 3 ) << col
	       << setw( 3 ) << row
	       << setw( 3 ) << modtrm[roc][col][row]
	       << endl;

      } // rows

  } // rocs

  cout << "valid thresholds " << nok << endl;

  if( nok < 2 ) return false;

  double mid = ( double ) sum / ( double ) nok;
  double rms = sqrt( ( double ) su2 / ( double ) nok - mid * mid );
  cout << "mean thr " << mid << endl;
  cout << "thr rms  " << rms << endl;

  double cut = mid - 3*rms;

  // treat weak pixels:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int thr = modthr[roc][col][row];
	if( thr == 0 )
	  cout << "thr " << setw( 3 ) << thr
	       << " for roc col row trim "
	       << setw( 2 ) << roc
	       << setw( 3 ) << col
	       << setw( 3 ) << row
	       << setw( 3 ) << modtrm[roc][col][row]
	       << endl;
	else if( thr == 255 )
	  cout << "thr " << setw( 3 ) << thr
	       << " for roc col row trim "
	       << setw( 2 ) << roc
	       << setw( 3 ) << col
	       << setw( 3 ) << row
	       << setw( 3 ) << modtrm[roc][col][row]
	       << endl;
	else if( thr < cut ) {
	  int trim = modtrm[roc][col][row];
	  cout << "roc col row trim thr"
	       << setw( 3 ) << roc
	       << setw( 3 ) << col
	       << setw( 3 ) << row
	       << setw( 3 ) << trim
	       << setw( 3 ) << thr
	       << endl;
	  int step = 1;
	  int nTrig = 16;
	  tb.roc_I2cAddr( roc );
	  tb.SetDAC( CtrlReg, 0 ); // small Vcal
	  while( thr < cut && trim < 15 ) {
	    ++trim;
	    modtrm[roc][col][row] = trim; // will be used next
	    thr = ThrPix( roc, col, row, Vcal, step, nTrig );
	    modthr[roc][col][row] = thr; // update
	    cout << "roc col row trim thr"
		 << setw( 3 ) << roc
		 << setw( 3 ) << col
		 << setw( 3 ) << row
		 << setw( 3 ) << trim
		 << setw( 3 ) << thr
		 << endl;
	  } // harden
	  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
	} // weak
      } // rows

  } // rocs

  return true;

} // modtrimbits

//------------------------------------------------------------------------------
CMD_PROC( phdac ) // phdac col row dac [stp] [nTrig] [roc] (PH vs dac)
{
  if( ierror ) return false;

  bool ldb = 0;

  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers
  int stp;
  if( !PAR_IS_INT( stp, 1, 255 ) )
    stp = 1;
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 10;
  int roc = 0;
  if( !PAR_IS_INT( roc, 0, 15 ) )
    roc = 0;

  if( dacval[roc][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  Log.section( "PHDAC", false );
  Log.printf( " ROC %i pixel %i %i DAC %i step %i ntrig %i \n",
	      roc, col, row, dac, stp, nTrig );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, stp, nTrig, nReadouts, PHavg, PHrms );

  int nstp = nReadouts.size();

  if( ldb ) cout << "nstp " << nstp << endl;

  if( h10 )
    delete h10;
  h10 = new
    TH1D( Form( "N_dac%02i_roc%02i_%02i_%02i", dac, roc, col, row ),
          Form( "responses vs %s ROC %i col %i row %i;%s [DAC];responses",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h10->Sumw2();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "ph_dac%02i_roc%02i_%02i_%02i", dac, roc, col, row ),
          Form( "PH vs %s ROC %i col %i row %i;%s [DAC];<PH> [ADC]",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "vcal_dac%02i_roc%02i_%02i_%02i", dac, roc, col, row ),
          Form( dacval[0][CtrlReg] == 0 ?
                "Vcal vs %s ROC %i col %i row %i;%s [DAC];calibrated PH [small Vcal DAC]"
                :
                "Vcal vs %s ROC %i col %i row %i;%s [DAC];calibrated PH [large Vcal DAC]",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( Form( "rms_dac%02i_roc%02i_%02i_%02i", dac, roc, col, row ),
          Form( "RMS vs %s ROC %i col %i row %i;%s [DAC];PH RMS [ADC]",
                dacName[dac].c_str(), row, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h13->Sumw2();

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

    int idac = dacStrt( dac ) + i * stp;
    if( ldb ) cout << i << " " << idac << flush;

    int nn = nReadouts.at( i );
    if( ldb ) cout << " " << nn << flush;

    double ph = PHavg.at( i );
    if( ldb ) cout << " " << ph << flush;

    Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );

    if( ph > -0.5 && ph < phmin )
      phmin = ph;

    h10->Fill( idac, nn );

    if( nn > 0 ) {
      h11->Fill( idac, ph );
      if( ldb ) cout  << " " << PHrms.at( i ) << flush;
      h13->Fill( idac, PHrms.at( i ) );
      double vc = PHtoVcal( ph, roc, col, row );
      h12->Fill( idac, vc );
      if( ldb ) cout << "  " << vc;
    }
    if( ldb ) cout << endl;
  } // dacs
  Log.printf( "\n" );
  Log.flush();

  cout << "min PH " << phmin << endl;

  h10->Write();
  h11->Write();
  h12->Write();
  h13->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 10, 11, 12, 13" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // phdac

//------------------------------------------------------------------------------
CMD_PROC( calsdac ) // calsdac col row dac [nTrig] [roc] (cals PH vs dac)
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

  int roc = 0;
  if( !PAR_IS_INT( roc, 0, 15 ) )
    roc = 0;

  int stp = 1;

  Log.section( "CALSDAC", false );
  Log.printf( " pixel %i %i DAC %i\n", col, row, dac );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  tb.SetDAC( CtrlReg, 4 ); // want large Vcal

  DacScanPix( roc, col, row, dac, stp, -nTrig, nReadouts, PHavg, PHrms ); // negative nTrig = cals

  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.Flush();

  int nstp = nReadouts.size();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "cals_dac%02i_%02i_%02i_%02i", dac, roc, col, row ),
          Form( "CALS vs %s roc %i col %i row %i;%s [DAC];<CALS> [ADC]",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "resp_cals_dac%02i_%02i_%02i_%02i", dac, roc, col, row ),
          Form( "CALS responses vs %s roc %i col %i row %i;%s [DAC];cals responses",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h12->Sumw2();

  // print:

  double phmin = 255;

  for( int32_t i = 0; i < nstp; ++i ) {

    int idac = dacStrt( dac ) + i * stp;

    double ph = PHavg.at( i );
    cout << setw( 4 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
	 <<"(" << setw( 3 ) << nReadouts.at( i ) << ")";
    Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );
    if( ph > -0.5 && ph < phmin )
      phmin = ph;
    h11->Fill( idac, ph );
    h12->Fill( idac, nReadouts.at( i ) );

  } // dacs
  cout << endl;
  Log.printf( "\n" );
  cout << "min PH " << phmin << endl;
  Log.flush();

  h11->Write();
  h12->Write();
  h12->SetStats( 0 );
  h12->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 12" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // calsdac

//------------------------------------------------------------------------------
int effvsdac( int col, int row, int dac, int stp, int nTrig, int roc )
{
  Log.section( "EFFDAC", false );
  Log.printf( " ROC %i pixel %i %i DAC %i step %i ntrig %i \n",
	      roc, col, row, dac, stp, nTrig );
  cout << endl;
  cout << "  [EFFDAC] pixel " << col << " " << row
       << " vs DAC " << dac << " step " << stp
       << ", nTrig " << nTrig
       << " for ROC " << roc
       << endl;
  cout << "  CalDel   " << dacval[0][CalDel] << endl;
  cout << "  VthrComp " << dacval[0][VthrComp] << endl;
  cout << "  CtrlReg  " << dacval[0][CtrlReg] << endl;
  cout << "  Vcal     " << dacval[0][Vcal] << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, stp, nTrig, nReadouts, PHavg, PHrms );

  int nstp = nReadouts.size();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "eff_dac%02i_roc%02i_col%02i_row%02i_stp%02i",
		dac, roc, col, row, stp ),
          Form( "Responses vs %s ROC %i col %i row %i;%s [DAC] step %i;responses",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str(), stp ),
	  nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h11->Sumw2();

  // print and plot:

  int i10 = 0;
  int i50 = 0;
  int i90 = 0;

  for( int32_t i = 0; i < nstp; ++i ) {

    int cnt = nReadouts.at( i );
    cout << setw( 4 ) << cnt;
    int idac = dacStrt( dac ) + i * stp;
    Log.printf( "%i %i\n", idac, cnt );
    h11->Fill( idac, cnt );

    if( cnt <= 0.1 * nTrig )
      i10 = idac; // thr - 1.28155 * sigma
    if( cnt <= 0.5 * nTrig )
      i50 = idac; // thr
    if( cnt <= 0.9 * nTrig )
      i90 = idac; // thr + 1.28155 * sigma

  } // dacs
  cout << endl;
  Log.printf( "\n" );
  Log.flush();

  if( dac == Vcal )
    cout << "  thr " << i50 << ", sigma " << (i90 - i10)/2.5631*stp << endl;

  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return i50;

} // effvsdac

//------------------------------------------------------------------------------
CMD_PROC( effdac ) // effdac col row dac [stp] [nTrig] [roc] (efficiency vs dac)
{
  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers
  int stp;
  if( !PAR_IS_INT( stp, 1, 255 ) )
    stp = 1;
  if( dac > 30 && stp < 10 ) stp = 10; // VD, VA [mV]
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 100;
  int roc = 0;
  if( !PAR_IS_INT( roc, 0, 15 ) )
    roc = 0;

  if( dacval[roc][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  effvsdac( col, row, dac, stp, nTrig, roc );

  return true;

} // effdac

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
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  Log.section( "THRDAC", false );
  Log.printf( " pixel %i %i DAC %i\n", col, row, dac );

  const int vdac = dacval[0][dac];

  tb.SetDAC( CtrlReg, 0 ); // small Vcal

  int nTrig = 20;
  //int start = 30;
  int step = 1;
  //int thrLevel = nTrig / 2;
  //bool xtalk = 0;
  //bool cals = 0;
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
            dacName[dac].c_str(), col, row, trim, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*dacstep, dacStop(dac) + 0.5*dacstep );
  h11->Sumw2();

  int tmin = 255;
  int imin = 0;

  for( int32_t i = dacstrt; i <= dacstop; i += dacstep ) {

    tb.SetDAC( dac, i );
    tb.uDelay( 1000 );
    tb.Flush();

    // NIOS code pixel_dtb.cc
    //for( int16_t i = 0; i < nTriggers; ++i) {
    //  Pg_Single();
    //  uDelay(4); // 4 us => WBC < 160
    //}

    int thr = ThrPix( 0, col, row, Vcal, step, nTrig );
    /* before FW 4.4
       int thr = tb.PixelThreshold( col, row,
       start, step,
       thrLevel, nTrig,
       Vcal, xtalk, cals );
    */
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

  h11->Write();
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.roc_Pix_Mask( col, row );
  tb.SetDAC( dac, vdac ); // restore dac value
  tb.SetDAC( CtrlReg, dacval[0][CtrlReg] );

  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // thrdac

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
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  Log.section( "MODTHRDAC", false );
  Log.printf( " pixel %i %i DAC %i\n", col, row, dac );

  const int vdac = dacval[roc][dac];

  tb.roc_I2cAddr( roc );
  tb.SetDAC( CtrlReg, 0 ); // small Vcal
  tb.Flush();

  int nTrig = 20; // for S-curve
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
            dacName[dac].c_str(), col, row, trim, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*dacstep, dacStop(dac) + 0.5*dacstep );
  h11->Sumw2();

  int tmin = 255;
  int imin = 0;

  for( int32_t i = dacstrt; i <= dacstop; i += dacstep ) {

    tb.SetDAC( dac, i );
    tb.uDelay( 1000 );
    tb.Flush();

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

  h11->Write();
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  tb.SetDAC( dac, vdac ); // restore dac value
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] );

  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modthrdac

//------------------------------------------------------------------------------
// utility function for ROC PH and eff map
bool GetRocData( int nTrig, vector < int16_t > & nReadouts,
                 vector < double > & PHavg, vector < double > & PHrms )
{
  uint16_t flags = 0;
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  uint16_t mTrig = abs( nTrig );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
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
  tb.Flush();

  cout << "  GetRocData mTrig " << mTrig << ", flags " << flags << endl;

  try {
    tb.LoopSingleRocAllPixelsCalibrate( 0, mTrig, flags );
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  tb.Daq_Stop();

  // header = 1 word
  // pixel = +2 words
  // size = 4160 * nTrig * 3 = 124'800 words

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );
  cout << "  DAQ size " << tb.Daq_GetSize() << endl;

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest );
    cout << "  data size " << data.size()
      //<< ", remaining " << rest
	 << " of " << data.capacity()
	 << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      cout << "  data size " << data.size()
	//<< ", remaining " << rest
	   << " of " << data.capacity()
	   << endl;
      dataB.clear();
    }
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
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
            ++cnt;
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

} // getrocdata

//------------------------------------------------------------------------------
// utility function for ROC PH and eff map inverse -masked
bool GetRocDataMasked( int nTrig, vector < int16_t > & nReadouts,
                 vector < double > & PHavg, vector < double > & PHrms )
{
  uint16_t flags = 0;
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  uint16_t mTrig = abs( nTrig );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );

  // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    // and then mask
    //tb.roc_Col_Mask( col );

    /*
      for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim ); // done on FPGA in TriggerLoops
      }
    */
  }
  tb.uDelay( 1000 );
  tb.Flush();

  // now mask all pixels and save initial trim values

  vector < uint8_t > trimvalues( 4160 );
  vector < uint8_t > trimvaluesSave( 4160 );

  for( int iroc = 0; iroc < 16; ++iroc ) {
    if( roclist[iroc] ) {
      for( int col = 0; col < 52; ++col ) {
	for( int row = 0; row < 80; ++row ) {
	  int trimSave = modtrm[iroc][col][row];
	  //tb.roc_Pix_Trim( col, row, trim );
	  int i = 80 * col + row;
	  trimvalues[i] = 128;
	  trimvaluesSave[i] = trimSave;
	} // row
      } // col
      tb.SetTrimValues( iroc, trimvalues ); // load into FPGA
    }
  }
  cout << "  GetRocData mTrig " << mTrig << ", flags " << flags << endl;

  try {
    tb.LoopSingleRocAllPixelsCalibrate( 0, mTrig, flags );
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  tb.Daq_Stop();

  // header = 1 word
  // pixel = +2 words
  // size = 4160 * nTrig * 3 = 124'800 words

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );
  cout << "  DAQ size " << tb.Daq_GetSize() << endl;

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest );
    cout << "  data size " << data.size()
	 << ", remaining " << rest << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      cout << "  data size " << data.size()
	   << ", remaining " << rest << endl;
      dataB.clear();
    }
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
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
            ++cnt;
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

  // now restore all trim-bits to original value
  // now mask all pixels and save initial trim values

  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      tb.SetTrimValues( iroc, trimvaluesSave ); // load into FPGA

  return 1;

} // getrocdatamasked

//------------------------------------------------------------------------------
bool geteffmap( int nTrig )
{
  const int vctl = dacval[0][CtrlReg];
  const int vcal = dacval[0][Vcal];

  Log.section( "EFFMAP", false );
  Log.printf( " nTrig %i Vcal %i CtrlReg %i\n", nTrig, vcal, vctl );
  cout << endl;
  cout << "  [EFFMAP] with " << nTrig << " triggers"
       << " at Vcal " << vcal << ", CtrlReg " << vctl << endl;
  
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( nTrig, nReadouts, PHavg, PHrms );

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec; // seconds since 1.1.1970
  long u2 = tv.tv_usec; // microseconds
  double dt = s2 - s0 + ( u2 - u0 ) * 1e-6;

  cout << "\tLoopSingleRocAllPixelsCalibrate takes " << dt << " s"
       << " = " << dt / 4160 / nTrig * 1e6 << " us / pix" << endl;

  if( h11 )
    delete h11;
  h11 = new TH1D( Form( "Responses_Vcal%i_CR%i", vcal, vctl ),
                  Form( "Responses at Vcal %i, CtrlReg %i;responses;pixels",
                        vcal, vctl ), nTrig + 1, -0.5, nTrig + 0.5 );
  h11->Sumw2();

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
        ++nok;
      if( cnt >= nTrig / 2 )
        ++nActive;
      if( cnt == nTrig )
        ++nPerfect;
      if( cnt < nTrig )
        cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
	     << " responses " << cnt << endl;
      Log.printf( " %i", cnt );

      ++j;
      if( j == nReadouts.size() )
        break;

    } // row

    Log.printf( "\n" );
    if( j == nReadouts.size() )
      break;

  } // col

  h11->Write();
  h21->Write();
  h21->SetStats( 0 );
  h21->SetMinimum( 0 );
  h21->SetMaximum( nTrig );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 11, 21" << endl;

  cout << endl;
  cout << " >0% pixels: " << nok << ", dead " << 4160-nok << endl;
  cout << ">50% pixels: " << nActive << endl;
  cout << "100% pixels: " << nPerfect << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // geteffmap

//------------------------------------------------------------------------------
CMD_PROC( effmap ) // single ROC response map ('PixelAlive')
{
  if( ierror ) return false;

  int nTrig; // DAQ size 1'248'000 at nTrig = 100 if fully efficient
  PAR_INT( nTrig, 1, 65000 );

  geteffmap( nTrig );

  return 1;

} // effmap

//------------------------------------------------------------------------------
CMD_PROC( effmask )
{
  if( ierror ) return false;

  int nTrig; // DAQ size 1'248'000 at nTrig = 100 if fully efficient
  PAR_INT( nTrig, 1, 65000 );

  const int vctl = dacval[0][CtrlReg];
  const int vcal = dacval[0][Vcal];

  Log.section( "EFFMASK", false );
  Log.printf( " nTrig %i Vcal %i CtrlReg %i\n", nTrig, vcal, vctl );
  cout << "effmap with " << nTrig << " triggers"
       << " at Vcal " << vcal << ", CtrlReg " << vctl << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocDataMasked( nTrig, nReadouts, PHavg, PHrms );

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec; // seconds since 1.1.1970
  long u2 = tv.tv_usec; // microseconds
  double dt = s2 - s0 + ( u2 - u0 ) * 1e-6;

  cout << "        LoopSingleRocAllPixelsCalibrate takes " << dt << " s"
       << " = " << dt / 4160 / nTrig * 1e6 << " us / pix" << endl;

  if( h21 )
    delete h21;
  h21 = new TH2D( Form( "MaskMap_Vcal%i_CR%i", vcal, vctl ),
                  Form( "Mask map at Vcal %i, CtrlReg %i;col;row;responses",
                        vcal, vctl ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  size_t j = 0;

  int nError;
  int nOk;

  nError = 0;
  nOk = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      int cnt = nReadouts.at( j );

      h11->Fill( cnt );
      if( cnt > 0 ) {
	h21->Fill( col, row, cnt );
	++nError;
      }
      else {
	h21->Fill( col, row, 1 );
	++nOk;
      }

      if( cnt >= 0 )
        cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
	     << " responses " << cnt << endl;
      Log.printf( " %i", cnt );

      ++j;
      if( j == nReadouts.size() )
        break;

    } // row

    Log.printf( "\n" );
    if( j == nReadouts.size() )
      break;

  } // col

  h21->Write();
  h21->SetStats( 0 );
  h21->SetMinimum( 0 );
  h21->SetMaximum( 2 );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histo 21" << endl;

  cout << endl;
  cout << " Pixels Masked and Responding: " << nError << endl;
  cout << " Pixels Masked and not Responding: " << nOk << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // effmask

//------------------------------------------------------------------------------
// CS 2014: utility function for module response map

void ModPixelAlive( int nTrig )
{
  for( int roc = 0; roc < 16; ++roc )
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	modcnt[roc][col][row] = 0;
	modamp[roc][col][row] = 0;	
      }

  // all on:

  vector < uint8_t > rocAddress;

  for( size_t iroc = 0; iroc < enabledrocslist.size(); ++iroc ) {
    int roc = enabledrocslist[iroc];
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    rocAddress.push_back( roc );
    vector < uint8_t > trimvalues( 4160 );
    for( int col = 0; col < 52; ++col ) {
      for( int row = 0; row < 80; ++row ) {
        int trim = modtrm[roc][col][row];
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row
    } // col
    tb.SetTrimValues( roc, trimvalues ); // load into FPGA
  } // roc
  tb.uDelay( 1000 );
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, 0 );
  tb.Daq_Open( Blocksize, 1 );
#endif
  tb.Daq_Select_Deser400();
  tb.uDelay( 100 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0; // or flags = FLAG_USE_CALS;

  //flags |= 0x0100; // FLAG_FORCE_UNMASKED: noisy

  vector < uint16_t > data[2];
  // 28 = 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail
  data[0].reserve( nTrig * 4160 * 28 );
  data[1].reserve( nTrig * 4160 * 28 );

  // measure:

  bool done = 0;
  double dtloop = 0;
  double dtread = 0;
  int loop = 0;
  timeval tv;

  while( done == 0 ) {

    ++loop;
    cout << "\tloop " << loop << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    tb.Daq_Start( 0 );
    tb.Daq_Start( 1 );
    tb.uDelay( 100 );

    try {
      done = tb.LoopMultiRocAllPixelsCalibrate( rocAddress, nTrig, flags );
    }
    catch( CRpcError & e ) {
      e.What();
      return;
    }

    cout << ( done ? "\tdone" : "\tnot done" ) << endl;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    dtloop += s2 - s1 + ( u2 - u1 ) * 1e-6;

    int dSize[2];
    dSize[0] = tb.Daq_GetSize( 0 );
    dSize[1] = tb.Daq_GetSize( 1 );

    // read TBM channels:

    for( int tbmch = 0; tbmch < 2; ++tbmch ) {

      cout << "\tDAQ size channel " << tbmch
	   << " = " << dSize[tbmch] << " words " << endl;

      //cout << "DAQ_Stop..." << endl;
      tb.Daq_Stop( tbmch );
      tb.uDelay( 100 );

      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );

      try {
        uint32_t rest;
        tb.Daq_Read( dataB, Blocksize, rest, tbmch );
        data[tbmch].insert( data[tbmch].end(), dataB.begin(),
                            dataB.end() );
        cout << "\tdata[" << tbmch << "] size " << data[tbmch].size()
	  //<< ", remaining " << rest
	     << " of " << data[tbmch].capacity()
	     << endl;
        while( rest > 0 ) {
          dataB.clear();
          tb.uDelay( 5000 );
          tb.Daq_Read( dataB, Blocksize, rest, tbmch );
          data[tbmch].insert( data[tbmch].end(), dataB.begin(),
                              dataB.end() );
          cout << "\tdata[" << tbmch << "] size " << data[tbmch].size()
	    //<< ", remaining " << rest
	       << " of " << data[tbmch].capacity()
	       << endl;
        }
      }
      catch( CRpcError & e ) {
        e.What();
        return;
      }

    } // tbmch

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

  } // while not done

  cout << "\tLoopMultiRocAllPixelsCalibrate takes "
       << dtloop << " s" << endl;
  cout << "\tDaq_Read takes " << dtread << " s" << " = "
       << 2 * ( data[0].size() + data[1].size() ) / dtread / 1024 / 1024
       << " MiB/s" << endl;

  // read and analyze:

  int PX[16] = { 0 };
  int event = 0;

  //try to match pixel address
  // loop cols
  //   loop rows
  //     activate this pix on all ROCs
  //     loop dacs
  //       loop trig

  // TBM channels:

  bool ldb = 0;
  long countLoop = -1;
  for( int tbmch = 0; tbmch < 2; ++tbmch ) {
    countLoop = -1;
    cout << "\tDAQ size channel " << tbmch
	 << " = " << data[tbmch].size() << " words " << endl;

    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t iroc = nrocsa * tbmch - 1; // will start at 0 or 8
    int32_t kroc = 0;
    cout << "\tnrocsa " << nrocsa << " nrocsb " << nrocsb << endl;
    // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data[tbmch].size(); ++i ) {

      int d = data[tbmch].at( i ) & 0xfff; // 12 bits data
      int v = ( data[tbmch].at( i ) >> 12 ) & 0xe; // 3 flag bits
      
      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;
      bool addressmatch = true;
      switch ( v ) {

	// TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
	//DecodeTbmHeader(hdr);
	if( ldb )
	  cout << "TBM header" << endl;
        iroc = nrocsa * tbmch-1; // new event, will start at 0 or 8
	++countLoop;
	if( ldb )
          cout << "event " << setw( 6 ) << event << " countloop " << countLoop << endl;

        break;

	// ROC header data:
      case 4:
	++iroc;
        kroc = enabledrocslist[iroc];
	if( ldb )
	  cout << "ROC header" << endl;
        if( ldb ) {
          if( kroc >= 0 )
          cout << "ROC " << setw( 2 ) << kroc << " iroc " << iroc << endl;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << " iroc " << iroc << endl;
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
	//        if( ldb )         
	{
	  long xi = (countLoop / nTrig ) ; 
	  int row = ( xi ) % 80;
	  int col = (xi - row) / 80;

	  if( col != x  or row != y ) {
	    addressmatch = false;
	    if( ldb ) {
	      cout << " pixel address mismatch " << endl;
	      cout << "from data " << " x " << x << " y " << y << " ph " << ph << endl;
	      cout << "from loop " << " col " << col << " row " << row << endl;
	    }
	  }
	} // dummy scope
	if( addressmatch ) {
          ++modcnt[kroc][x][y];
          modamp[kroc][x][y] += ph;
	  ++PX[kroc];
	} // dummy scope
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
          ++event;
	if( ldb )
	  cout << "TBM trailer" << endl;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "\tTBM ch " << tbmch
	 << " events " << event
	 << " = " << event / nTrig << " pixels / ROC" << endl;

  } // tbmch

  // all off:

  for( size_t iroc = 0; iroc < enabledrocslist.size(); ++iroc ) {
    int roc = enabledrocslist[iroc];
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close( 0 );
  tb.Daq_Close( 1 );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  Log.flush();

  for ( size_t iroc = 0; iroc < enabledrocslist.size(); ++iroc ) {
    int roc = enabledrocslist[iroc];
    cout << "ROC " << setw( 2 ) << roc << ", hits " << PX[roc] << endl;
  }

} // ModPixelAlive

//------------------------------------------------------------------------------
bool tunePHmod( int col, int row, int roc )
{
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  cout << "[tunePHmod] for ROC " << roc << endl;

  tb.roc_I2cAddr( roc );
  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan Vcal for one pixel

  int dac = Vcal;
  int16_t nTrig = 16;

  vector < int16_t > nReadouts; // size 0
  vector < double >PHavg;
  vector < double >PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

  if( nReadouts.size() < 256 ) {
    cout << "only " << nReadouts.size() << " Vcal points"
	 << ". choose different pixel, check CalDel, check Ia, or give up"
	 << endl;
    return 0;
  }

  if( nReadouts.at( nReadouts.size() - 1 ) < nTrig ) {
    cout << "only " << nReadouts.at( nReadouts.size() - 1 )
	 << " responses at " << nReadouts.size() - 1
	 << ". choose different pixel, check CalDel, check Ia, or give up" <<
      endl;
    return 0;
  }

  // scan from end, search for smallest responding Vcal:

  int minVcal = 255;

  for( int idac = nReadouts.size() - 1; idac >= 0; --idac )
    if( nReadouts.at( idac ) == nTrig )
      minVcal = idac;
  cout << "min responding Vcal " << minVcal << endl;
  minVcal += 1; // safety
  minVcal += 1; // safety
  minVcal += 1; // safety

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // offset and gain DACs:

  int offsdac = VoffsetOp; // digV2 dac 15, positive slope
  int gaindac = VIref_ADC; //       dac 20
  if( Chip >= 400 ) {
    offsdac = PHOffset; // digV2.1 dac 17, positive slope
    gaindac = PHScale; //         dac 20
  }

  cout << "start offset dac " << offsdac
       << " at " << dacval[roc][offsdac] << endl;
  cout << "start gain   dac " << gaindac
       << " at " << dacval[roc][gaindac] << endl;

  // set gain to minimal (at DAC 255), to avoid overflow or underflow:

  tb.SetDAC( gaindac, 255 );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Scan col row PH vs offset at Vcal 255:

  tb.SetDAC( Vcal, 255 ); // max Vcal
  vector < double >PHmax;
  PHmax.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmax, PHrms );

  // Scan col row PH vs offset at minVcal:

  tb.SetDAC( Vcal, minVcal );
  vector < double >PHmin;
  PHmin.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmin, PHrms );

  // use offset to center PH at 132:

  int offs = 0;
  double phmid = 0;
  for( size_t idac = 0; idac < PHmin.size(); ++idac ) {
    double phmean = 0.5 * ( PHmin.at( idac ) + PHmax.at( idac ) );
    if( fabs( phmean - 132 ) < fabs( phmid - 132 ) ) {
      offs = idac;
      phmid = phmean;
    }
  }

  cout << "mid PH " << phmid << " at offset " << offs << endl;

  tb.SetDAC( offsdac, offs );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan col row PH vs gain at Vcal 255 and at minVcal:

  tb.SetDAC( Vcal, 255 );
  vector < double >PHtop;
  PHtop.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHtop, PHrms );

  tb.SetDAC( Vcal, minVcal );
  vector < double >PHbot;
  PHbot.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHbot, PHrms );

  // set gain:

  int gain = PHtop.size() - 1;
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
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check for all pixels:

  tb.SetDAC( Vcal, 255 );
  tb.Flush();

  bool again = 0;

  int colmax = 0;
  int rowmax = 0;

  do { // no overflows

    ModPixelAlive( nTrig ); // fills modcnt and modamp

    double phmax = 0;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        if( modcnt[roc][col][row] < nTrig / 2 ) {
          cout << "weak pixel " << col << " " << row << endl;
        }
        else {
          double ph = 1.0*modamp[roc][col][row] / modcnt[roc][col][row];
	  if( ph > phmax ) {
	    phmax = ph;
	    colmax = col;
	    rowmax = row;
	  }
	}

      } // row

    } // col

    cout << "max PH " << phmax << " at " << colmax << " " << rowmax << endl;

    if( phmax > 252 && gain < 255 ) {

      DacScanPix( roc, colmax, rowmax, gaindac, 1, nTrig, nReadouts, PHmax, PHrms );

      // set gain:

      int gain = PHmax.size() - 1;
      for( ; gain >= 1; --gain ) {
	if( PHmax.at( gain ) > 233 )
	  break;
      }
      cout << "max PH " << PHmax.at( gain )
	   << " at gain " << gain
	   << " for Vcal 255" << " for pixel " << colmax << " " << rowmax << endl;

      gain += 1; // reduce gain
      tb.SetDAC( gaindac, gain );
      tb.Flush();

      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;

      again = 1;

    }
    else
      again = 0;

  } while( again );

  cout << "no overflows at " << dacName[gaindac] << " = " << gain << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check all pixels for underflow at minVcal:

  tb.SetDAC( Vcal, minVcal );
  tb.Flush();

  again = 0;

  int colmin = 0;
  int rowmin = 0;

  do { // no underflows

    ModPixelAlive( nTrig ); // fills modcnt and modamp

    double phmin = 255;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        if( modcnt[roc][col][row] < nTrig / 2 ) {
          cout << "pixel " << col << " " << row << " below threshold at Vcal "
	       << minVcal << endl;
        }
        else {
          double ph = 1.0*modamp[roc][col][row] / modcnt[roc][col][row];
          if( ph < phmin ) {
            phmin = ph;
            colmin = col;
            rowmin = row;
          }
        }

      } // row

    } // col

    cout << "min PH " << phmin << " at " << colmin << " " << rowmin << endl;

    if( phmin < 3 && gain > 0 ) {

      DacScanPix( roc, colmin, rowmin, gaindac, 1, nTrig, nReadouts, PHmin, PHrms );

      // set gain:

      int gain = PHmin.size() - 1;
      for( ; gain >= 1; --gain ) {
	if( PHmin.at( gain ) < 23 )
	  break;
      }
      cout << "min PH " << PHmin.at( gain )
	   << " at gain " << gain
	   << " for Vcal 255" << " for pixel " << colmin << " " << rowmin << endl;

      gain += 1; // reduce gain
      tb.SetDAC( gaindac, gain );
      tb.Flush();

      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;

      again = 1;

    }
    else
      again = 0;

  } while( again );

  cout << "no underflows at " << dacName[gaindac] << " = " << gain << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // we have min and max.
  // adjust gain and offset such that max-min = 200, mid = 132

  cout << "ROC " << roc << endl;
  cout << "min " << colmin << " " << rowmin << endl;
  cout << "max " << colmax << " " << rowmax << endl;

  again = 0;

  do {

    int cnt = 0;
    double rms;

    double phmin = 255;
    bool measure = 1;
    while( measure ) {
      tb.SetDAC( Vcal, minVcal );
      tb.Flush();
      GetPixData( roc, colmin, rowmin, nTrig, cnt, phmin, rms );
      cout << "min " << setw(2) << colmin << setw(3) << rowmin << " " << cnt
	   << " " << phmin << endl;
      if( cnt < nTrig/2 ) {
	minVcal += 1; // safety
	cout << "increase minVcal to " << minVcal << endl;
      }
      else
	measure = 0; // done
    }
    tb.SetDAC( Vcal, 255 );
    tb.Flush();
    double phmax = 0;
    GetPixData( roc, colmax, rowmax, nTrig, cnt, phmax, rms );
    cout << "max " << setw(2) << colmax << setw(3) << rowmax << " " << phmax << endl;

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
      tb.Flush();
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
      tb.Flush();
      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;
    }
  } while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // done

  cout << "ROC " << roc << endl;
  cout << "min " << colmin << " " << rowmin << endl;
  cout << "max " << colmax << " " << rowmax << endl;
  cout << "final offs dac " << dacName[offsdac] << " set to " << offs << endl;
  cout << "final gain dac " << dacName[gaindac] << " set to " << gain << endl;

  dacval[roc][offsdac] = offs;
  dacval[roc][gaindac] = gain;

  Log.printf( "[SETDAC] %i  %i\n", gaindac, gain );
  Log.printf( "[SETDAC] %i  %i\n", offsdac, offs );
  Log.flush();

  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tunePHmod

//------------------------------------------------------------------------------
CMD_PROC( modtune ) // adjust PH gain and offset to fit into ADC range
{
  if( ierror ) return false;

  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  int rocmin =  0;
  int rocmax = 15;
  int aroc;
  if( !PAR_IS_INT( aroc, 0, 15 ) )
    aroc = 16; // all
  else {
    rocmin = aroc;
    rocmax = aroc;
  }

  for( int roc = rocmin; roc <= rocmax; ++roc )
    tunePHmod( col, row, roc );

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( modmap ) // pixelAlive for modules
{
  int nTrig = 20;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 20;

  Log.section( "MODMAP", false );
  Log.printf( " nTrig %i\n", nTrig );
  cout << "modmap with " << nTrig << " triggers" << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  ModPixelAlive( nTrig ); // fills modcnt and modamp

  if( h11 )
    delete h11;
  h11 = new TH1D( Form( "Responses_Vcal%i_CR%i",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  Form( "Responses at Vcal %i, CtrlReg %i;responses;pixels",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  nTrig + 1, -0.5, nTrig + 0.5 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new TH1D( Form( "Mod_PH_spectrum_Vcal%i_CR%i",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  Form( "PH at Vcal %i, CtrlReg %i;PH [ADC];pixels",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  256, -0.5, 255.5 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new TH1D( Form( "Mod_Vcal_spectrum_Vcal%i_CR%i",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  Form( "Vcal at Vcal %i, CtrlReg %i;PH [Vcal DAC];pixels",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  256, -0.5, 255.5 );
  h13->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleHitMap",
                  "Module hit map;col;row;responses",
                  8*52, -0.5, 8*52 - 0.5, 2*80, -0.5, 2*80 - 0.5 );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis()->SetTitleOffset( 1.3 );

  if( h22 )
    delete h22;
  h22 = new TProfile2D( "ModulePHmap",
			"Module PH map;col;row;<PH> [ADC]",
			8 * 52, -0.5, 8 * 52 - 0.5,
			2 * 80, -0.5, 2 * 80 - 0.5, -1, 256 );

  if( h23 )
    delete h23;
  h23 = new TProfile2D( "ModuleVcalMap",
			"Module Vcal map;col;row;<PH> [Vcal DAC]",
			8 * 52, -0.5, 8 * 52 - 0.5,
			2 * 80, -0.5, 2 * 80 - 0.5, -1, 256 );

  int ND = 0;

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    int nn = 0;
    int nd = 0;

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int l = roc % 8; // 0..7
	int xm = 52 * l + col; // 0..415  rocs 0 1 2 3 4 5 6 7
	int ym = row; // 0..79
	if( roc > 7 ) {
	  xm = 415 - xm; // rocs 8 9 A B C D E F
	  ym = 159 - row; // 80..159
	}
	int nresponse = modcnt[roc][col][row];
	double ph = 0;
	if( nresponse > 0 ) ph = modamp[roc][col][row] / nresponse;
	double vc = PHtoVcal( ph , roc, col, row );
	if( nresponse == 0 )
	  ++nd;
	else if( nresponse >= nTrig/2 )
	  ++nn;

	h11->Fill( nresponse );
	h12->Fill( ph );
	h13->Fill( vc );
	h21->Fill( xm, ym, nresponse );
	h22->Fill( xm, ym, ph );
	h23->Fill( xm, ym, vc );

      } // row

    cout << "ROC " << setw( 2 ) << roc
	 << ": active " << nn
	 << ", dead " << nd
	 << endl;
    ND += nd;

  } // rocs

  cout << "dead " << ND << endl;

  h11->Write();
  h12->Write();
  h13->Write();
  h21->Write();
  h22->Write();
  h23->Write();
  h21->SetMinimum( 0 );
  h21->SetMaximum( nTrig );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 11, 12, 13, 21, 22, 23" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modmap

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: measure ROC threshold Vcal map

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

  //const int thrLevel = nTrig / 2;

  int mTrig = nTrig;
  if( cals ) mTrig = -nTrig; // cals flag for DacScanPix

  tb.roc_I2cAddr( roc ); // just to be sure

  tb.Flush();

  for( int col = 0; col < 52; ++col ) {

    tb.roc_Col_Enable( col, true );
    cout << setw( 3 ) << col;
    cout.flush();

    for( int row = 0; row < 80; ++row ) {

      int trim = modtrm[roc][col][row];
      tb.roc_Pix_Trim( col, row, trim );

      int thr = ThrPix( 0, col, row, Vcal, step, mTrig );
      /* before FW 4.4
	 int thr = tb.PixelThreshold( col, row,
	 start, step,
	 thrLevel, nTrig,
	 Vcal, xtalk, cals );

	 if( thr == 255 ) // try again
	 thr = tb.PixelThreshold( col, row,
	 start, step,
	 thrLevel, nTrig, Vcal, xtalk, cals );
      */
      modthr[roc][col][row] = thr;

      tb.roc_Pix_Mask( col, row );

    } // rows

    tb.roc_Col_Enable( col, 0 );

  } // cols

  cout << endl;
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore

  tb.Flush();
}

//------------------------------------------------------------------------------
CMD_PROC( thrmap ) // for single ROCs
{
  int guess;
  PAR_INT( guess, 0, 255 );

  Log.section( "THRMAP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int nTrig = 20;

  const int step = 1;
  const int xtalk = 0;
  const int cals = 0;

  const int vthr = dacval[0][VthrComp];
  const int vtrm = dacval[0][Vtrim];

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_dist_Vthr%i_Vtrm%i", vthr, vtrm ),
	  Form( "Threshold distribution Vthr %i Vtrim %i;threshold [small Vcal DAC];pixels", vthr, vtrm ),
	  255, -0.5, 254.5 ); // 255 = overflow
  h11->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( Form( "ThrMap_Vthr%i_Vtrm%i", vthr, vtrm ),
                  Form
                  ( "Threshold map at Vthr %i, Vtrim %i;col;row;threshold [small Vcal DAC]",
                    vthr, vtrm ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  int roc = 0;

  tb.roc_I2cAddr( roc );
  tb.SetDAC( CtrlReg, 0 ); // measure thresholds at ctl 0

  RocThrMap( roc, guess, step, nTrig, xtalk, cals ); // fills modthr

  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.Flush();

  int nok = 0;
  printThrMap( 1, roc, nok );

  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row ) {
      h11->Fill( modthr[roc][col][row] );
      h21->Fill( col, row, modthr[roc][col][row] );
    }

  cout << " valid thr " << nok << endl;

  h11->Write();
  h21->Write();

  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 21" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // thrmap

//------------------------------------------------------------------------------
CMD_PROC( thrmapsc ) // raw data S-curve: 60 s / ROC
{
  int stop = 255;
  int ils = 0; // small Vcal

  if( !PAR_IS_INT( stop, 30, 255 ) ) {
    stop = 255;
    ils = 4; // large Vcal
  }

  int dac = Vcal;

  Log.section( "THRMAPSC", false );
  Log.printf( " stop at %i\n", stop );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int roc = 0;

  tb.SetDAC( CtrlReg, ils ); // measure at small or large Vcal

  int nTrig = 20;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
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
  tb.Flush();

  uint16_t flags = 0; // normal CAL
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
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    tb.Daq_Start();
    tb.uDelay( 100 );

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
      e.What();
      return 0;
    }

    int dSize = tb.Daq_GetSize();

    tb.Daq_Stop(); // avoid extra (noise) data

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
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
      data.insert( data.end(), dataB.begin(), dataB.end() );
      cout << "data size " << data.size()
	   << ", remaining " << rest << endl;
      while( rest > 0 ) {
        dataB.clear();
        tb.Daq_Read( dataB, Blocksize, rest );
        data.insert( data.end(), dataB.begin(), dataB.end() );
        cout << "data size " << data.size()
	     << ", remaining " << rest << endl;
      }
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
    tread += dtr;
    cout << "Daq_Read takes " << tread << " s"
	 << " = " << 2 * dSize / tread / 1024 / 1024 << " MiB/s" << endl;

  } // while not done

  // all off:

  for( int col = 0; col < 52; ++col )
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] );
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "thr_dist",
          "Threshold distribution;threshold [small Vcal DAC];pixels",
          256, -0.5, 255.5 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "noisedist",
          "Noise distribution;width of threshold curve [small Vcal DAC];pixels",
          51, -0.5, 50.5 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( "maxdist",
          "Response distribution;responses;pixels",
          nTrig + 1, -0.5, nTrig + 0.5 );
  h13->Sumw2();

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
              ++cnt[idc];
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
      if( col == 22 && row == -22 )
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
        cout << "thr " << i50 << ", rms " << (i90 - i10)/2.5631 << endl;
      modthr[roc][col][row] = i50;
      h11->Fill( i50 );
      h12->Fill( (i90 - i10)/2.5631 );
      h13->Fill( nmx );

    } // row

    if( err )
      break;

  } // col

  Log.flush();

  int nok = 0;
  printThrMap( 1, roc, nok );

  h11->Write();
  h12->Write();
  h13->Write();
  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 12, 13" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // thrmapsc

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

} // TrimStep

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 10.8.2014: set ROC global threshold using 5 pixels
CMD_PROC( vthrcomp5 )
{
  if( ierror ) return false;

  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  int guess;
  if( !PAR_IS_INT( guess, 0, 255 ) )
    guess = 80;

  Log.section( "VTHRCOMP5", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int nTrig = 20;
  //const int thrLevel = nTrig / 2;
  const int step = 1;
  //const int xtalk = 0;
  //const int cals = 0;

  size_t roc = 0;

  tb.roc_I2cAddr( roc );

  int vthr = dacval[roc][VthrComp];
  int vtrm = dacval[roc][Vtrim];

  cout << endl
       << setw( 2 ) << "ROC " << roc
       << ", VthrComp " << vthr << ", Vtrim " << vtrm << endl;

  tb.SetDAC( CtrlReg, 0 ); // this ROC, small Vcal

  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // look at a few pixels:

  const int ntst = 5;
  int coltst[ntst] = {  1,  2, 26, 49, 50 };
  int rowtst[ntst] = {  1, 78, 40,  1, 78 };

  int vmin = 255;
  int colmin = -1;
  int rowmin = -1;

  for( int itst = 0; itst < ntst; ++itst ) {

    int col = coltst[itst];
    int row = rowtst[itst];
    tb.roc_Col_Enable( col, true );
    int trim = modtrm[roc][col][row];
    tb.roc_Pix_Trim( col, row, trim );

    int thr = ThrPix( 0, col, row, Vcal, step, nTrig );
    /* before FW 4.4
       int thr = tb.PixelThreshold( col, row,
       guess, step,
       thrLevel, nTrig,
       Vcal, xtalk, cals );
    */
    tb.roc_Pix_Mask( col, row );
    tb.roc_Col_Enable( col, 0 );
    cout
      << setw(2) << col
      << setw(3) << row
      << setw(4) << thr
      << endl;

    if( thr < vmin ) {
      vmin = thr;
      colmin = col;
      rowmin = row;
    }

  } // tst

  cout << "min thr " << vmin << " at " << colmin << " " << rowmin << endl;
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // setVthrComp using min pixel:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_vs_VthrComp_col%i_row%i", colmin, rowmin ),
	  Form
	  ( "Threshold vs VthrComp pix %i %i target %i;VthrComp [DAC];threshold [small Vcal DAC]",
	    colmin, rowmin, target ), 200, 0, 200 );
  h11->Sumw2();

  // VthrComp has negative slope: higher = softer

  int vstp = -1; // towards harder threshold
  if( vmin > target )
    vstp = 1; // towards softer threshold

  tb.roc_Col_Enable( colmin, true );
  int tbits = modtrm[roc][colmin][rowmin];
  tb.roc_Pix_Trim( colmin, rowmin, tbits );

  tb.Flush();

  guess = vmin;

  for( ; vthr < 255 && vthr > 0; vthr += vstp ) {

    tb.SetDAC( VthrComp, vthr );
    tb.uDelay( 1000 );
    tb.Flush();

    int thr = ThrPix( 0, colmin, rowmin, Vcal, step, nTrig );
    /* before FW 4.4
       int thr =
       tb.PixelThreshold( colmin, rowmin,
       guess, step,
       thrLevel, nTrig,
       Vcal, xtalk, cals );
    */
    h11->Fill( vthr, thr );
    cout << setw( 3 ) << vthr << setw( 4 ) << thr << endl;

    if( vstp * thr <= vstp * target ) // signed
      break;

  } // vthr

  h11->Draw( "hist" );
  c1->Update();
  h11->Write();

  vthr -= 1; // margin towards harder threshold
  tb.SetDAC( VthrComp, vthr );
  cout << "set VthrComp to " << vthr << endl;
  dacval[roc][VthrComp] = vthr;

  tb.roc_Pix_Mask( colmin, rowmin );
  tb.roc_Col_Enable( colmin, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.Flush();
  cout << "roc " << roc << " back to Vcal " << dacval[roc][Vcal]
       << ", CtrlReg " << dacval[roc][CtrlReg]
       << endl;

  cout << "  histos 10,11" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // vthrcomp5

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 2.6.2014: set ROC global threshold
CMD_PROC( vthrcomp )
{
  if( ierror ) return false;

  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  int guess;
  if( !PAR_IS_INT( guess, 0, 255 ) )
    guess = 50;

  Log.section( "VTHRCOMP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int nTrig = 20;
  //const int thrLevel = nTrig / 2;
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
  h10->Sumw2();

  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row )
      h10->Fill( modthr[roc][col][row] );

  h10->Draw( "hist" );
  c1->Update();
  h10->Write();

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
  h11->Sumw2();

  // VthrComp has negative slope: higher = softer

  int vstp = -1; // towards harder threshold
  if( vmin > target )
    vstp = 1; // towards softer threshold

  tb.roc_Col_Enable( colmin, true );
  int tbits = modtrm[roc][colmin][rowmin];
  tb.roc_Pix_Trim( colmin, rowmin, tbits );

  tb.Flush();

  guess = vmin;

  for( ; vthr < 255 && vthr > 0; vthr += vstp ) {

    tb.SetDAC( VthrComp, vthr );
    tb.uDelay( 1000 );
    tb.Flush();

    int thr = ThrPix( 0, colmin, rowmin, Vcal, step, nTrig );
    /* before FW 4.4
       int thr =
       tb.PixelThreshold( colmin, rowmin,
       guess, step,
       thrLevel, nTrig,
       Vcal, xtalk, cals );
    */
    h11->Fill( vthr, thr );
    cout << setw( 3 ) << vthr << setw( 4 ) << thr << endl;

    if( vstp * thr <= vstp * target ) // signed
      break;

  } // vthr

  h11->Draw( "hist" );
  c1->Update();
  h11->Write();

  vthr -= 1; // margin towards harder threshold
  tb.SetDAC( VthrComp, vthr );
  cout << "set VthrComp to " << vthr << endl;
  dacval[roc][VthrComp] = vthr;

  tb.roc_Pix_Mask( colmin, rowmin );
  tb.roc_Col_Enable( colmin, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.Flush();
  cout << "roc " << roc << " back to Vcal " << dacval[roc][Vcal]
       << ", CtrlReg " << dacval[roc][CtrlReg]
       << endl;

  cout << "  histos 10,11" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // vthrcomp

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: trim procedure, 83 s / ROC
CMD_PROC( trim )
{
  if( ierror ) return false;

  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  int guess;
  if( !PAR_IS_INT( guess, 1, 255 ) )
    guess = 50;

  Log.section( "TRIM", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int nTrig = 20;
  //const int thrLevel = nTrig / 2;
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

    int tbits = 15; // 15 = off

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
    h10->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h10->Fill( modthr[roc][col][row] );

    h10->Draw( "hist" );
    c1->Update();
    h10->Write();

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
    // set Vtrim using max pixel:

    tbits = 0; // 0 = strongest

    modtrm[roc][colmax][rowmax] = tbits;

    guess = target;

    tb.roc_Col_Enable( colmax, true );

    if( h11 )
      delete h11;
    h11 = new
      TH1D( Form( "thr_vs_Vtrim_col%i_row%i", colmax, rowmax ),
            Form
            ( "Threshold vs Vtrim pix %i %i;Vtrim [DAC];threshold [small Vcal DAC]",
              colmax, rowmax ), 128, 0, 256 );
    h11->Sumw2();

    tb.Flush();

    vtrm = 1;
    for( ; vtrm < 253; vtrm += 2 ) {

      tb.SetDAC( Vtrim, vtrm );
      tb.uDelay( 100 );
      tb.Flush();

      int thr = ThrPix( 0, colmax, rowmax, Vcal, step, nTrig );
      /* before FW 4.4
	int thr = tb.PixelThreshold( colmax, rowmax,
	guess, step,
	thrLevel, nTrig,
	Vcal, xtalk, cals );
      */
      h11->Fill( vtrm, thr );
      //cout << setw(3) << vtrm << "  " << setw(3) << thr << endl;
      if( thr < target )
        break;
    } // vtrm

    h11->Draw( "hist" );
    c1->Update();
    h11->Write();

    vtrm += 2; // margin
    tb.SetDAC( Vtrim, vtrm );
    cout << "set Vtrim to " << vtrm << endl;
    dacval[roc][Vtrim] = vtrm;

    tb.roc_Pix_Mask( colmax, rowmax );
    tb.roc_Col_Enable( colmax, 0 );
    tb.roc_ClrCal();
    tb.Flush();

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
    h12->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h12->Fill( modthr[roc][col][row] );

    h12->Draw( "hist" );
    c1->Update();
    h12->Write();

    int correction = 4;

    TrimStep( roc, target, correction, guess, step, nTrig, xtalk, cals ); // fills modtrm, fills modthr

    if( h13 )
      delete h13;
    h13 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_4", vthr, vtrm, tbits ),
	    Form( "Threshold distribution Vthr %i Vtrim %i bits %i 4;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ),
	    255, -0.5, 254.5 ); // 255 = overflow
    h13->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h13->Fill( modthr[roc][col][row] );

    h13->Draw( "hist" );
    c1->Update();
    h13->Write();

    correction = 2;

    TrimStep( roc, target, correction, guess, step, nTrig, xtalk, cals ); // fills modtrm, fills modthr

    if( h14 )
      delete h14;
    h14 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_4_2", vthr, vtrm, tbits ),
	    Form( "Threshold distribution Vthr %i Vtrim %i bits %i 4 2;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ),
	    255, -0.5, 254.5 ); // 255 = overflow
    h14->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h14->Fill( modthr[roc][col][row] );

    h14->Draw( "hist" );
    c1->Update();
    h14->Write();

    correction = 1;

    TrimStep( roc, target, correction, guess, step, nTrig, xtalk, cals ); // fills modtrm, fills modthr

    if( h15 )
      delete h15;
    h15 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_4_2_1", vthr, vtrm, tbits ),
	    Form( "Threshold distribution Vthr %i Vtrim %i bits %i 4 2 1;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ),
	    255, -0.5, 254.5 ); // 255 = overflow
    h15->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h15->Fill( modthr[roc][col][row] );

    h15->Draw( "hist" );
    c1->Update();
    h15->Write();

    correction = 1;

    TrimStep( roc, target, correction, guess, step, nTrig, xtalk, cals ); // fills modtrm, fills modthr

    if( h16 )
      delete h16;
    h16 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_4_2_1_1", vthr, vtrm, tbits ),
	    Form( "Threshold distribution Vthr %i Vtrim %i bits %i 4 2 1 1;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ),
	    255, -0.5, 254.5 ); // 255 = overflow
    h16->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        h16->Fill( modthr[roc][col][row] );

    h16->Draw( "hist" );
    c1->Update();
    h16->Write();

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
    tb.Flush();
    cout << "roc " << roc << " back to Vcal " << dacval[roc][Vcal]
	 << ", CtrlReg " << dacval[roc][CtrlReg]
	 << endl;

  } // rocs

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // trim

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 30.5.2014: fine tune trim bits (full efficiency)
CMD_PROC( trimbits )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( ierror ) return false;

  Log.section( "TRIMBITS", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

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

    for( int row = 0; row < 80 && j < nReadouts.size(); ++row ) {

      int cnt = nReadouts.at( j );
      ++j;

      //if( cnt == nTrig )
      if( cnt >= 0.98 * nTrig )
        continue; // perfect

      int trm = modtrm[roc][col][row];

      cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
	   << " trimbits " << trm << " responses " << cnt << endl;

      if( trm == 15 )
        continue;

      // increase trim bits:

      ++trm;

      for( ; trm <= 15; ++trm ) {

        modtrm[roc][col][row] = trm;

        vector < uint8_t > trimvalues( 4160 );

        for( int ix = 0; ix < 52; ++ix )
          for( int iy = 0; iy < 80; ++iy ) {
            int i = 80 * ix + iy;
            trimvalues[i] = modtrm[roc][ix][iy];
          } // iy
        tb.SetTrimValues( roc, trimvalues ); // load into FPGA
        tb.Flush();

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
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // trimbits

//------------------------------------------------------------------------------
bool tunePH( int col, int row, int roc )
{
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  //tb.SetDAC( CtrlReg, 4 ); // large Vcal
  //tb.Flush();

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

  if( nReadouts.size() < 256 ) {
    cout << "only " << nReadouts.size() << " Vcal points"
	 << ". choose different pixel, check CalDel, check Ia, or give up"
	 << endl;
    return 0;
  }

  if( nReadouts.at( nReadouts.size() - 1 ) < nTrig ) {
    cout << "only " << nReadouts.at( nReadouts.size() - 1 )
	 << " responses at " << nReadouts.size() - 1
	 << ". choose different pixel, check CalDel, check Ia, or give up" <<
      endl;
    return 0;
  }

  // scan from end, search for smallest responding Vcal:

  int minVcal = 255;

  for( int idac = nReadouts.size() - 1; idac >= 0; --idac )
    if( nReadouts.at( idac ) == nTrig )
      minVcal = idac;
  cout << "min responding Vcal " << minVcal << endl;
  minVcal += 1; // safety

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // offset and gain DACs:

  int offsdac = VoffsetOp; // digV2, positive slope
  int gaindac = VIref_ADC;
  if( Chip >= 400 ) {
    //offsdac = VoffsetRO; // digV2.1, positive slope
    offsdac = PHOffset; // digV2.1, positive slope
    gaindac = PHScale;
  }

  cout << "start offset dac " << offsdac
       << " at " << dacval[roc][offsdac] << endl;
  cout << "start gain   dac " << gaindac
       << " at " << dacval[roc][gaindac] << endl;

  // set gain to minimal (at DAC 255), to avoid overflow or underflow:

  tb.SetDAC( gaindac, 255 );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Scan PH vs offset at Vcal 255:

  tb.SetDAC( Vcal, 255 ); // max Vcal
  vector < double >PHmax;
  PHmax.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmax, PHrms );

  // Scan PH vs offset at minVcal:

  tb.SetDAC( Vcal, minVcal );
  vector < double >PHmin;
  PHmin.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmin, PHrms );

  // use offset to center PH at 132:

  int offs = 0;
  double phmid = 0;
  for( size_t idac = 0; idac < PHmin.size(); ++idac ) {
    double phmean = 0.5 * ( PHmin.at( idac ) + PHmax.at( idac ) );
    if( fabs( phmean - 132 ) < fabs( phmid - 132 ) ) {
      offs = idac;
      phmid = phmean;
    }
  }

  cout << "mid PH " << phmid << " at offset " << offs << endl;

  tb.SetDAC( offsdac, offs );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan PH vs gain at Vcal 255 and at minVcal

  tb.SetDAC( Vcal, 255 );
  vector < double >PHtop;
  PHtop.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHtop, PHrms );

  tb.SetDAC( Vcal, minVcal );
  vector < double >PHbot;
  PHbot.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHbot, PHrms );

  // set gain:

  int gain = PHtop.size() - 1;
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
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check for all pixels:

  vector < int16_t > nResponses; // size 0
  vector < double >QHmax;
  vector < double >QHrms;
  nResponses.reserve( 4160 ); // size 0, capacity 4160
  QHmax.reserve( 4160 );
  QHrms.reserve( 4160 );

  tb.SetDAC( Vcal, 255 );
  tb.Flush();

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
        if( j == nReadouts.size() )
          break;

      } // row

      if( j == nReadouts.size() )
        break;

    } // col

    cout << "max PH " << phmax << " at " << colmax << " " << rowmax << endl;

    if( phmax > 252 && gain < 255 ) {

      gain += 1; // reduce gain
      tb.SetDAC( gaindac, gain );
      tb.Flush();

      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;

      again = 1;
      nResponses.clear();
      QHmax.clear();
      QHrms.clear();

    }
    else
      again = 0;

  } while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check all pixels for underflow at minVcal:

  vector < double >QHmin;
  QHmin.reserve( 4160 );
  nResponses.clear();
  QHrms.clear();

  tb.SetDAC( Vcal, minVcal );
  tb.Flush();

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
        if( j == nReadouts.size() )
          break;

      } // row

      if( j == nReadouts.size() )
        break;

    } // col

    cout << "min PH " << phmin << " at " << colmin << " " << rowmin << endl;

    if( phmin < 3 && gain > 0 ) {

      gain += 1; // reduce gain
      tb.SetDAC( gaindac, gain );
      tb.Flush();

      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;

      again = 1;
      nResponses.clear();
      QHmin.clear();
      QHrms.clear();

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
    tb.Flush();
    double phmin = 255;
    GetPixData( roc, colmin, rowmin, nTrig, cnt, phmin, rms );

    tb.SetDAC( Vcal, 255 );
    tb.Flush();
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
      tb.Flush();
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
      tb.Flush();
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
  Log.flush();

  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tunePH

//------------------------------------------------------------------------------
CMD_PROC( tune ) // adjust PH gain and offset to fit into ADC range
{
  if( ierror ) return false;

  int roc = 0;
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  tunePH( col, row, roc );

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( phmap ) // check gain tuning and calibration
{
  if( ierror ) return false;

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
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

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
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "vcal_dist_Vcal%i_CR%i", vcal, vctl ),
          Form( dacval[0][CtrlReg] == 0 ?
                "PH Vcal distribution at Vcal %i, CtrlReg %i;calibrated PH [small Vcal DAC];pixels"
                :
                "PH Vcal distribution at Vcal %i, CtrlReg %i;calibrated PH [large Vcal DAC];pixels",
                vcal, vctl ), 256, 0, 256 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( Form( "rms_dist_Vcal%i_CR%i", vcal, vctl ),
          Form
          ( "PH RMS distribution at Vcal %i, CtrlReg %i;PH RMS [ADC];pixels",
            vcal, vctl ), 100, 0, 2 );
  h13->Sumw2();

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
        ++nok;
        sum += ph;
        su2 += ph * ph;
        if( ph < phmin )
          phmin = ph;
        if( ph > phmax )
          phmax = ph;
        h11->Fill( ph );
        double vc = PHtoVcal( ph, 0, col, row );
        h12->Fill( vc );
        h13->Fill( rms );
        h21->Fill( col, row, ph );
        h22->Fill( col, row, vc );
        h23->Fill( col, row, rms );
      }

      ++j;
      if( j == nReadouts.size() )
        break;

    } // row

    Log.printf( "\n" );
    if( j == nReadouts.size() )
      break;

  } // col

  cout << endl;

  h11->Write();
  h12->Write();
  h13->Write();
  h21->Write();
  h22->Write();
  h23->Write();
  cout << "  histos 11, 12, 13, 21, 22" << endl;

  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw( "hist" );
  h12->SetLineColor( 2 );
  h12->SetMarkerColor( 2 );
  h12->Draw( "same" );
  c1->Update();

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
  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // phmap

//------------------------------------------------------------------------------
CMD_PROC( calsmap ) // test PH map through sensor
{
  if( ierror ) return false;

  int nTrig;
  PAR_INT( nTrig, 1, 65000 );

  Log.section( "CALSMAP", false );
  Log.printf( " CtrlReg %i Vcal %i nTrig %i\n",
              dacval[0][CtrlReg], dacval[0][Vcal], nTrig );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

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
  h11->Sumw2();

  int nok = 0;
  double sum = 0;
  double su2 = 0;
  double phmin = 256;
  double phmax = -1;

  size_t j = 0;

  for( int col = 0; col < 52; ++col ) {
    cout << endl << setw( 2 ) << col << " ";
    for( int row = 0; row < 80 && j < nReadouts.size(); ++row ) {
      int cnt = nReadouts.at( j );
      double ph = PHavg.at( j );
      cout << setw( 4 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
	   <<"(" << setw( 3 ) << cnt << ")";
      if( row % 10 == 9 )
        cout << endl << "   ";
      Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );
      if( cnt > 0 ) {
        ++nok;
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

  h11->Write();
  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

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
  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // calsmap

//------------------------------------------------------------------------------
CMD_PROC( bbtest ) // bump bond test
{
  if( ierror ) return false;

  int nTrig;
  PAR_INT( nTrig, 1, 65000 );

  Log.section( "BBTEST", false );
  Log.printf( " nTrig %i\n", nTrig );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int vctl = dacval[0][CtrlReg];
  tb.SetDAC( CtrlReg, 4 ); // large Vcal

  const int vcal = dacval[0][Vcal];
  tb.SetDAC( Vcal, 255 ); // max Vcal

  tb.Flush();

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
  tb.Flush();

  // analyze:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "calsPH",
          "Cals PH distribution;Cals PH [ADC];pixels", 256, 0, 256 );
  h11->Sumw2();

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

  size_t j = 0; // index in data vectors

  for( int col = 0; col < 52; ++col ) {
    cout << endl << setw( 2 ) << col << " ";
    for( int row = 0; row < 80 && j < nReadouts.size(); ++row ) {
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
        ++nok;
        if( cnt >= nTrig / 2 )
          ++nActive;
        if( cnt == nTrig )
          ++nPerfect;
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
  Log.flush();

  h11->Write();
  h21->Write();
  h22->Write();
  cout << "  histos 11, 21, 22" << endl;

  h21->SetStats( 0 );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->SetMinimum( 0 );
  h21->SetMaximum( nTrig );
  h21->Draw( "colz" );
  c1->Update();

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
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // bbtest

//------------------------------------------------------------------------------
void CalDelRoc() // scan and set CalDel using all pixel: 17 s
{
  Log.section( "CALDELROC", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

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
    TH1D( "caldel_act", "CalDel ROC scan;CalDel [DAC];active pixels",
	  128, -1, 255 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "caldel_per", "CalDel ROC scan;CalDel [DAC];perfect pixels",
	  128, -1, 255 );
  h12->Sumw2();

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize ); // 2^24
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
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
  tb.Flush();

  int n0 = 0;
  int idel = 10;
  int step = 10;

  do {

    tb.SetDAC( CalDel, idel );
    tb.Daq_Start();
    tb.uDelay( 1000 );
    tb.Flush();

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
      ++n0; // count empty responses

    idel += step;

  }
  while( n0 < 4 && idel < 256 );

  cout << endl;
  Log.flush();

  // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
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

  h11->Write();
  h12->Write();

  h11->SetStats( 0 );
  h12->SetStats( 0 );
  h12->Draw( "hist" );
  c1->Update();

  cout << "  histos 11, 12" << endl;

  // analyze:

  int nmax = 0;
  int i0 = 0;
  int i9 = 0;

  for( int j = 0; j < nstp; ++j ) {

    int idel = plx[j];
    //int nn = pl5[j]; // 50% active
    int nn = pl9[j]; // 100% perfect
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
    tb.Flush();
    dacval[0][CalDel] = i2;
    printf( "set CalDel to %i\n", i2 );
    Log.printf( "[SETDAC] %i  %i\n", CalDel, i2 );
  }
  else
    tb.SetDAC( CalDel, val ); // back to default

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

}

//------------------------------------------------------------------------------
CMD_PROC( caldelroc ) // scan and set CalDel using all pixel: 17 s
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( ierror ) return false;
  CalDelRoc();
  return true;
}

//------------------------------------------------------------------------------
// Jamie Cameron, 11.10.14. Measure Thrmap RMS as a function of VthrComp.

CMD_PROC( scanvthr )
{
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int vthrmin;
  PAR_INT( vthrmin, 0, 254 );
  int vthrmax;
  PAR_INT( vthrmax, 1, 255 );
  int vthrstp;
  if( !PAR_IS_INT( vthrstp, 1, 255 ) )
    vthrstp = 5;

  if(vthrmin > vthrmax) {
    cout << "vthrmin > vthrmax, switching parameters." << endl;
    int temp = vthrmax;
    vthrmax = vthrmin;
    vthrmin = temp;
  }

  int nstp = (vthrmax-vthrmin)/vthrstp + 1;
  cout << "Number of steps " << nstp << endl;

  double mid = 150 - 1.35*vthrmin; // empirical estimate of first guess.

  const int roc = 0;
  const int nTrig = 20;
  const int step = 1;
  const int xtalk = 0;
  const int cals = 0;

  tb.roc_I2cAddr( roc );
  tb.SetDAC( CtrlReg, 0 ); // measure thresholds at ctl 0
  tb.Flush();

  Log.section( "SCANVTHR", true );

  if( h13 )
    delete h13;
  h13 = new
    TProfile( "vthr_scan_caldel",
	      "Vthr scan caldel;VthrComp [DAC];CalDelRoc [DAC]",
	      nstp, vthrmin - 0.5*vthrstp, vthrmax + 0.5*vthrstp );
  if( h14 )
    delete h14;
  h14 = new
    TProfile( "vthr_scan_thrmean",
	      "Vthr scan thr mean;VthrComp [DAC];Threshold Mean [DAC]",
	      nstp, vthrmin - 0.5*vthrstp, vthrmax + 0.5*vthrstp );
  if( h15 )
    delete h15;
  h15 = new
    TProfile( "vthr_scan_thrrms",
	      "Vthr scan thr RMS;VthrComp [DAC];Threshold RMS [DAC]",
	      nstp, vthrmin - 0.5*vthrstp, vthrmax + 0.5*vthrstp );

  for( int vthr = vthrmin; vthr <= vthrmax; vthr += vthrstp ) {

    cout << "VthrComp " << vthr << endl;

    tb.SetDAC( VthrComp, vthr );

    // caldelroc:
    cout << "CalDelRoc..." << endl;
    CalDelRoc(); // fills histos 11, 12

    h13->Fill( vthr,  dacval[0][CalDel] );

    // take thrmap:
    cout << "Thrmap with guess " << int(mid) << endl;
    RocThrMap( roc, int(mid), step, nTrig, xtalk, cals ); // guess = previous mean

    int nok = 0;
    int sum = 0;
    int su2 = 0;

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int thr = modthr[roc][col][row];

	// update RMS parameters:
	if( thr < 255 ) {
	  sum += thr;
	  su2 += thr * thr;
	  ++nok;
	}
      }

    // calculate RMS:

    cout << "nok " << nok << endl;

    if( nok > 0 ) {
      mid = ( double ) sum / ( double ) nok;
      double rms = sqrt( ( double ) su2 / ( double ) nok - mid * mid );
      cout << "  mean thr " << mid << ", rms " << rms << endl;
      Log.flush();

      Log.printf( "%i %f\n", vthr, mid );
      h14->Fill( vthr, mid );

      Log.printf( "%i %f\n", vthr, rms );
      h15->Fill( vthr, rms );

    }

  } // vthr loop

  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.SetDAC( VthrComp, dacval[roc][VthrComp] );
  tb.SetDAC( Vcal, dacval[roc][Vcal] );
  tb.Flush();

  // plot profile:
  h13->Write();

  h14->Write();

  h15->Write();
  h15->SetStats( 0 );
  h15->Draw( "hist" );
  c1->Update();

  cout << "  histos 13, 14, 15" << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "scan duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // scanvthr

//------------------------------------------------------------------------------
CMD_PROC( vanaroc ) // scan and set Vana using all pixel: 17 s
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( ierror ) return false;

  Log.section( "VANAROC", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

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
    TProfile( "active_vs_vana", "Vana ROC scan;Vana [DAC];active pixels",
	      256, -0.5, 255.5, -1, 9999 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TProfile( "perfect_vs_vana", "Vana ROC scan;Vana [DAC];perfect pixels",
              256, -0.5, 255.5, -1, 9999 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TProfile( "perfect_vs_ia", "efficiency vs Ia;IA [mA];perfect pixels",
	      80, 0, 80, -1, 9999 );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize ); // 2^24
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
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
  tb.Flush();

  int idac = 10;
  int step = 10;
  int nmax = 0;
  int kmax = 0;

  do {

    tb.SetDAC( Vana, idac );
    tb.Daq_Start();
    tb.uDelay( 1000 );
    tb.Flush();

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
      ++kmax;

    double ia = tb.GetIA() * 1E3;

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
  Log.flush();

  // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
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

  h11->Write();
  h12->Write();
  h13->Write();
  if( par.isInteractive() ) {
    h11->SetStats( 0 );
    h12->SetStats( 0 );
    h13->SetStats( 0 );
    h12->Draw( "hist" );
    c1->Update();
  }
  cout << "  histos 11, 12, 13" << endl;

  // analyze:

  nmax = 0;
  int i0 = 0;
  int i9 = 0;

  for( int j = 0; j < nstp; ++j ) {

    int idac = plx[j];
    //int nn = pl5[j]; // 50% active
    int nn = pl9[j]; // 100% perfect
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
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // vanaroc

//------------------------------------------------------------------------------
CMD_PROC( gaindac ) // calibrated PH vs Vcal: check gain
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( ierror ) return false;

  Log.section( "GAINDAC", false );
  Log.printf( " CtrlReg %i\n", dacval[0][CtrlReg] );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "vc_dac",
          dacval[0][CtrlReg] == 0 ?
          "mean Vcal PH vs Vcal;Vcal [DAC];calibrated PH [small Vcal DAC]" :
          "mean Vcal PH vs Vcal;Vcal [DAC];calibrated PH [large Vcal DAC]",
          256, -0.5, 255.5 );
  h11->Sumw2();

  if( h14 )
    delete h14;
  h14 = new
    TH1D( "rms_dac",
          dacval[0][CtrlReg] == 0 ?
          "PH Vcal RMS vs Vcal;Vcal [DAC];calibrated PH RMS [small Vcal DAC]"
          :
          "PH Vcal RMS vs Vcal;Vcal [DAC];calibrated PH RMS [large Vcal DAC]",
          256, -0.5, 255.5 );
  h14->Sumw2();

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
    tb.Daq_Start();
    tb.uDelay( 1000 );
    tb.Flush();

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
      for( int row = 0; row < 80 && j < nReadouts.size(); ++row ) {
        int cnt = nReadouts.at( j );
        double ph = PHavg.at( j );
        hph[j].Fill( ical, ph );
        double vc = PHtoVcal( ph, 0, col, row );
        if( cnt > 0 && vc < 999 ) {
          ++nok;
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
  tb.Flush();

  j = 0;
  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row ) {
      hph[j].Write();
      ++j;
    }

  h14->Write();
  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 14" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // gaindac

//------------------------------------------------------------------------------
bool dacscanroc( int dac, int nTrig=10, int step=1, int stop=255 )
{
  cout << "scan dac " << dacName[dac]
       << " step " << step
       << " at CtrlReg " << dacval[0][CtrlReg]
       << ", VthrComp " << dacval[0][VthrComp]
       << ", CalDel " << dacval[0][CalDel]
       << endl;

  Log.section( "DACSCANROC", false );
  Log.printf( " DAC %i, step %i, nTrig %i\n", dac, step, nTrig );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
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

  int ctl = dacval[0][CtrlReg];
  int cal = dacval[0][Vcal];

  if( nTrig < 0 ) {
    tb.SetDAC( CtrlReg, 4 ); // large Vcal
    ctl = 4;
  }

  tb.Flush();

  uint16_t flags = 0; // normal CAL
  if( nTrig < 0 ) {
    flags = 0x0002; // FLAG_USE_CALS
    cout << "CALS used..." << endl;
  }
  // flags |= 0x0010; // FLAG_FORCE_MASK is FPGA default

  uint16_t mTrig = abs( nTrig );

  int16_t dacLower1 = dacStrt( dac );
  int16_t dacUpper1 = stop;
  int32_t nstp = (dacUpper1 - dacLower1) / step + 1;

  // measure:

  cout << "pulsing 4160 pixels with " << mTrig << " triggers for "
       << nstp << " DAC steps may take " << int ( 4160 * mTrig * nstp * 6e-6 ) + 1
       << " s..." << endl;

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
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    tb.Daq_Start();
    tb.uDelay( 100 );

    try {
      done =
	tb.LoopSingleRocAllPixelsDacScan( 0, mTrig, flags, dac, step, dacLower1, dacUpper1 );
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    int dSize = tb.Daq_GetSize();
    cout << "DAQ size " << dSize << " words" << endl;

    tb.Daq_Stop(); // avoid extra (noise) data

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
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
      data.insert( data.end(), dataB.begin(), dataB.end() );
      cout << "data size " << data.size()
	//<< ", remaining " << rest
	   << " of " << data.capacity()
	   << endl;
      while( rest > 0 ) {
	dataB.clear();
	tb.Daq_Read( dataB, Blocksize, rest );
	data.insert( data.end(), dataB.begin(), dataB.end() );
	cout << "data size " << data.size()
	  //<< ", remaining " << rest
	     << " of " << data.capacity()
	     << endl;
      }
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
    tread += dtr;
    cout << "Daq_Read takes " << tread << " s"
	 << " = " << 2 * dSize / tread / 1024 / 1024 << " MiB/s" << endl;

    if( !done )
      cout << "loop more..." << endl << flush;

  } // while not done

  tb.SetDAC( dac, dacval[0][dac] ); // restore
  tb.SetDAC( CtrlReg, dacval[0][CtrlReg] ); // restore

  // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  if( h11 )
    delete h11;
  h11 = new TH1D( "Responses",
		  "Responses;max responses;pixels",
		  mTrig + 1, -0.5, mTrig + 0.5 );
  h11->Sumw2();

  dacUpper1 = dacLower1 + (nstp-1)*step; // 255 or 254 (23.8.2014)

  if( h21 )
    delete h21;
  if( nTrig < 0 ) // cals
    h21 = new TH2D( Form( "cals_PH_DAC%02i_map", dac ),
		    Form( "cals PH vs %s map;pixel;%s [DAC];cals PH [ADC]",
			  dacName[dac].c_str(),
			  dacName[dac].c_str() ),
		    4160, -0.5, 4160-0.5,
		    nstp, dacLower1 - 0.5*step, dacUpper1 + 0.5*step );
  else
    h21 = new TH2D( Form( "PH_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
		    Form( "PH vs %s CR %i Vcal %i map;pixel;%s [DAC];PH [ADC]",
			  dacName[dac].c_str(), ctl, cal,
			  dacName[dac].c_str() ),
		    4160, -0.5, 4160-0.5,
		    nstp, dacLower1 - 0.5*step, dacUpper1 + 0.5*step ); // 23.8.2014 * step

  if( h22 )
    delete h22;
  if( nTrig < 0 ) // cals
    h22 = new TH2D( Form( "cals_N_DAC%02i_map", dac ),
		    Form( "cals N vs %s map;pixel;%s [DAC];cals responses",
			  dacName[dac].c_str(),
			  dacName[dac].c_str() ),
		    4160, -0.5, 4160-0.5,
		    nstp, dacLower1 - 0.5*step, dacUpper1 + 0.5*step );
  else
    h22 = new TH2D( Form( "N_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
		    Form( "N vs %s CR %i Vcal %i map;pixel;%s [DAC];responses",
			  dacName[dac].c_str(), ctl, cal,
			  dacName[dac].c_str() ),
		    4160, -0.5, 4160-0.5,
		    nstp, dacLower1 - 0.5*step, dacUpper1 + 0.5*step ); // 23.8.2014 * step

  if( h23 )
    delete h23;
  h23 = new TH2D( "ResponseMap",
		  "Response map;col;row;max responses",
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
	      ++cnt;
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
	uint32_t idc = dacLower1 + step*j;
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

  h11->Write();
  h21->Write();
  h22->Write();
  h23->Write();

  if( nTrig < 0 && dac == 12 ) { // BB test

    if( h12 )
      delete h12;
    h12 = new TH1D( "CalsVthrPlateauWidth",
		    "Width of VthrComp plateau for cals;width of VthrComp plateau for cals [DAC];pixels",
		    151, -0.5, 150.5 );
    h12->Sumw2();

    if( h24 )
      delete h24;
    h24 = new TH2D( "BBtestMap",
		    "BBtest map;col;row;cals VthrComp response plateau width",
		    52, -0.5, 51.5, 80, -0.5, 79.5 );

    if( h25 )
      delete h25;
    h25 = new TH2D( "BBqualityMap",
		    "BBtest quality map;col;row;dead good missing",
		    52, -0.5, 51.5, 80, -0.5, 79.5 );

    // Localize the missing Bump from h22

    int nbinx = h22->GetNbinsX(); // pix
    int nbiny = h22->GetNbinsY(); // dac

    int nActive = 0;
    int nMissing = 0;
    int nIneff = 0;

    int ipx;

    // pixels:

    for( int ibin = 1; ibin <= nbinx; ++ibin ) {

      ipx = h22->GetXaxis()->GetBinCenter( ibin );

      // max response:

      int imax;
      imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = h22->GetBinContent( ibin, j );
	if( cnt > imax )
	  imax = cnt;
      }

      if( imax < mTrig / 2 ) {
	++nIneff;
	cout << "Dead pixel at col row " << ipx / 80
	     << " " << ipx % 80 << endl;
      }
      else {

	// Search for the Plateau

	int iEnd = 0;
	int iBegin = 0;

	// search for DAC response plateau:

	for( int jbin = 0; jbin <= nbiny; ++jbin ) {

	  int idac = h22->GetYaxis()->GetBinCenter( jbin );

	  int cnt = h22->GetBinContent( ibin, jbin );

	  if( cnt >= imax / 2 ) {
	    iEnd = idac; // end of plateau
	    if( iBegin == 0 )
	      iBegin = idac; // begin of plateau
	  }
	}

	// cout << "Bin: " << ibin << " Plateau Begins and End in  " << iBegin << " - " << iEnd << endl;
	// narrow plateau is from noise

	h12->Fill( iEnd - iBegin );
	h24->Fill( ipx / 80, ipx % 80, iEnd - iBegin );

	if( iEnd - iBegin < 35 ) {

	  ++nMissing;
	  cout << "Missing Bump at col row " << ipx / 80
	       << " " << ipx % 80 << endl;
	  Log.printf( "[Missing Bump at col row] %i %i\n", ibin / 80, ibin % 80 );
	  h25->Fill( ipx / 80, ipx % 80, 1 ); // red
	}
	else {
	  ++nActive;
	  h25->Fill( ipx / 80, ipx % 80, 2 ); // green
	} // plateau width

      } // active imax

    } // x-bins

    h12->Write();

    // save the map in the log file

    for( int ibin = 1; ibin <= h24->GetNbinsX(); ++ibin ) {
      for( int jbin = 1; jbin <= h24->GetNbinsY(); ++jbin ) {
	int c_val = h24->GetBinContent( ibin, jbin );
	Log.printf( " %i", c_val );
	//cout << ibin << " " << jbin << " " << c_val << endl;
      } //row
      Log.printf( "\n" );
    } // col
    Log.flush();

    Log.printf( "Number of Active bump bonds [above trig/2]: %i\n", nActive );
    Log.printf( "Number of Missing bump bonds: %i\n", nMissing );
    Log.printf( "Number of Dead pixel: %i\n", nIneff );

    cout << "Number of Active bump bonds: " << nActive << endl;
    cout << "Number of Missing bump bonds: " << nMissing << endl;
    cout << "Number of Dead pixel: " << nIneff << endl;

    h24->Write();
    h25->Write();

    h25->SetStats( 0 );
    h25->SetMinimum(0);
    h25->SetMaximum(2);
    h25->Draw( "colz" );
    c1->Update();
    cout << "  histos 11, 12, 21, 22, 23, 24, 25" << endl;

  } // BB test

  else {
    h22->SetStats( 0 );
    h22->Draw( "colz" );
    c1->Update();
    cout << "  histos 11, 21, 22, 23" << endl;
  }

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // dacscanroc

//------------------------------------------------------------------------------
CMD_PROC( dacscanroc ) // LoopSingleRocAllPixelsDacScan: 72 s with nTrig 10
// dacscanroc dac [[-]ntrig] [step] [stop]
// PH vs Vcal (dac 25) = gain calibration
// N  vs Vcal (dac 25) = Scurves
// cals N  vs VthrComp (dac 12) = bump bond test (give negative ntrig)
{
  if( ierror ) return false;

  int dac;
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  int nTrig;
  if( !PAR_IS_INT( nTrig, -65000, 65000 ) )
    nTrig = 10;

  int step;
  if( !PAR_IS_INT( step, 1, 8 ) )
    step = 1;

  int stop;
  if( !PAR_IS_INT( stop, 1, 255 ) )
    stop = dacStop( dac );

  dacscanroc( dac, nTrig, step, stop );

  return true;
}

//------------------------------------------------------------------------------
bool dacdac( int col, int row, int dac1, int dac2, int cals=0 )
{
  Log.section( "DACDAC", false );
  Log.printf( " pixel %i %i DAC %i vs %i\n", col, row, dac1, dac2 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize ); // 2^24
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );

  // pixel on:

  tb.roc_Col_Enable( col, 1 );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.Flush();

  uint16_t nTrig = 10; // size = 4160 * 256 * nTrig * 3 words = 32 MW for 10 trig

  uint16_t flags = 0; // normal CAL

  if( cals )
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
  long s1 = tv.tv_sec; // seconds since 1.1.1970
  long u1 = tv.tv_usec; // microseconds

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
    e.What();
    return 0;
  }

  tb.Daq_Stop(); // avoid extra (noise) data

  int dSize = tb.Daq_GetSize();

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec; // seconds since 1.1.1970
  long u2 = tv.tv_usec; // microseconds
  double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;

  cout << "LoopSingleRocOnePixelDacDacScan takes " << dt << " s"
       << " = " << dt / nTrig / nstp1 / nstp2 * 1e6 << " us / pix" << endl;

  tb.SetDAC( dac1, dacval[0][dac1] ); // restore
  tb.SetDAC( dac2, dacval[0][dac2] ); // restore

  tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

  // header = 1 word
  // pixel = +2 words
  // size = 256 dacs * 4160 pix * nTrig * 3 words = 32 MW

  cout << "DAQ size " << dSize << endl;

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest );
    cout << "data size " << data.size()
      //<< ", remaining " << rest
	 << " of " << data.capacity()
	 << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      cout << "data size " << data.size()
	//<< ", remaining " << rest
	   << " of " << data.capacity()
	   << endl;
      dataB.clear();
    }
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  gettimeofday( &tv, NULL );
  long s3 = tv.tv_sec; // seconds since 1.1.1970
  long u3 = tv.tv_usec; // microseconds
  double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
  cout << "Daq_Read takes " << dtr << " s"
       << " = " << 2 * dSize / dtr / 1024 / 1024 << " MiB/s" << endl;

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  if( h21 )
    delete h21;
  h21 = new
    TH2D( Form( "PH_DAC%02i_DAC%02i_col%02i_row%02i", dac1, dac2, col, row ),
          Form( "PH vs %s vs %s col %i row %i;%s [DAC];%s [DAC];PH [ADC]",
                dacName[dac1].c_str(), dacName[dac2].c_str(), col, row,
                dacName[dac1].c_str(), dacName[dac2].c_str() ),
          nstp1, -0.5, nstp1 - 0.5, nstp2, -0.5, nstp2 - 0.5 );

  if( h22 )
    delete h22;
  h22 = new
    TH2D( Form( "N_DAC%02i_DAC%02i_col%02i_row%02i", dac1, dac2, col, row ),
          Form( "N vs %s vs %s col %i row %i;%s [DAC];%s [DAC];responses",
                dacName[dac1].c_str(), dacName[dac2].c_str(), col, row,
                dacName[dac1].c_str(), dacName[dac2].c_str() ),
          nstp1, -0.5, nstp1 - 0.5, nstp2, -0.5, nstp2 - 0.5 );

  if( h23 )
    delete h23;
  h23 = new
    TH2D( Form
          ( "N_Trig2DAC%02i_DAC%02i_col%02i_row%02i", dac1, dac2, col, row ),
          Form
          ( "Responses vs %s vs %s col %i row %i;%s [DAC];%s [DAC];responses",
            dacName[dac1].c_str(), dacName[dac2].c_str(), col, row,
            dacName[dac1].c_str(), dacName[dac2].c_str() ), nstp1, -0.5,
          nstp1 - 0.5, nstp2, -0.5, nstp2 - 0.5 );

  TH1D *h_one =
    new TH1D( Form( "h_optimal_DAC%02i_col%02i_row%02i", dac2, col, row ),
              Form( "optimal DAC %i col %i row %i", dac2, col, row ),
              nstp1, -0.5, nstp1 - 0.5 );
  h_one->Sumw2();

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
            ++cnt;
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

  Log.flush();

  h21->Write();
  h22->Write();
  h23->Write();

  h23->SetStats( 0 );
  h23->Draw( "colz" );
  c1->Update();
  cout << "  histos 21, 22, 23" << endl;

  if( cals && dac1 == 26 && dac2 == 12 ) { // Tornado plot: VthrComp vs CalDel

    int dac1Mean = int ( h23->GetMean( 1 ) );

    int i1 = h23->GetXaxis()->FindBin( dac1Mean );

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

    h_one->Write();
    delete h_one;

  } // Tornado

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // dacdac

//------------------------------------------------------------------------------
CMD_PROC( dacdac ) // LoopSingleRocOnePixelDacDacScan: 
// dacdac 22 33 26 12 0 = N vs CalDel and VthrComp = tornado
// dacdac 22 33 26 25 0 = N vs CalDel and Vcal = timewalk
// dacdac 22 33 26 12 = cals N vs CalDel and VthrComp = tornado, set VthrComp
{
  if( ierror ) return false;

  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  int dac1;
  PAR_INT( dac1, 1, 32 ); // only DACs, not registers
  int dac2;
  PAR_INT( dac2, 1, 32 ); // only DACs, not registers
  int cals;
  if( !PAR_IS_INT( cals, 0, 1 ) )
    cals = 1;

  if( dacval[0][dac1] == -1 ) {
    cout << "DAC " << dac1 << " not active" << endl;
    return false;
  }
  if( dacval[0][dac2] == -1 ) {
    cout << "DAC " << dac2 << " not active" << endl;
    return false;
  }

  dacdac( col, row, dac1, dac2, cals );

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
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  int i;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Pg_Stop();
  tb.Pg_SetCmd( 0, PG_TOK );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );

  // take data:

  tb.Daq_Start();
  for( i = 0; i < 36; ++i ) {
    tb.Pg_Single();
    tb.uDelay( 20 );
  }
  tb.Daq_Stop();

  // read out data
  vector < uint16_t > data;
  tb.Daq_Read( data, 40 );
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
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

} // readback

//------------------------------------------------------------------------------
CMD_PROC( oneroc ) // single ROC test
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  int roc = 0;

  int target = 30; // [IA [mA]
  if( !setia( target ) ) return 0;

  int ctl = dacval[roc][CtrlReg];

  // caldel at large Vcal:

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  int nTrig = 100;
  bool ok = 0;
  unsigned int col = 26;
  unsigned int row = 40;
  unsigned int ntry = 0;
  while( !ok && ntry < 20 ) {
    ok = setcaldel( col, row, nTrig );
    col = (col+1) % 50 + 1; // 28, 30, 32..50, 2, 4, 
    row = (row+1) % 78 + 1; // 42, 44, 46..78, 2, 4, 
    ++ntry;
  }
  if( !ok ) return 0;

  // response map at large Vcal:

  geteffmap( nTrig );

  // threshold at small Vcal:

  tb.SetDAC( CtrlReg, 0 ); // small Vcal
  dacval[roc][CtrlReg] = 0;
  int stp = 1;
  int dac = Vcal;
  ntry = 0;
  int vthr = dacval[roc][VthrComp];
  int thr = effvsdac( col, row, dac, stp, nTrig, roc );
  target = 50; // Vcal threshold [DAC]

  while( abs(thr-target) > 5 && ntry < 11 && vthr > 5 && vthr < 250 ) {

    if( thr > target )
      vthr += 5;
    else
      vthr -= 5;

    tb.SetDAC( VthrComp, vthr );

    dacval[roc][VthrComp] = vthr;

    ok = setcaldel( col, row, nTrig ); // update CalDel

    thr = effvsdac( col, row, dac, stp, nTrig, roc );

    ++ntry;

  } // vthr

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  tunePH( col, row, roc ); // opt PHoffset, PHscale into ADC range

  // S-curves, noise, gain, cal:

  tb.SetDAC( CtrlReg, 0 ); // small Vcal
  dacval[roc][CtrlReg] = 0;
  dacscanroc( 25, 16, 1, 127 ); // dac nTrig step stop

  tb.SetDAC( CtrlReg, ctl ); // restore
  dacval[roc][CtrlReg] = ctl;
  tb.Flush();

  return ok;

} // oneroc

//------------------------------------------------------------------------------
CMD_PROC( bare ) // bare module test, without or with Hansen bump height test
{
  int Hansen;
  if( !PAR_IS_INT( Hansen, 0, 1 ) )
    Hansen = 1;

  int roc = 0;

  int target = 30; // [IA [mA]
  if( !setia( target ) ) return 0;

  int ctl = dacval[roc][CtrlReg];

  // caldel at large Vcal:

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  int nTrig = 100;
  bool ok = 0;
  unsigned int col = 26;
  unsigned int row = 40;
  unsigned int ntry = 0;
  while( !ok && ntry < 20 ) {
    ok = setcaldel( col, row, nTrig );
    // % has higher precedence than +
    col = (col+1) % 50 + 1; // 28, 30, 32..50, 2, 4, 
    row = (row+1) % 78 + 1; // 42, 44, 46..78, 2, 4, 
    ++ntry;
  }
  if( !ok ) return 0;

  // response map at large Vcal:

  geteffmap( nTrig );

  // threshold at small Vcal:

  tb.SetDAC( CtrlReg, 0 ); // small Vcal
  dacval[roc][CtrlReg] = 0;
  int stp = 1;
  int dac = Vcal;
  ntry = 0;
  int vthr = dacval[roc][VthrComp];
  int thr = effvsdac( col, row, dac, stp, nTrig, roc );
  target = 50; // Vcal threshold [DAC]

  while( abs(thr-target) > 5 && ntry < 11 && vthr > 5 && vthr < 250 ) {

    if( thr > target )
      vthr += 5;
    else
      vthr -= 5;

    tb.SetDAC( VthrComp, vthr );

    dacval[roc][VthrComp] = vthr;

    ok = setcaldel( col, row, nTrig ); // update CalDel

    thr = effvsdac( col, row, dac, stp, nTrig, roc );

    ++ntry;

  } // vthr

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  tunePH( col, row, roc ); // opt PHoffset, PHscale into ADC range

  // S-curves, noise, gain, cal:
  if( Hansen ) {
    tb.SetDAC( CtrlReg, 0 ); // small Vcal
    dacval[roc][CtrlReg] = 0;
    cout << "[S-curves]" << endl;
    dacscanroc( 25, 25, 1, 127 ); // dac nTrig step stop
  }
  // cals:

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  dacdac( col, row, 26, 12, 1 ); // sets CalDel and VthrComp for cals

  if( Hansen ) {
    cout << "[Hansen Test]" << endl;
    dacscanroc( 25, -25, 2, 255 ); // for bump height
  }
  cout << "[Bump Bond Test]" << endl;
  dacscanroc( 12, -10, 1, 255 ); // cals, scan VthrComp, bump bond test

  tb.SetDAC( CtrlReg, ctl ); // restore
  dacval[roc][CtrlReg] = ctl;
  tb.Flush();

  // write all dac values in a file with name from the root-file

  string globalName = gROOT->GetFile()->GetName(); 
  int sizeName = globalName.size();
  string prefixName = globalName.substr( 0, sizeName-5 );

  writedac( prefixName );

  return ok;
}

//------------------------------------------------------------------------------
void cmdHelp()
{
  fputs(
	"+-cmd commands --------------+\n"
	"| help    list of commands   |\n"
	"| exit    exit commander     |\n"
	"| quit    quit commander     |\n"
	"+----------------------------+\n"
	, stdout );
}

//------------------------------------------------------------------------------
CMD_PROC( h )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  cmdHelp();
  return true;
}

//------------------------------------------------------------------------------
void cmd() // called once from psi46test
{
  CMD_REG( scan,      0, "scan                          enumerate USB devices" );
  CMD_REG( open,      0, "open <name>                   open USB connection to DTB" );
  CMD_REG( boardid,   0, "boardid                       get board ID" );
  CMD_REG( welcome,   0, "welcome                       blinks with LEDs" );
  CMD_REG( setled,    0, "setled <bits>                 set atb LEDs" );
  CMD_REG( close,     0, "close                         close USB connection to DTB" );

  CMD_REG( upgrade,   0, "upgrade <filename>            upgrade DTB firmware" );

  CMD_REG( init,      0, "init                          inits the testboard" );
  CMD_REG( flush,     0, "flush                         send USB command buffer to DTB" );
  CMD_REG( clear,     0, "clear                         clears USB data buffer" );

  // DTB:

  CMD_REG( udelay,    1, "udelay <us>                   waits <us> microseconds" );
  CMD_REG( mdelay,    1, "mdelay <ms>                   waits <ms> milliseconds" );

  CMD_REG( d1,        1, "d1 <signal>                   assign signal to D1 output" );
  CMD_REG( d2,        1, "d2 <signal>                   assign signal to D2 outout" );
  CMD_REG( a1,        1, "a1 <signal>                   assign analog signal to A1 output" );
  CMD_REG( a2,        1, "a2 <signal>                   assign analog signal to A2 outout" );
  CMD_REG( probeadc,  1, "probeadc <signal>             assign analog signal to ADC" );

  CMD_REG( clksrc,    1, "clksrc <source>               Select clock source" );
  CMD_REG( clkok,     1, "clkok                         Check if ext clock is present" );
  CMD_REG( fsel,      1, "fsel <freqdiv>                clock frequency select" );
  CMD_REG( stretch,   1, "stretch <src> <delay> <width> stretch clock" );

  CMD_REG( deser160,  1, "deser160                      align deser160" );
  CMD_REG( deser160,  1, "deserext                      scan deser160 edxt trig" );
  CMD_REG( deser,     1, "deser value                   controls deser160" );

  CMD_REG( clk,       1, "clk <delay>                   clk delay" );
  CMD_REG( ctr,       1, "ctr <delay>                   ctr delay" );
  CMD_REG( sda,       1, "sda <delay>                   sda delay" );
  CMD_REG( tin,       1, "tin <delay>                   tin delay" );
  CMD_REG( rda,       1, "rda <delay>                   tin delay" );
  CMD_REG( clklvl,    1, "clklvl <level>                clk signal level" );
  CMD_REG( ctrlvl,    1, "ctrlvl <level>                ctr signel level" );
  CMD_REG( sdalvl,    1, "sdalvl <level>                sda signel level" );
  CMD_REG( tinlvl,    1, "tinlvl <level>                tin signel level" );
  CMD_REG( clkmode,   1, "clkmode <mode>                clk mode" );
  CMD_REG( ctrmode,   1, "ctrmode <mode>                ctr mode" );
  CMD_REG( sdamode,   1, "sdamode <mode>                sda mode" );
  CMD_REG( tinmode,   1, "tinmode <mode>                tin mode" );

  CMD_REG( showclk,   1, "showclk                       show CLK signal" );
  CMD_REG( showctr,   1, "showctr                       show CTR signal" );
  CMD_REG( showsda,   1, "showsda                       show SDA signal" );

  CMD_REG( sigoffset, 1, "sigoffset <offset>            output signal offset" );
  CMD_REG( lvds,      1, "lvds                          LVDS inputs" );
  CMD_REG( lcds,      1, "lcds                          LCDS inputs" );

  CMD_REG( pgset,     1, "pgset <addr> <bits> <delay>   set pattern generator entry" );
  CMD_REG( pgstop,    1, "pgstop                        stops pattern generator" );
  CMD_REG( pgsingle,  1, "pgsingle                      send single pattern" );
  CMD_REG( pgtrig,    1, "pgtrig                        enable external pattern trigger" );
  CMD_REG( pgloop,    1, "pgloop <period>               start patterngenerator in loop mode" );
  CMD_REG( trigdel,   1, "trigdel <delay>               delay in trigger loop [BC]" );

  CMD_REG( dopen,     1, "dopen <buffer size> [<ch>]    Open DAQ and allocate memory" );
  CMD_REG( dsel,      1, "dsel <MHz>                    select deserializer 160 or 400 MHz" );
  CMD_REG( dreset,    1, "dreset <reset>                DESER400 reset 1, 2, or 3" );
  CMD_REG( dclose,    1, "dclose [<channel>]            Close DAQ" );
  CMD_REG( dstart,    1, "dstart [<channel>]            Enable DAQ" );
  CMD_REG( dstop,     1, "dstop [<channel>]             Disable DAQ" );
  CMD_REG( dsize,     1, "dsize [<channel>]             Show DAQ buffer fill state" );
  CMD_REG( dread,     1, "dread                         Read Daq buffer and show as ROC data" );
  CMD_REG( dreadm,    1, "dreadm [channel]              Read Daq buffer and show as module data" );

  // power and bias:

  CMD_REG( pon,       2, "pon                           switch ROC power on" );
  CMD_REG( poff,      2, "poff                          switch ROC power off" );
  CMD_REG( va,        2, "va <mV>                       set VA supply in mV" );
  CMD_REG( vd,        2, "vd <mV>                       set VD supply in mV" );
  CMD_REG( ia,        2, "ia <mA>                       set IA limit in mA" );
  CMD_REG( id,        2, "id <mA>                       set ID limit in mA" );

  CMD_REG( getva,     2, "getva                         set VA in V" );
  CMD_REG( getvd,     2, "getvd                         set VD in V" );
  CMD_REG( getia,     2, "getia                         set IA in mA" );
  CMD_REG( getid,     2, "getid                         set ID in mA" );
  CMD_REG( optia,     2, "optia <ia>                    set Vana to desired ROC Ia [mA]" );
  CMD_REG( optiamod,  2, "optiamod <ia>                 set Vana to desired ROC Ia [mA] for module" );

  CMD_REG( hvon,      2, "hvon                          switch HV on" );
  CMD_REG( hvoff,     2, "hvoff                         switch HV off" );
  CMD_REG( vb,        2, "vb <V>                        set -Vbias in V" );
  CMD_REG( getvb,     2, "getvb                         measure bias voltage" );
  CMD_REG( getib,     2, "getib                         measure bias current" );
  CMD_REG( scanvb,    2, "scanvb vmax [vstp]            bias voltage scan" );
  CMD_REG( ibvst,     2, "ibvst                         bias current vs time, any key to stop" );

  // ROC:

  CMD_REG( chip,      3, "chip num                      set chip number" );
  CMD_REG( reson,     3, "reson                         activate reset" );
  CMD_REG( resoff,    3, "resoff                        deactivate reset" );
  CMD_REG( rocaddr,   3, "rocaddr                       set ROC address" );
  CMD_REG( rowinvert, 3, "rowinvert                     invert row address psi46dig" );

  CMD_REG( readback,  3, "readback                      read out ROC data" );

  CMD_REG( dac,       3, "dac address value [roc]       set DAC" );
  CMD_REG( vdig,      3, "vdig value                    set Vdig" );
  CMD_REG( vana,      3, "vana value                    set Vana" );
  CMD_REG( vtrim,     3, "vtrim value                   set Vtrim" );
  CMD_REG( vthr,      3, "vthr value                    set VthrComp" );
  CMD_REG( subvthr,   3, "subvthr value                 subtract from VthrComp" );
  CMD_REG( vcal,      3, "vcal value                    set Vcal" );
  CMD_REG( ctl,       3, "ctl value                     set control register" );
  CMD_REG( wbc,       3, "wbc value                     set WBC" );
  CMD_REG( wdac,      3, "wdac [description]            write dacParameters_chip_desc.dat" );
  CMD_REG( rddac,     3, "rddac [description]           read dacParameters_chip_desc.dat" );
  CMD_REG( wtrim,     3, "wtrim [description]           write trimParameters_chip_desc.dat" );
  CMD_REG( rdtrim,    3, "rdtrim [description]          read trimParameters_chip_desc.dat" );

  CMD_REG( cole,      3, "cole <range>                  enable column" );
  CMD_REG( cold,      3, "cold <range>                  disable columns" );
  CMD_REG( pixi,      3, "pixi roc col row              show trim bits" );
  CMD_REG( pixt,      3, "pixt roc col row trim         set trim bits" );
  CMD_REG( pixe,      3, "pixe <range> <range> <value>  trim pixel" );
  CMD_REG( pixd,      3, "pixd <range> <range>          mask pixel" );
  CMD_REG( cal,       3, "cal <range> <range>           enable calibrate pixel" );
  CMD_REG( arm,       3, "arm <range> <range>           activate pixel" );
  CMD_REG( cals,      3, "cals <range> <range>          sensor calibrate pixel" );
  CMD_REG( cald,      3, "cald                          clear calibrate" );
  CMD_REG( mask,      3, "mask                          mask all pixel and cols" );
  CMD_REG( fire,      3, "fire col row [nTrig]          single pulse and read" );
  CMD_REG( fire2,     3, "fire col row [-][nTrig]       correlation" );

  CMD_REG( daci,      3, "daci dac                      current vs dac" );
  CMD_REG( vanaroc,   3, "vanaroc                       ROC efficiency scan vs Vana" );
  CMD_REG( vthrcompi, 3, "vthrcompi roc                 Id vs VthrComp for one ROC" );
  CMD_REG( caldel,    3, "caldel col row                CalDel efficiency scan" );
  CMD_REG( caldelmap, 3, "caldelmap                     map of CalDel range" );
  CMD_REG( caldelroc, 3, "caldelroc                     ROC CalDel efficiency scan" );

  CMD_REG( takedata,  3, "takedata period               readout 40 MHz/period (stop: s enter)" );
  CMD_REG( tdscan,    3, "tdscan vmin vmax              take data vs VthrComp" );
  CMD_REG( onevst,    3, "onevst col row period         <PH> vs time (stop: s enter)" );

  CMD_REG( vthrcomp5, 3, "vthrcomp5 target [guess]      set VthrComp to target Vcal from 5" );
  CMD_REG( vthrcomp,  3, "vthrcomp target [guess]       set VthrComp to target Vcal from all" );
  CMD_REG( trim,      3, "trim target [guess]           set Vtrim and trim bits" );
  CMD_REG( trimbits,  3, "trimbits                      set trim bits for efficiency" );
  CMD_REG( thrdac,    3, "thrdac col row dac            Threshold vs DAC one pixel" );
  CMD_REG( thrmap,    3, "thrmap guess                  threshold map trimmed" );
  CMD_REG( thrmapsc,  3, "thrmapsc stop (4=cals)        threshold map" );
  CMD_REG( scanvthr,  3, "scanvthr vthrmin vthrmax [vthrstp]    threshold RMS vs VthrComp" );

  CMD_REG( effdac,    3, "effdac col row dac [stp] [nTrig] [roc]  Efficiency vs DAC one pixel" );
  CMD_REG( phdac,     3, "phdac col row dac [stp] [nTrig] [roc]  PH vs DAC one pixel" );
  CMD_REG( gaindac,   3, "gaindac                       calibrated PH vs Vcal" );
  CMD_REG( calsdac,   3, "calsdac col row dac [nTrig] [roc]  cals vs DAC one pixel" );
  CMD_REG( dacdac,    3, "dacdac col row dacx dacy [cals] DAC DAC scan" );

  CMD_REG( dacscanroc,3, "dacscanroc dac [-][nTrig] [stp]  PH vs DAC, all pixels" );

  CMD_REG( tune,      3, "tune col row                  tune gain and offset" );
  CMD_REG( phmap,     3, "phmap nTrig                   ROC PH map" );
  CMD_REG( calsmap,   3, "calsmap nTrig                 CALS map = bump bond test" );
  CMD_REG( effmap,    3, "effmap nTrig                  pixel alive map" );
  CMD_REG( effmask,   3, "effmask nTrig                 pixel alive map - all pixels are masked-" );
  CMD_REG( bbtest,    3, "bbtest nTrig                  CALS map = bump bond test" );
  CMD_REG( oneroc,    3, "oneroc                        single ROC test" );
  CMD_REG( bare,      3, "bare [Hansen]                 bare module ROC test" );

  // module:

  CMD_REG( module,    4, "module num                    set module number" );
  CMD_REG( tbmsel,    4, "tbmsel <hub> <port>           set hub and port address, port 6=all" );
  CMD_REG( modsel,    4, "modsel <hub>                  set hub address for module" );
  CMD_REG( tbmset,    4, "tbmset <reg> <value>          set TBM register" );
  CMD_REG( tbmget,    4, "tbmget <reg>                  read TBM register" );
  CMD_REG( tbmgetraw, 4, "tbmgetraw <reg>               read TBM register" );
  CMD_REG( tbmdis,    4, "tbmdis                        disable TBM" );
  CMD_REG( select,    4, "select addr:range             set i2c address" );

  CMD_REG( dselmod,   4, "dselmod                       select deser400 for DAQ channel 0" );
  CMD_REG( dmodres,   4, "dmodres                       reset all deser400" );
  CMD_REG( dselroca,  4, "dselroca value                select adc for channel 0" );
  CMD_REG( dseloff,   4, "dseloff                       deselect all" );

  CMD_REG( modcaldel, 4, "modcaldel col row             CalDel efficiency scan for module" );
  CMD_REG( modpixsc,  4, "modpixsc col row ntrig        module pixel S-curve" );
  CMD_REG( dacscanmod,4, "dacscanmod dac [-][ntrig] [step] [stop] module dac scan" );
  CMD_REG( modthrdac, 4, "modthrdac col row dac         Threshold vs DAC one pixel" );
  CMD_REG( modvthrcomp, 4, "modvthrcomp target            set VthrComp on each ROC" );
  CMD_REG( modtrim,   4, "modtrim target                set Vtrim and trim bits" );
  CMD_REG( modtrimbits, 4, "modtrimbits                   adjust weak trim bits" );
  CMD_REG( modtune,   4, "modtune col row [roc]         tune PH gain and offset" );
  CMD_REG( modmap,    4, "modmap nTrig                  module map" );
  CMD_REG( modthrmap, 4, "modthrmap [new] [cut]         module threshold map" );

  CMD_REG( modtd,     4, "modtd period                  module take data 40MHz/period (stop: s enter)" );

  // info:

  CMD_REG( log,       5, "log <text>                    writes text to log file" );
  CMD_REG( version,   5, "version                       shows DTB software version" );
  CMD_REG( info,      5, "info                          shows detailed DTB info" );
  CMD_REG( rpcinfo,   5, "rpcinfo                       lists DTB functions" );
  CMD_REG( showtb,    5, "showtb                        print DTB state" );
  CMD_REG( showhv,    5, "showhv                        status of iseg HV supply" );
  CMD_REG( show,      5, "show [roc]                    print dacs" );
  CMD_REG( show1,     5, "show1 dac                     print one dac for all ROCs" );
  CMD_REG( upd,       5, "upd <histo>                   re-draw ROOT histo in canvas" );

  cmdHelp();

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
	modcnt[roc][col][row] = 0;
	modamp[roc][col][row] = 0;
      }

  ierror = 0;

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

  //gStyle->SetPalette( 1 ); // rainbow colors, red on top

  // ROOT View colors Color Wheel:

  int pal[20];

  pal[ 0] = 1;
  pal[ 1] = 922; // gray
  pal[ 2] = 803; // brown
  pal[ 3] = 634; // dark red
  pal[ 4] = 632; // red
  pal[ 5] = 810; // orange
  pal[ 6] = 625;
  pal[ 7] = 895; // dark pink
  pal[ 8] = 614;
  pal[ 9] = 609;
  pal[10] = 616; // magenta
  pal[11] = 593;
  pal[12] = 600; // blue
  pal[13] = 428;
  pal[14] = 423;
  pal[15] = 400; // yellow
  pal[16] = 391;
  pal[17] = 830;
  pal[18] = 416; // green
  pal[19] = 418; // dark green on top

  gStyle->SetPalette( 20, pal );

  gStyle->SetHistMinimumZero(); // no zero suppression

  gStyle->SetOptDate();

  cout << "open ROOT window..." << endl;
  MyMainFrame *myMF = new MyMainFrame( gClient->GetRoot(), 800, 600 );

  myMF->SetWMPosition( 99, 0 );

  cout << "open Canvas..." << endl;
  c1 = myMF->GetCanvas();

  c1->SetBottomMargin( 0.15 );
  c1->SetLeftMargin( 0.15 );
  c1->SetRightMargin( 0.18 );

  gPad->Update(); // required

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

} // cmd
