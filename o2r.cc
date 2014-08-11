
// Daniel Pitzl (DESY) Aug 2014
// .out to .root

// o2r -c 205 run5.out

#include <stdlib.h> // atoi
#include <iostream> // cout
#include <iomanip> // setw
#include <string> // strings
#include <sstream> // stringstream
#include <fstream> // files
#include <vector>
#include <cmath>

#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TRandom1.h>

using namespace std;

#include "pixelForReadout.h" //struct pixel, struct cluster

// globals:

bool haveGain = 0;

double p0[52][80];
double p1[52][80];
double p2[52][80];
double p3[52][80];
double p4[52][80];
double p5[52][80];

pixel pb[4160]; // global declaration: vector of pixels with hit
int fNHit; // global

//------------------------------------------------------------------------------
// inverse decorrelated Weibull PH -> large Vcal DAC
double PHtoVcal( double ph, int col, int row )
{
  if( !haveGain )
    return ph;

  if( col < 0 )
    return ph;
  if( col > 51 )
    return ph;
  if( row < 0 )
    return ph;
  if( row > 79 )
    return ph;

  // phroc2ps decorrelated: ph = p4 - p3*exp(-t^p2), t = p0 + q/p1

  double Ared = ph - p4[col][row]; // p4 is asymptotic maximum

  if( Ared >= 0 ) {
    Ared = -0.1; // avoid overflow
  }

  double a3 = p3[col][row]; // positive

  // large Vcal = ( (-ln(-(A-p4)/p3))^1/p2 - p0 )*p1

  double vc =
    p1[col][row] * ( pow( -log( -Ared / a3 ), 1 / p2[col][row] ) - p0[col][row] );

  if( vc > 999 )
    cout << "overflow " << vc << " at"
	 << setw( 3 ) << col
	 << setw( 3 ) << row << ", Ared " << Ared << ", a3 " << a3 << endl;

  //return vc * p5[col][row]; // small Vcal
  return vc; // large Vcal
}

// ----------------------------------------------------------------------
vector<cluster> getClus()
{
  // returns clusters with local coordinates
  // decodePixels should have been called before to fill pixel buffer pb 
  // simple clusterization
  // cluster search radius fCluCut ( allows fCluCut-1 empty pixels)

  const int fCluCut = 1; // clustering: 1 = no gap (15.7.2012)

  vector<cluster> v;
  if( fNHit == 0 ) return v;

  int* gone = new int[fNHit];

  for( int i = 0; i < fNHit; ++i )
    gone[i] = 0;

  int seed = 0;

  while( seed < fNHit ) {

    // start a new cluster

    cluster c;
    c.vpix.push_back( pb[seed] );
    gone[seed] = 1;

    // let it grow as much as possible:

    int growing;
    do{
      growing = 0;
      for( int i = 0; i < fNHit; ++i ) {
        if( !gone[i] ){ // unused pixel
          for( unsigned int p = 0; p < c.vpix.size(); p++ ) { // vpix in cluster so far
            int dr = c.vpix.at(p).row - pb[i].row;
            int dc = c.vpix.at(p).col - pb[i].col;
            if( (   dr>=-fCluCut) && (dr<=fCluCut) 
		&& (dc>=-fCluCut) && (dc<=fCluCut) ) {
              c.vpix.push_back(pb[i]);
	      gone[i] = 1;
              growing = 1;
              break; // important!
            }
          } // loop over vpix
        } // not gone
      } // loop over all pix
    }
    while( growing );

    // added all I could. determine position and append it to the list of clusters:

    c.sumA = 0;
    c.charge = 0;
    c.size = 0;
    c.col = 0;
    c.row = 0;
    //c.xy[0] = 0;
    //c.xy[1] = 0;
    double sumQ = 0;

    for( vector<pixel>::iterator p = c.vpix.begin();  p != c.vpix.end();  p++ ) {
      c.sumA += p->ana; // Aout
      double Qpix = p->anaVcal; // calibrated [ke]
      if( Qpix < 0 ) Qpix = 1; // DP 1.7.2012
      c.charge += Qpix;
      //if( Qpix > 20 ) Qpix = 20; // DP 25.8.2013, cut tail. tiny improv
      sumQ += Qpix;
      c.col += (*p).col*Qpix;
      c.row += (*p).row*Qpix;
      //c.xy[0] += (*p).xy[0]*Qpix;
      //c.xy[1] += (*p).xy[1]*Qpix;
    }

    c.size = c.vpix.size();

    //cout << "(cluster with " << c.vpix.size() << " pixels)" << endl;

    if( ! c.charge == 0 ) {
      c.col /= sumQ;
      c.row /= sumQ;
      //c.col /= c.charge;
      //c.row /= c.charge;
      //c.xy[0] /= c.charge;
      //c.xy[1] /= c.charge;
    }
    else {
      c.col = (*c.vpix.begin()).col;
      c.row = (*c.vpix.begin()).row;
      //c.xy[0] = (*c.vpix.begin()).xy[0];
      //c.xy[1] = (*c.vpix.begin()).xy[1];
      cout << "GetClus: cluster with zero charge" << endl;
    }

    v.push_back(c); // add cluster to vector

    // look for a new seed = used pixel:

    while( (++seed < fNHit) && gone[seed] );

  } // while over seeds

  // nothing left,  return clusters

  delete gone;
  return v;
}

//------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
  cout << "main " << argv[0] << " called with " << argc << " arguments" << endl;

  if( argc == 1 ) {
    cout << "give file name" << endl;
    return 1;
  }

  // file name = last argument:

  string evFileName( argv[argc-1] );

  cout << "try to open  " << evFileName;

  ifstream evFile( argv[argc-1] );

  if( !evFile ) {
    cout << " : failed " << endl;
    return 2;
  }

  cout << " : succeed " << endl;

  // further arguments:

  int chip = 205;

  for( int i = 1; i < argc; i++ ) {

    if( !strcmp( argv[i], "-c" ) )
      chip = atoi( argv[++i] );

  } // argc

  // gain:

  string gainFileName;

  if( chip == 205 )
    gainFileName = "/home/pitzl/psi/dtb/tst303/c205-gaincal-trim36.dat"; // 6.8.2014

  double ke = 0.350; // large Vcal -> ke

  if( gainFileName.length(  ) > 0 ) {

    ifstream gainFile( gainFileName.c_str() );

    if( gainFile ) {

      haveGain = 1;
      cout << "gain: " << gainFileName << endl;

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

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // (re-)create root file:

  TFile* histoFile = new TFile( "run.root", "RECREATE" );

  // book histos:

  TH1D hnpx( "npx", "pixel per event;pixels;events", 51, -0.5,  50.5 );
  TH1D hcol( "px_col", "pixel col;column;pixels", 52, -0.5, 51.5 );
  TH1D hrow( "px_row", "pixel row;row;pixels", 80, -0.5, 79.5 );
  TH2D hmap( "px_map", "pixel map;column;row;pixels",
	     52, -0.5, 51.5, 80, -0.5, 79.5 );
  TH1D hq( "px_charge", "pixel charge;pixel charge [ke];pixels",
	     100, 0, 20 );

  TH1D hncl( "ncl", "cluster per event;cluster;events", 21, -0.5,  20.5 );

  TH1D hsiz( "cl_size", "cluster size;pixels/cluster;cluster",
	     31, -0.5, 30.5 );
  TH1D hchg( "cl_charge", "cluster charge;cluster charge [ke];clusters",
	     200, 0, 100 );
  TH1D hchg1( "cl_charge1", "cluster charge Loch;cluster charge [ke];clusters",
	     200, 0, 100 );
  TH1D hchg2( "cl_charge2", "cluster charge Blende;cluster charge [ke];clusters",
	     200, 0, 100 );
  TProfile2D
    sizevsxy( "cl_size_map",
	      "pixels/cluster;cluster col;cluster row;<pixels/cluster>",
	      52, -0.5, 51.5, 80, -0.5, 79.5, 0, 99 );
  TProfile2D
    chrgvsxy( "cl_chrg_map",
	      "cluster charge;cluster col;cluster row;<cluster charge> [ke]",
	      52, -0.5, 51.5, 80, -0.5, 79.5, 0, 999 );
  TH2D
    lowmap( "low_chrg_map",
	      "small cluster charge;cluster col;cluster row",
	      52, -0.5, 51.5, 80, -0.5, 79.5 );
  TProfile
    sizevsdd( "cl_size_scan",
	      "pixels/cluster;center distance [mm];cluster row;<pixels/cluster>",
	      50, 0, 6, 0, 99 );
  TProfile
    chrgvsdd( "cl_chrg_scan",
	      "cluster charge;center distance [mm];cluster row;<cluster charge> [ke]",
	      50, 0, 6, 0, 999 );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // event loop:

  bool ldb = 0;
  int nev = 0;
  int mpx = 0;
  int iev = 0;
  int col = 0;
  int row = 0;
  int adc = 0;

  // Read file by lines

  string sl;

  while( evFile.good() && ! evFile.eof() ) {

    getline( evFile, sl ); // read one line into string
    istringstream is( sl ); // tokenize string

    is >> iev;

    ++nev;
    int npx = 0;
    do {
      is >> col;
      is >> row;
      is >> adc;
      double q = PHtoVcal( adc, col, row ) * ke; // [ke]
      hcol.Fill( col );
      hrow.Fill( row );
      hmap.Fill( col, row );
      hq.Fill( q );
      // fill pixel block for clustering:
      pb[npx].col = col;
      pb[npx].row = row;
      pb[npx].ana = adc;
      pb[npx].anaVcal = q;
      ++npx;
      ++mpx;
    }
    while( ! is.eof() );

    hnpx.Fill( npx );

    fNHit = npx; // for cluster search

    vector<cluster> clust = getClus();	    

    hncl.Fill( clust.size() );

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // look at clusters:

    for( vector<cluster>::iterator c = clust.begin(); c != clust.end(); c++ ) {

      hsiz.Fill( c->size );
      hchg.Fill( c->charge );
      sizevsxy.Fill( c->col, c->row, c->size );
      chrgvsxy.Fill( c->col, c->row, c->charge );

      if( c->charge < 15 )
	lowmap.Fill( c->col, c->row );

      // Lochblende:

      double dx = 0.15 * (c->col - 26); // [mm]
      double dy = 0.10 * (c->row - 43); // [mm]
      double dd = sqrt( dx*dx + dy*dy );

      sizevsdd.Fill( dd, c->size );
      chrgvsdd.Fill( dd, c->charge );

      if( dd < 1.5 ) {
	hchg1.Fill( c->charge );
      }
      else
	hchg2.Fill( c->charge );

      // pix in clus:

    } // clusters

    if( ldb ) cout << endl << "event " << nev << endl;
    if( !(nev%1000) ) cout << "event " << nev << endl;

  } // while events

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  cout << "eof" << endl;
  cout << "events " << nev
       << ", pixel " << mpx
       << endl;

  histoFile->Write();
  histoFile->Close();

  return 0;
}
