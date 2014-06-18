//
// Daniel Pitzl, Jun 2013, Jan 2014, May 2014
// minimize Weibull function to pixel PH vs large and small Vcal
// decorrelated p0, p4
// start values for digital chip
// nlopt
//
// root -l phroc-c247-trim30.root
// from ~/psi/dtb/tst215/cmd.cpp dacscanroc 25
// TH2D
//
// For Linux:
// gSystem->Load("/usr/local/lib/libnlopt.so")
// For Mac:
// gSystem->Load("/usr/local/lib/libnlopt.dylib")
// .files (shows libs)
// .L phroc2ps.C++
// .x phroc2ps.C+

#include "TDirectory.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TH1.h"
#include "TH2D.h"
#include "TMath.h"
#include "TF1.h"
#include "TProfile2D.h"
#include "TKey.h"
#include "TObject.h"
//#include "TLegend.h"
#include "TCanvas.h"
#include "TSystem.h"

#include <fstream>
#include <string>
#include <iomanip>

#include <nlopt.h>

double G = 1e-5; // x axis scaling

//------------------------------------------------------------------------------
double Weibull( double *x, const double *par )
{
  // Weibull distribution

  double t = par[0] + G*x[0]/par[5] / par[1]; // t near 1, decorrelate p0

  if( t < 0 ) {
    cout << "negative t " << t << " at " << x[0]*par[5] << endl;
    t = -t;
  }

  double l = log(t);
  double p = par[2];
  double a = exp( p*l ); // t^p
  double c = exp(-a);
  double f = par[4] - par[3]*c; // sign changed!

  return f; // Weib = p4 - p3*exp(-t^p2), t = p0 + x/p1
}

//------------------------------------------------------------------------------
double dWeibdx( double *x, const double *par )
{
  // Weibull derivative for large Vcal (par[5] = 1):

  double t = par[0] + G*x[0] / par[1]; // t near 1, decorrelate p0
  if( t < 0 ) {
    cout << "negative t " << t << " at " << x[0]*par[5] << endl;
    t = -t;
  }
  double l = log(t);
  double p = par[2];
  double a = exp( l*p ); // t^p
  double b = exp( l*(p-1) ); // t^(p-1)
  return par[3] * exp(-a) * p * b * G/par[1]; // sign changed
}

//------------------------------------------------------------------------------
double d2Weibdx2( double *x, const double *par )
{
  // Weibull 2nd derivative:

  double s = par[1];
  double t = par[0] + G*x[0] / s;
  if( t < 0 ) {
    cout << "negative t " << t << " at " << x[0]*par[5] << endl;
    t = -t;
  }
  double p = par[2];
  double l = log(fabs(t));
  double a = exp( p*l ); // t^p
  double d = exp( (p-2)*l ); // t^(p-2)

  return -par[3] * p *G/s *G/s * exp(-a) * d * ( p-1 - p*a ); // sign changed
}

//------------------------------------------------------------------------------
double dWeibdp( double *x, const double *par, double *der )
{
  // Weibull derivative w.r.t. par

  double t = par[0] + G*x[0] / par[1]; // t near 1, decorrelate p0

  if( t < 0 ) {
    cout << "negative t " << t << " at " << x[0]*par[5] << endl;
    t = -t;
  }

  double l = log(t); // small, ln(1) = 0
  double p = par[2];
  double a = exp( p*l ); // t^p
  double b = exp( (p-1)*l ); // t^(p-1)
  double c = exp(-a); // not 1-exp(-a), decorrelates p4
  double f = par[4] - par[3]*c; // sign changed!

  der[4] = 1;
  der[3] =-c;
  der[2] = par[3] * c * a * l; // dWeib/dp2
  der[1] =-par[3] * c * p * b * G*x[0]/par[5]/par[1]/par[1]; // dWeib/dp1
  der[0] = par[3] * c * p * b; // dWeib/dp0
  der[5] =-par[3] * c * p * b * G*x[0]/par[5]/par[5]/par[1]; // dWeib/r

  return f; // Weib
}

//------------------------------------------------------------------------------

typedef struct {
  int Ndata;
  int Ndata2;
  int flag;
  int Ncall;
  double chisq4;
  double chisq0;
  double *xx;
  double *yy;
  double *x2;
  double *y2;
} data2_struct;

//------------------------------------------------------------------------------
// function to be minimized:

double FCN( unsigned int npar, const double *par, double *grd, void * mydata )
{
  data2_struct * d = (data2_struct *) mydata;

  bool ldb = 0;
  if( d->flag == 5 ) ldb = 1;
  if( ldb ) {
    cout << d->Ncall << ":";
    for( unsigned int ip = 0; ip < npar; ++ip ) cout << "  " << par[ip];
    cout << ", " << d->Ndata << " + " << d->Ndata2 << " data points"
	 << endl;
  }

  double chisq = 0;

  if( grd != NULL )
    for( unsigned int j = 0; j < npar; ++j ) grd[j] = 0;

  // large Vcal:

  double xar[npar];
  for( unsigned int j = 0; j < npar; ++j ) xar[j] = par[j];
  xar[5] = 1; // don't re-scale

  double der[npar];

  for( int ii = 0; ii < d->Ndata; ++ii ) {

    double x = d->xx[ii];
    double y = d->yy[ii];
    if( y < 0.5 ) continue;

    double f = Weibull( &x, xar );

    double ey = 2;
    double r = y - f; // resid
    double c = r/ey;
    chisq += c*c;

    if( grd != NULL ) {
      double df = dWeibdp( &x, xar, der );
      for( unsigned int ip = 0; ip < npar-1; ++ip ) // last par is 1 for large Vcal
	grd[ip] -= 2*c/ey * der[ip]; // Gradient
    }

  } // bins ii

  d->chisq4 = chisq;

  // small Vcal: par

  for( int ii = 0; ii < d->Ndata2; ++ii ) {

    double x = d->x2[ii];
    double y = d->y2[ii];
    if( y < 0.5 ) continue;

    double f = Weibull( &x, par );

    double ey = 1;
    double r = y - f; // resid
    double c = r/ey;
    chisq += c*c;

    if( grd != NULL ) {
      double df = dWeibdp( &x, par, der );
      for( unsigned int ip = 0; ip < npar; ++ip ) // all par
	grd[ip] -= 2*c/ey * der[ip]; // Gradient
    }

  } // bins ii

  d->chisq0 = chisq - d->chisq4;

  if( ldb ) {
    if( grd != NULL )
      for( unsigned int ip = 0; ip < npar; ++ip ) cout << "  " << grd[ip];
    cout << ", chisq " << chisq << endl;
  }

  d->Ncall++;

  return chisq;

} // FCN

//----------------------------------------------------------------------
void phroc2ps( )
{
  using namespace std;

  gROOT->Time();

  // set styles:

  gStyle->SetTextFont(62); // 62 = Helvetica bold LaTeX
  gStyle->SetTextAlign(11);

  gStyle->SetTickLength( -0.02, "x" ); // tick marks outside
  gStyle->SetTickLength( -0.02, "y" );
  gStyle->SetTickLength( -0.02, "z" );

  gStyle->SetLabelOffset( 0.022, "x" );
  gStyle->SetLabelOffset( 0.022, "y" );
  gStyle->SetLabelOffset( 0.022, "z" );
  gStyle->SetLabelFont( 62, "X" );
  gStyle->SetLabelFont( 62, "Y" );
  gStyle->SetLabelFont( 62, "Z" );

  gStyle->SetTitleOffset( 1.3, "x" );
  gStyle->SetTitleOffset( 2.0, "y" );
  gStyle->SetTitleOffset( 1.9, "z" );
  gStyle->SetTitleFont( 62, "X" );
  gStyle->SetTitleFont( 62, "Y" );
  gStyle->SetTitleFont( 62, "Z" );

  gStyle->SetTitleBorderSize(0); // no frame around global title
  gStyle->SetTitleX( 0.20 ); // global title
  gStyle->SetTitleY( 0.98 ); // global title
  gStyle->SetTitleAlign(13); // 13 = left top align

  gStyle->SetLineWidth(1);// frames
  gStyle->SetHistLineColor(4); // 4=blau
  gStyle->SetHistLineWidth(3);
  gStyle->SetHistFillColor(5); // 5 = gelb
  //  gStyle->SetHistFillStyle(4050); // 4050 = half transparent
  gStyle->SetHistFillStyle(1001); // 1001 = solid

  gStyle->SetFrameLineWidth(2);

  // statistics box:

  gStyle->SetOptStat(11);
  gStyle->SetStatFormat( "8.6g" ); // more digits, default is 6.4g
  gStyle->SetStatFont(42); // 42 = Helvetica normal
  //  gStyle->SetStatFont(62); // 62 = Helvetica bold
  gStyle->SetStatBorderSize(1); // no 'shadow'

  gStyle->SetStatX(0.80); // cvsq
  gStyle->SetStatY(0.90);

  gStyle->SetPalette(1); // rainbow colors

  gStyle->SetHistMinimumZero(); // no zero suppression

  gStyle->SetOptDate();

  gROOT->ForceStyle();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // book new histos:

  TH1D* hnpt = new TH1D( "hnpt", "data points;data points;pixels", 512, 0.5, 512.5 );
  TH1D* hiter = new TH1D( "hiter", "fit iterations;fit iterations;pixels", 100, 0, 1000 );
  TH1D* hchisq = new TH1D( "hchisq", "chisq/ndof;chisq/ndof;pixels", 100, 0, 10 );

  TH1D* hmax = new TH1D( "hmax", "Amax;Amax [ADC];pixels", 256, 0, 256 );

  TH1D* hoff = new TH1D( "hoff", "Weibull PH at 0;Weibull PH at 0 [ADC];pixels", 250, 0, 250 );
  TH1D* htop = new TH1D( "htop", "top PH;maximum PH [ADC];pixels", 256, -0.5, 255.5 );

  TH1D* hinf = new TH1D( "hinf", "inflection point;point of maximum gain [large Vcal DACs];pixels", 200, 0, 100 );
  TH1D* hslp = new TH1D( "hslp", "max slope;Weibull max slope [ADC/DAC];pixels", 500, 0, 5 );
  TH1D* hrat = new TH1D( "hrat", "large/small;large/small Vcal;pixels", 100, 6, 8 );

  TProfile2D* h10 = new TProfile2D( "h10", "Weibull PH at 0 map;col;row;Weibull PH at 0 [ADC]", 52, -0.5, 51.5, 80, -0.5, 79.5, -100, 250 );
  TProfile2D* h11 = new TProfile2D( "h11", "top PH map;col;row;maximum PH [ADC]", 52, -0.5, 51.5, 80, -0.5, 79.5, 0, 300 );
  TProfile2D* h12 = new TProfile2D( "h12", "inflection point map;col;row;Weibull inflection point [large Vcal DAC]", 52, -0.5, 51.5, 80, -0.5, 79.5, 0, 250 );
  TProfile2D* h13 = new TProfile2D( "h13", "maximum gain map;col;row;Weibull maximum gain [ADC/large Vcal DAC]", 52, -0.5, 51.5, 80, -0.5, 79.5, 0, 50 );
  TProfile2D* h14 = new TProfile2D( "h14", "ratio map;col;row;large/small Vcal", 52, -0.5, 51.5, 80, -0.5, 79.5, 0, 20 );
  TProfile2D* h15 = new TProfile2D( "h15", "chisq map;col;row;mpfit chisq", 52, -0.5, 51.5, 80, -0.5, 79.5, 0, 99 );

  // open output file:

  ofstream gainfile( "phroc.dat" );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  cout << "pwd: ";
  gDirectory->pwd();

  cout << "gDir " << gDirectory->GetName() << endl;

  gDirectory->ls();

  cout << "keys " << gDirectory->GetNkeys() << endl;

  TH2D * h24 = NULL; // large Vcal (CtrlReg 4)
  TH2D * h20 = NULL; // small Vcal (CtrlReg 0)
  int found = 0;

  TIter nextkey( gDirectory->GetListOfKeys() );

  TKey * key;

  while( ( key = (TKey*)nextkey() ) ) {

    TObject * obj = key->ReadObj();

    if( obj->IsA()->InheritsFrom( "TH2D" ) ) {

      TH2D * h2 = (TH2D*)obj;

      string hn = h2->GetName(); // PH_DAC25_CR4_Vcal222_map

      if( h24 == NULL && hn.substr( 0, 12 ) == "PH_DAC25_CR4" ) {
	h24 = h2;
	found++;
	cout << "Large Vcal: " << hn
	     << " has " << h24->GetNbinsX()
	     << " x " << h24->GetNbinsY() << " bins"
	     << " with " << (int) h24->GetEntries() << " entries"
	     << ", sum " << h24->GetSum()
	     << endl;
      }

      if( h20 == NULL && hn.substr( 0, 12 ) == "PH_DAC25_CR0" ) {
	h20 = h2;
	found++;
	cout << "Small Vcal: " << hn
	     << " has " << h20->GetNbinsX()
	     << " x " << h20->GetNbinsY() << " bins"
	     << " with " << (int) h20->GetEntries() << " entries"
	     << ", sum " << h20->GetSum()
	     << endl;
      }

    } // TH2

  } // while over keys

  if( found != 2 ) {
    cout << "PH_DAC25_CR4, PH_DAC25_CR0 found " << found
	 << ", return"
	 << endl;
    return;
  }

  bool err = 0;
  int npx = 0;

  for( int px = 0; px < h24->GetNbinsX(); ++px ) { // pixels
    //for( int px = 200; px <= 200; ++px ) { // test
    //for( int px = 1559; px <= 1559; ++px ) { // debug
    //for( int px = 3331; px <= 3331; ++px ) { // debug

    npx++;

    double a = 0;
    double b = 0;
    double p = 0;
    double g = 0;
    double v = 0;
    double r = 0;

    int col = px / 80; // 0..51
    int row = px % 80; // 0..79

    cout << endl
	 << "pixel " << setw(2) << col << setw(3) << row << setw(5) << px
	 << endl;

    double phmax = 0;

    const int mdata = 256;
    double x4[mdata] = {0};
    double y4[mdata] = {0};

    int n4 = h24->GetNbinsY();
    if( n4 > mdata ) {
      cout << "h24 too many y bins " << n4 << endl;
      n4 = mdata;
    }

    for( int ii = 0; ii < n4; ++ii ) { // dacs

      double ph = h24->GetBinContent( px+1, ii+1 ); // ROOT bin counting starts at 1
      if( ph > phmax ) phmax = ph;
      x4[ii] = ii;
      y4[ii] = ph;

    }

    hmax->Fill( phmax );

    // valid data?

    if( phmax > 9 ) {

      // fit range:

      int ib0 = 1; // bin 0 is spike
      int ib9 = n4-1; // last bin
      double phprv = y4[ib9];

      for( int ii = ib9; ii > 0; --ii ) { // scan from right to left
	double ph = y4[ii];
	if( ph > 0 && abs( ph - phprv ) < 8 ) // stop at zero or spike
	  ib0 = ii; // overwritten
	else
	  break; // first zero bin from right
	phprv = ph;
      }

      cout << "large Vcal fit range " << setw(3) << ib0
	   << " to " << setw(3) << ib9;

      // shift data vectors left:

      int jj = 0;
      for( int ii = ib0; ii <= ib9; ++ii ) {
	x4[jj] = x4[ii];
	y4[jj] = y4[ii];
	jj++;
      }
      cout << " = " << setw(3) << jj << " points" << endl;

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // low range Vcal:

      double x0[mdata] = {0};
      double y0[mdata] = {0};
      int n0 = h20->GetNbinsY();
      if( n0 > mdata ) {
	cout << "h20 too many y bins " << n0 << endl;
	n0 = mdata;
      }

      for( int ii = 0; ii < n0; ++ii ) { // dacs

	double ph = h20->GetBinContent( px+1, ii+1 ); // ROOT bin counting starts at 1
	x0[ii] = ii;
	y0[ii] = ph;

      }

      int jb0 = 1; // first bin is spike
      int jb9 = n0-1;
      double phprv0 = y0[jb9];

      for( int ii = jb9; ii > 0; --ii ) { // scan from right to left

	double ph = y0[ii];
	if( ph > 0 && abs( ph - phprv0 ) < 4 ) // stop at zero or spike
	  jb0 = ii; // overwritten
	else
	  break; // first zero bin from right
	phprv0 = ph;
      }

      int j2 = 0;
      for( int ii = jb0; ii <= jb9; ++ii ) {
	x0[j2] = x0[ii];
	y0[j2] = y0[ii];
	j2++;
      }
      cout << "small Vcal fit range " << setw(3) << jb0
	   << " to " << setw(3) << jb9
	   << " = " << setw(3) << j2 << " points" << endl;

      hnpt->Fill( jj+j2 );

      data2_struct mydata;
      mydata.Ndata = jj;
      mydata.Ndata2 = j2;
      mydata.Ncall = 0;
      mydata.xx = x4;
      mydata.yy = y4;
      mydata.x2 = x0;
      mydata.y2 = y0;

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // NLopt:

      const int npar = 6;

      if( npx == 1 ) {
	int major, minor, bugfix;
	nlopt_version( &major, &minor, &bugfix );
	cout << "NLOPT " << major << "." << minor << "." << bugfix << endl;
      }

      //nlopt_opt myopt = nlopt_create( NLOPT_GN_DIRECT_L, npar );
      //nlopt_opt myopt = nlopt_create( NLOPT_GN_DIRECT_L_NOSCAL,npar ); // bad

      //nlopt_opt myopt = nlopt_create( NLOPT_GN_CRS2_LM, npar ); // nice, 1'033
      //nlopt_opt myopt = nlopt_create( NLOPT_GN_ISRES, npar ); // nice, 12'067

      // Local, no derivative:

      nlopt_opt myopt = nlopt_create( NLOPT_LN_NELDERMEAD, npar ); // chisq 313
      //nlopt_opt myopt = nlopt_create( NLOPT_LN_SBPLX, npar ); // chisq 336
      //nlopt_opt myopt = nlopt_create( NLOPT_LN_PRAXIS, npar ); // chisq 461

      //nlopt_opt myopt = nlopt_create( NLOPT_LN_COBYLA, npar ); // slow
      //nlopt_opt myopt = nlopt_create( NLOPT_LN_NEWUOA_BOUND, npar ); // poor
      //nlopt_opt myopt = nlopt_create( NLOPT_LN_BOBYQA, npar ); // chisq 313

      // need derivative dF/dpar:

      //nlopt_opt myopt = nlopt_create(   NLOPT_LD_MMA, npar ); // chisq 313, robust!
      //nlopt_opt myopt = nlopt_create( NLOPT_LD_LBFGS, npar ); // chisq 314
      //nlopt_opt myopt = nlopt_create( NLOPT_LD_SLSQP, npar ); // failure
      //nlopt_opt myopt = nlopt_create( NLOPT_LD_TNEWTON_PRECOND_RESTART, npar ); // csq 313

      if( npx == 1 ) {
	cout << "Algorithm " << nlopt_get_algorithm(myopt) << ":" << endl;
	cout << nlopt_algorithm_name( nlopt_get_algorithm(myopt) ) << endl;
      }

      nlopt_set_min_objective( myopt, FCN, &mydata ); // minimize chisq

      nlopt_set_ftol_abs( myopt, 1e-6 ); // convergence

      // set start values from high range:

      double par[npar];
      par[0] = 0.999; // horizontal offset, critical!
      par[1] = 1.97; // width, x scaled
      par[2] = 3394; // power
      par[3] = 1.1*phmax; // critical !
      par[4] = phmax;
      par[5] = 6.7;

      // initial step size:

      double dx[npar];
      dx[0] = 0.01; // shift
      dx[1] = 0.1; // width
      dx[2] = 2.9; // power
      dx[3] = 2.9; // scale
      dx[4] = 2.9; // asymptote
      dx[5] = 0.3; // ratio
      //nlopt_set_initial_step( myopt, dx ); // not needed, better without

      // lower bounds:

      double lb[npar];
      lb[0] = 0.9; // 
      lb[1] = 0.5; // width
      lb[2] = 10; // power
      lb[3] = 10; // scale
      lb[4] = 10; // asymptote
      lb[5] = 1.0; // ratio
      nlopt_set_lower_bounds( myopt, lb );

      // upper bounds:

      double ub[npar];
      ub[0] = 1.0; // !
      ub[1] = 5.0; // 
      ub[2] = 99900; // 
      ub[3] = 2*phmax; // 
      ub[4] = 2*phmax; // 
      ub[5] = 99.0; // ratio
      nlopt_set_upper_bounds( myopt, ub );

      mydata.flag = 0;
      cout << "starting chisq " << FCN( npar, par, 0, &mydata ) << endl;

      double minFCN;
      nlopt_result ret = nlopt_optimize( myopt, par, &minFCN );

      // man nlopt
      // less /usr/local/include/nlopt.h
      /*
	typedef enum {
	NLOPT_FAILURE = -1, // generic failure code
	NLOPT_INVALID_ARGS = -2,
	NLOPT_OUT_OF_MEMORY = -3,
	NLOPT_ROUNDOFF_LIMITED = -4,
	NLOPT_FORCED_STOP = -5,
	NLOPT_SUCCESS = 1, // generic success code
	NLOPT_STOPVAL_REACHED = 2,
	NLOPT_FTOL_REACHED = 3,
	NLOPT_XTOL_REACHED = 4,
	NLOPT_MAXEVAL_REACHED = 5,
	NLOPT_MAXTIME_REACHED = 6
	} nlopt_result;
      */
      cout << "nlopt_result " << ret
	   << " after " << mydata.Ncall << " loops"
	   << endl;
      double chisq = FCN( npar, par, 0, &mydata );
      cout << "chisq = " << mydata.chisq4 << " + " << mydata.chisq0
	   << " = " << chisq << endl;

      for( int j = 0; j < npar; ++j )
	cout << "par[" << j << "] = " << par[j] << endl;

      hiter->Fill( mydata.Ncall );
      int ndf = jj+j2-npar;
      hchisq->Fill( chisq/ndf );
      h15->Fill( col, row, chisq / ndf );

      a = par[0];
      b = par[1];
      p = par[2];
      g = par[3];
      v = par[4];
      r = par[5];

      TF1 *f1 = new TF1( "f1", Weibull, 0, 256, npar );
      for( int i = 0; i < npar-1; ++i ) 
	f1->SetParameter( i, par[i] );
      f1->SetParameter( 5, 1 ); // large Vcal

      double A0 = f1->Eval(0); // amplitude at zero
      double A255 = f1->Eval(255); // amplitude at 255

      // gradient = derivative = slope:

      TF1 *g1 = new TF1( "g1", dWeibdx, 0, 256, npar );
      for( int i = 0; i < npar-1; ++i )
	g1->SetParameter( i,  par[i] );
      g1->SetParameter( 5, 1 ); // large Vcal

      double tmax = pow( (p-1)/p, 1/p ); // zero 2nd deriv
      double xmax = (b * (tmax - a) )/G; // pos of max slope
      double gmax = g1->Eval(xmax); // max slope

      cout << "A0 " << A0
	   << ", A255 " << A255
	   << ", gmax " << gmax
	   << " at " << xmax
	   << endl;

      hoff->Fill( A0 );
      htop->Fill( A255 );
      hinf->Fill( xmax );
      hslp->Fill( gmax );
      hrat->Fill( r );

      h10->Fill( col, row, A0 );
      h11->Fill( col, row, A255 );
      h12->Fill( col, row, xmax );
      h13->Fill( col, row, gmax );
      h14->Fill( col, row, r );

      delete f1;
      delete g1;

    } // phmax > 9

    gainfile << "map";
    gainfile << setw(4) << col;
    gainfile << setw(4) << row;

    gainfile.setf(ios::fixed);
    gainfile.setf(ios::showpoint);

    gainfile << "  ";
    gainfile << setprecision(6);
    gainfile << setw(12) << a;   // horiz

    gainfile << "  ";
    gainfile << setprecision(1);
    gainfile << setw(12) << b/G;     // wid

    gainfile << "  ";
    gainfile << setprecision(1);
    gainfile << setw(12) << p;     // pow

    gainfile << "  ";
    gainfile << setprecision(1);
    gainfile << setw(12) << g;     // gain

    gainfile << "  ";
    gainfile << setprecision(1);
    gainfile << setw(12) << v;  // vert

    gainfile << "  ";
    gainfile << setprecision(3);
    gainfile << setw(12) << r;  // ratio

    gainfile << endl;

  } // while

  if( err ) cout << endl << "Error" << endl << endl;

  gainfile.close();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // square:
  //               topleft x, y, width x, y
  TCanvas c1( "c1", "c1", 585, 246, 863, 837 );

  c1.SetBottomMargin(0.15);
  c1.SetLeftMargin(0.15);
  c1.SetRightMargin(0.20); // for colz

  gPad->Update();// required

  //cout << "PadWidth  " << c1.GetWw() << endl;
  //cout << "PadHeight " << c1.GetWh() << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // create postscript file:

  c1.Print( "phroc.ps[", "Portrait" ); // [ opens file

  gStyle->SetPaperSize( 18, 27 );

  gStyle->SetOptStat(111111);
  gROOT->ForceStyle();

  hnpt->Draw();
  c1.Print( "phroc.ps" );

  hiter->Draw();
  c1.Print( "phroc.ps" );

  hchisq->Draw();
  c1.Print( "phroc.ps" );

  hmax->Draw();
  c1.Print( "phroc.ps" );

  hoff->Draw();
  c1.Print( "phroc.ps" );

  htop->Draw();
  c1.Print( "phroc.ps" );

  hslp->Draw();
  c1.Print( "phroc.ps" );

  hinf->Draw();
  c1.Print( "phroc.ps" );

  hrat->Draw();
  c1.Print( "phroc.ps" );

  gStyle->SetOptStat(10);
  gStyle->SetStatY(0.95);

  h10->Draw( "colz" ); // Weibull at 0
  c1.Print( "phroc.ps" );

  h11->Draw( "colz" ); // phmax
  c1.Print( "phroc.ps" );

  h12->Draw( "colz" ); // inflection point
  c1.Print( "phroc.ps" );

  h13->Draw( "colz" ); // gmax
  c1.Print( "phroc.ps" );

  h14->Draw( "colz" ); // large / small Vcal
  c1.Print( "phroc.ps" );

  // chisq map:

  h15->Draw( "colz" );
  c1.Print( "phroc.ps" );

  if( !err && hmax->GetEntries() > 99 ) {

    for( int ix = 1; ix <= h10->GetNbinsX(); ++ix )
      for( int iy = 1; iy <= h10->GetNbinsY(); ++iy ){
	if( h10->GetBinContent(ix,iy) < 1 || h10->GetBinContent(ix,iy) > 255 )
	  cout << "col " << setw(2) << ix-1
	       << ", row " << setw(2) << iy-1
	       << ", pix " << 80*(ix-1)+iy-1
	       << " Weibull at zero " << h10->GetBinContent(ix,iy)
	       << endl;
	if( h11->GetBinContent(ix,iy) < 5 || h11->GetBinContent(ix,iy) > 255 )
	  cout << "col " << setw(2) << ix-1
	       << ", row " << setw(2) << iy-1
	       << ", pix " << 80*(ix-1)+iy-1
	       << " max PH " << h11->GetBinContent(ix,iy)
	       << endl;
	if( h12->GetBinContent(ix,iy) < 5 || h12->GetBinContent(ix,iy) > 200 )
	  cout << "col " << setw(2) << ix-1
	       << ", row " << setw(2) << iy-1
	       << ", pix " << 80*(ix-1)+iy-1
	       << " inflection point " << h12->GetBinContent(ix,iy)
	       << endl;
	if( h13->GetBinContent(ix,iy) < 0.1 || h13->GetBinContent(ix,iy) > 5 )
	  cout << "col " << setw(2) << ix-1
	       << ", row " << setw(2) << iy-1
	       << ", pix " << 80*(ix-1)+iy-1
	       << " max slope " << h13->GetBinContent(ix,iy)
	       << endl;
	if( h14->GetBinContent(ix,iy) < 1 || h14->GetBinContent(ix,iy) > 10 )
	  cout << "col " << setw(2) << ix-1
	       << ", row " << setw(2) << iy-1
	       << ", pix " << 80*(ix-1)+iy-1
	       << " bad Vcal ratio " << h14->GetBinContent(ix,iy)
	       << endl;
	if( h15->GetBinContent(ix,iy) > 10 )
	  cout << "col " << setw(2) << ix-1
	       << ", row " << setw(2) << iy-1
	       << " bad fit chisq " << h15->GetBinContent(ix,iy)
	       << endl;
      }
  }
  //--------------------------------------------------------------------

  c1.Print( "phroc.ps]" ); // ] closes file

  system( "ps2pdf phroc.ps" );
  system( "rm -f phroc.ps" );
  cout << "acroread phroc.pdf" << endl;

  //delete c1;

}
