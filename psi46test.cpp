/* -------------------------------------------------------------
 *
 *  file:        psi46test.cpp
 *
 *  description: main program for PSI46V2 Wafer tester
 *
 *  author:      Beat Meier
 *  modified:    6.2.2006
 *
 *  rev:
 *
 * -------------------------------------------------------------
 */

#include <string>
#include <vector>
#include <iostream> // cout

#include <TApplication.h>
#include <TGClient.h>
#include <TGFrame.h>
#include <TFile.h>

#include "psi46test.h"

#include "profiler.h"

using namespace std;

// globals:

int nEntry; // counts the chip tests

CTestboard tb;
CSettings settings;  // global settings
CProber prober;
CProtocol Log;

char filename[512]; // log file

//------------------------------------------------------------------------------
void help()
{
  printf( "usage: psi46test a.log\n" );
}

#ifdef ROOT
//------------------------------------------------------------------------------
MyMainFrame::MyMainFrame( const TGWindow *p, UInt_t w, UInt_t h )
  : TGMainFrame(p, w, h)
{
  cout << "MyMainFrame..." << endl;
  // Create a main frame:
  fMain = new TGMainFrame( p, w, h );

  // Create canvas widget:
  fEcanvas = new TRootEmbeddedCanvas( "Ecanvas", fMain, w, h );
  fMain->AddFrame( fEcanvas,
		   new TGLayoutHints( kLHintsExpandX | kLHintsExpandY, 1, 1, 1, 1 ) );

  //TCanvas *c1 = fEcanvas->GetCanvas();

  // Set a name to the main frame:
  fMain->SetWindowName("psi46test");

  // Map all subwindows of main frame:
  fMain->MapSubwindows();

  // Initialize the layout algorithm:
  fMain->Resize(fMain->GetDefaultSize());

  // Map main frame:
  fMain->MapWindow();
}

MyMainFrame::~MyMainFrame() {
  // Clean up used widgets: frames, buttons, layouthints
  fMain->Cleanup();
  delete fMain;
}

TCanvas * MyMainFrame::GetCanvas() {
//TCanvas *c1 = fEcanvas->GetCanvas();
  return( fEcanvas->GetCanvas() );
}
#endif

//------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
  string usbId;
  printf( VERSIONINFO "\n" );

  if( argc != 2) { help(); return 1; }
  strncpy( filename, argv[1], sizeof(filename) );

  // load settings:

  cout << "reading psi46test.ini..." << endl;
  if( !settings.read( "psi46test.ini" ) ) {
    printf( "error reading \"psi46test.ini\"\n" );
    return 2;
  }

  cout << "logging to " << filename << endl;
  if( !Log.open( filename ) ) {
    printf( "log: error creating file\n" );
    return 3;
  }

  for( size_t iroc = 0; iroc < 16; ++iroc )
    for( size_t idac = 0; idac < 256; ++idac )
      dacval[iroc][idac] = -1; // DP

  // open test board on USB:

  Log.section("DTB");

  try {
    if( !tb.FindDTB(usbId) ) {
      printf( "found DTB %s\n", usbId.c_str() );
    }
    else if( tb.Open( usbId ) ) {
      printf( "\nDTB %s opened\n", usbId.c_str() );
      string info;
      try {
	tb.GetInfo(info);
	printf( "--- DTB info-------------------------------------\n"
		"%s"
		"-------------------------------------------------\n",
		info.c_str() );
	Log.puts( info.c_str() );
	tb.Welcome();
	tb.Flush();
      }
      catch( CRpcError &e ) {
	e.What();
	printf( "ERROR: DTB software version could not be identified, please update it!\n" );
	tb.Close();
	printf( "Connection to Board %s has been cancelled\n", usbId.c_str() );
      }
    }
    else {
      printf( "USB error: %s\n", tb.ConnectionError() );
      printf( "DTB: could not open port to device %s\n", settings.port_tb );
      printf( "Connect testboard and try command 'scan' to find connected devices.\n" );
      printf( "Make sure you have permission to access USB devices.\n" );
    }

    // open wafer prober:

    if( settings.port_prober >= 0 )
      if( !prober.open( settings.port_prober ) ) {
	printf( "Prober: could not open port %i\n", settings.port_prober );
	Log.puts( "Prober: could not open port\n" );
	return 4;
      }

    Log.flush();

    // Beat Meier:

    vector<uint16_t> vx;
    vx.resize(13);
    for (unsigned int i=0; i<vx.size(); i++) vx[i] = i+1000;
    tb.VectorTest(vx, vx);
    printf("vx={");
    for (unsigned int i=0; i<vx.size(); i++) printf(" %i",int(vx[i]));
    printf(" }\n");

#ifdef ROOT
    TFile* histoFile = new TFile( "Test.root", "RECREATE" );

    cout << "ROOT application..." << endl;

    TApplication theApp( "psi46test", &argc, argv );
#endif

    // call command interpreter:

    nEntry = 0;

    cmd();

    cout << "Daq close..." << endl;
    tb.Daq_Close();
    tb.Flush();

    cout << "ROOT close ..." << endl;
    //histoFile->Write();
    histoFile->Close();

    tb.Close();
  }
  catch( CRpcError &e ) {
    e.What();
  }

  return 0;
}
