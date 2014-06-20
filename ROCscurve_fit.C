//
// Daniel Pitzl, Jun 2013
// fit S-curve, 3 parametrizations: tanh, errf, Stud-t
// Minuit, binomial max likelihood
//
// root -l scurves-c405-trim30.root
//
//
// .x ROCscurve_fit.C+
//  modified Claudia Seitz Mai 2014

#include "TDirectory.h"
#include "TStyle.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TMath.h"
#include "TMinuit.h"
#include "TF1.h"
#include "TLegend.h"
#include "TCanvas.h"
#include "TSystem.h"
#include "TLatex.h"
#include "TPaveText.h"
#include "TFile.h"
#include "TNtuple.h"
#include "TString.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
//#include <iomanip> // setw

template <class T>
inline std::string to_string (const T& t)
{
        std::stringstream ss;
        ss << t;
        return ss.str();
}

string ftostr_prec (const double& d, const int prec = 1)
{
        std::stringstream ss;
        ss << std::fixed << std::setprecision(prec) << d;
        return ss.str();
}
// global:

const int mdata = 256;
double xx[mdata];
double yy[mdata];
int Ndata = 0;
int Nmax = 0;

int mode = 2;

double lsum = 0;
double chisq = 0;

//------------------------------------------------------------------------------
Double_t logistic( Double_t *x, Double_t *par ) {

  double d = x[0] - par[0];
  double b = par[1]/1.8138; // sigma 1.8138 = pi/sqrt(3)
  double t = d / b;
  double e = 1 / ( 1 + exp(-t) );
  return e;

}

//------------------------------------------------------------------------------
Double_t errf( Double_t *x, Double_t *par ) {

  //--  Mean and width:

  double dx = x[0] - par[0];
  double sm = par[1];
  double t =  dx / (sm*sqrt(2));

  // error function according to Numerical Recipes p163
  // erf(v) = 2/sqrt(pi) int_0^v exp(-u^2) du
  // u = (x-x_m)/s
  // note: integral only from zero
  // note: exp(-u^2), not exp(-0.5*u^2) like in Gaussian

  // better use t = u/sqrt(2) = (x-x_m)/s/sqrt(2)
  // Gaussian distribution function N(s) = 1/sqrt(pi) int_-inf^s exp(-t^2) dt
  // then
  // N(s) = 0.5 ( 1 + erf(s) )

  double aa = 0.5* ( 1 + TMath::Erf(t) );

  return aa;
}

//------------------------------------------------------------------------------
Double_t stud( Double_t *x, Double_t *par ) {

  double t =  ( x[0] - par[0] ) / par[1];
  double nu = par[2];
  double F = TMath::StudentI( t, nu ); // nu is global par, fixeda

  return F;
}

//------------------------------------------------------------------------------
// Minuit expects:

void FCN( Int_t &npar, Double_t *grd, Double_t &f, Double_t *par, Int_t flag )
{
  bool ldb = 0;
  if( flag == 5 ) ldb = 1;
  if( ldb ) cout << endl << "FCN called with flag " << flag << endl;

  // select case using flag
  switch(flag)
  case 1:
    // Initialization
  case 2:
    // Compute derivatives
    // store them in grd
  case 3:
    // after the fit is finished
  default:
    // compute function itself
    {

    lsum = 0;
    chisq = 0;  
      if( ldb ) {
	cout << "par";
	for( int ip = 0; ip < npar; ++ip ) cout << "  " << par[ip];
	cout << endl;
      }
      for( int ii = 0; ii < Ndata; ++ii ) {

	double x = xx[ii];
	double y = yy[ii];
	double e; // efficiency at x, 0 < e < 1
	
	if(      mode == 1 )
	  e = logistic( &x, par );
	else if( mode == 2 )
	  e = errf( &x, par );
	else
	  e = stud( &x, par );

	if( e >= 1 ) e = 1-1E-9;
	if( e <= 0 ) e = 1E-9;

	// -log like for binomial:

	double l = -y*log(e) - (Nmax-y)*log(1-e);
	lsum += l;

	double ey = sqrt( e*(1-e) * Nmax );
	double r = y - Nmax*e; // resid
	double c = r/ey;
	chisq += c*c;

      } // bins ii

      if( ldb ) cout << "lsum " << lsum
		     << ", chisq " << chisq
		     << endl;

      f = lsum;

    } // flag switch

} // FCN

//----------------------------------------------------------------------
void ROCscurve_fit( char* hs="N_DAC25_CR0_Vcal222_map", int kmode = 2 ) {

   Int_t nlines = 0;

  using namespace std;

  double thr = 0;
  double sig = 0;
  double nu = 0;

  double ethr = 0;
  double esig = 0;
  double enu = 0;
  
  

  TFile* _file0;
  cout<<gDirectory->GetName()<<endl;
  TH2 *h2 = (TH2*)gDirectory->Get(hs);
  TCanvas c1( "c1", "c1", 800, 600);
  //  TFile *f = new TFile("basic.root","RECREATE");
  TNtuple *ntuple = new TNtuple("fitTree","data from fits","col:row:thr:ethr:sig:esig:status:chisq");
  for ( int xi = 1; xi <=h2->GetNbinsX(); xi++){
    //get row and column
    int row = (xi-1) % 80;
    int col = (xi - row) / 80;
    cout << "row " << row << " col" << col <<endl; 
    string pname =  "scurve_roc0_row" + to_string(row) + "_col" + to_string(col);
    string ptitle =  "S-curve for ROC 0 row " + to_string(row) + " col " + to_string(col);
    TH1 *h = h2->ProjectionY(pname.c_str(),xi,xi);

    //TProfile *h = (TProfile*)gDirectory->Get(hs);
    
    if( h == NULL ) {
      cout << hs << " does not exist\n";
      return;
    }
    
    cout << h->GetTitle() << endl;
    
    h->SetMarkerStyle(21);
    h->SetMarkerSize(0.8);
    h->SetStats(1);
    gStyle->SetOptFit(101);
    //gStyle->SetOptFit(111); // 111 with errors
    gStyle->SetOptStat(11);

    // find a few bins:
    
    int ib0 = 1;
    int nn = h->GetNbinsX();
    int ib9 = nn;
    
    for( int ii = 1; ii <= nn; ++ii ) {
      
      double m = h->GetBinContent(ii);
      
      if( m > Nmax ) Nmax = m;
      
      if( m > 0 ) ib9 = ii; // overwritten until end
      
      int jj = nn - ii + 1; // from right to left
      if( h->GetBinContent(jj) > 0 ) ib0 = jj; // overwritten until begin
      
    } // ii
    
    if( Nmax < 9 ) {
      cout << "Nmax " << Nmax << " too small" << endl;
      return;
    }
    cout << "Nmax " << Nmax << endl;
    
    ib0 = ib0 - 4; // add a few zero bins
    if( ib0 < 1 ) ib0 = 1;
    
    double x0 = h->GetBinLowEdge(ib0);
    double x9 = h->GetBinLowEdge(ib9) + h->GetBinWidth(ib9);
    
    cout << "fit from " << x0 << " to " << x9 << endl;
    
    if( ib9-ib0 >= mdata ) {
      cout << "too many bins " << ib9-ib0 << endl;
    ib9 = ib0 + mdata-1;
    }
    
    int jj = 0;
    for( int ii = ib0; ii <= ib9; ++ii ) {
      xx[jj] = h->GetBinCenter(ii);
      yy[jj] = h->GetBinContent(ii);
      jj++;
    }
    Ndata = jj;
    
    double n50 = 0.5*Nmax;
    double x50 = 0;
    
    for( int ii = ib0; ii <= ib9; ++ii ) {
      
      int kk = ii + 1;
      
      if( h->GetBinContent(ii) <= n50 &&
	  h->GetBinContent(kk) >= n50 ) {
	
	double n1 = h->GetBinContent(ii);
	double n2 = h->GetBinContent(kk);
	double x1 = h->GetBinCenter(ii);
	double x2 = h->GetBinCenter(kk);
	double dx = x2 - x1;
	double dn = n2 - n1;
	x50 = x1 + (n50-n1)/dn * dx;
	break;
      }
      
    } // ii
    
    cout << "x50 " << x50 << endl;
    
    mode = kmode;
    
    // set start values:
    
    double par[3];
    int npar = 2;
    par[0] = x50;
    par[1] = 2.2;
    if( mode == 3 ) {
      npar = 3;
      par[2] = 3.3; // nu
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // init:
    
    TMinuit * myMinuit = new TMinuit( npar );
    
    //myMinuit–>SetFitObject( &FCN );
    myMinuit->SetFCN( FCN );
    
    // Vector of step, initial min and max value
    
    int ierflg = 0;
    myMinuit->mnparm( 0, "thr", par[0], 0.2, 0, 0, ierflg );
    myMinuit->mnparm( 1, "sig", par[1], 0.2, 0, 0, ierflg );
    if( mode == 3 )
    myMinuit->mnparm( 2, "nu", par[2], 0.2, 0, 0, ierflg );
    
    // Set Print Level
    // -1 no output
    // 1 standard output
  myMinuit->SetPrintLevel(0);
  // No Warnings
  //myMinuit->mnexcm("SET NOW", arglist ,1,ierflg);
  
  // Set error Definition:
  // 1 for Chi square
  // 0.5 for negative log likelihood
  myMinuit->SetErrorDef(0.5);

  //Minimization strategy:
  // 1 standard
  // 2 try to improve minimum (slower)
  double arglist[10];
  arglist[0] = 2;
  myMinuit->mnexcm( "SET STR", arglist, 1, ierflg );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // call minimizer:

  cout << "migrad:" << endl;
  myMinuit->SetMaxIterations(500);

  myMinuit->Migrad();

  //Get minimum -log likelihood:

  double ln0, edm, errdef;
  int nvpar, nparx, icstat;
  myMinuit->mnstat( ln0, edm, errdef, nvpar, nparx, icstat );

  // errors:

  myMinuit->GetParameter( 0, thr, ethr );
  myMinuit->GetParameter( 1, sig, esig );
  if( mode == 3 )
    myMinuit->GetParameter( 2, nu, enu );

  cout << "thr " << thr << " +- " << ethr << endl;
  cout << "sig " << sig << " +- " << esig << endl;
  if( mode == 3 )
    cout << "nu " << nu << " +- " << enu << endl;

  double grd[3]; // dummy
  double logl;
  par[0] = thr;
  par[1] = sig;
  if( mode == 3 )
    par[2] = nu;
  FCN( npar, grd, logl, par, 5 );

  // minos errors:

  cout << "minos:" << endl;
  myMinuit->mnmnos();

  if( mode == 3 ) {
    double enuup, enudn, gccnu;
    myMinuit->mnerrs( 2, enuup, enudn, enu, gccnu );
    cout << "nu " << nu
	 << " - " << enudn
	 << " + " << enuup
	 << " +- " << enu
	 << " global correlation " << gccnu
	 << endl;    
  }

  //get the minuit status (might add some more strings good = positive, bad = negative
  int status = 0;
  TString fcstatus( (myMinuit->fCstatu).Strip(TString::kTrailing,' ') );
  if ( fcstatus == "SUCCESSFUL" ) status = 1;
  else if ( fcstatus == "CONVERGED" ) status = 2;
  else if (fcstatus == "FAILURE" ) status = -1;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // create postscript file:

  gStyle->SetPaperSize( 18, 27 );
  gPad->Print( "minscurve.ps[", "Landscape" ); // [ opens file
  
  // draw FCN around minimum:

  double ex = ethr;
  double dx = 0.1*ex;
  double xmin = thr-3*ex;
  double xmax = thr+3*ex;
  int nb = (xmax-xmin)/dx+1;
  TProfile *pl0 =
    new TProfile( "pl0", "-log likelihood vs thr;thr;#Delta(-log L)",
		  nb, xmin-0.5*dx, xmax+0.5*dx, 0, 1E9 );

  for( double x = xmin; x < xmax+0.5*dx; x += dx ) {
    par[0] = x;
    FCN( npar, grd, logl, par, 4 );
    pl0->Fill( x, logl-ln0 );
  }
  pl0->SetStats(0);
  string ltitle =  "-log likelihood vs thr for ROC 0 row " + to_string(row) + " col " + to_string(col);
  pl0->SetTitle( ltitle.c_str() );
  pl0->Draw();
  gPad->Print( "minscurve.ps" );

  if( mode == 3 ) {
    ex = enu;
    dx = 0.1*ex;
    xmin = nu-3*ex;
    xmax = nu+3*ex;
    nb = (xmax-xmin)/dx+1;
    TProfile *pl2 =
      new TProfile( "pl2", "-log likelihood vs nu;nu;#Delta(-log L)",
		    61, xmin-0.5*dx, xmax+0.5*dx, 0, 1E9 );

    par[0] = thr;
    par[1] = sig;
    if( mode == 3 )
      par[2] = nu;

    for( double x = xmin; x < xmax+0.5*dx; x += dx ) {
      par[2] = x;
      FCN( npar, grd, logl, par, 4 );
      pl2->Fill( x, logl-ln0 );
    }
    pl2->SetStats(0);
    pl2->SetMaximum(5);
    pl2->Draw();
    gPad->Print( "minscurve.ps" );
  }

  // draw fit function:

  TF1 *fa;
  if( mode == 1 ) 
    fa = new TF1( "fa", "[2]/(1+exp(-(x-[0])/[1]*1.8138));Vcal_[DAC];valid_readouts", x0, x9 );
  else if( mode == 2 ) 
    fa = new TF1( "fa", "[2]*0.5* ( 1 + TMath::Erf( (x-[0]) / (  [1]*sqrt(2) ) ) );Vcal_[DAC];valid_readouts", x0, x9 );
  else
    fa = new TF1( "fa", "[2]*TMath::StudentI( (x-[0])/[1], [3]);Vcal_[DAC];valid_readouts", x0, x9 );

  fa->SetParameter( 0, thr );
  fa->SetParameter( 1, sig );
  fa->SetParameter( 2, Nmax );
  if( mode == 3 )
    fa->SetParameter( 3, nu );

  fa->SetParName( 0, "thr" );
  fa->SetParName( 1, "sigma" );
  fa->SetParName( 2, "N" );
  if( mode == 3 )
    fa->SetParName( 3, "nu" );
  fa->SetLineColor(7);
  fa->SetLineWidth(3);
  h->GetListOfFunctions()->Add(fa);
  
  fa->SetTitle( ptitle.c_str());
  fa->Draw(); // draw function underneath
  fa->GetYaxis()->SetTitle("Number of responses");
  fa->GetXaxis()->SetTitle("Vcal [DAC]");
  // set binomial errors for plotting:

  par[0] = thr;
  par[1] = sig;
  if( mode == 3 )
    par[2] = nu;

  for( int ii = ib0; ii <= ib9; ++ii ) {

    double x = h->GetBinCenter(ii);
    double eps = 0.5;
    if( mode == 1 )
      eps = logistic( &x, par );
    else if( mode == 2 )
      eps = errf( &x, par );
    else
      eps = stud( &x, par );

    double err = sqrt( Nmax * eps * (1-eps) );
    h->SetBinError( ii, err );

  }
  TPaveText *pt = new TPaveText(150.794,2.925353,250.7547,6.64087,"br");
  pt->SetFillColor(0);
  pt->SetFillStyle(0);
  pt->SetTextAlign(12);
  pt->SetTextFont(42);
  pt->SetTextSize(0.04075235);
  pt->SetShadowColor(0);
  string fname;
  if( mode == 1 ) fname = "Logistic function";
  else if( mode == 2 ) fname = "Error function";
  else fname = "Student-T function";
  pt->AddText( ("Fit: "+fname).c_str() );
  pt->AddText( ("Threshold: " + ftostr_prec(thr,2) + " #pm "+ ftostr_prec(ethr,2)).c_str() );
  pt->AddText( ("Sigma: " +ftostr_prec (sig,2) + " #pm " +ftostr_prec (esig,2)).c_str() );
  pt->Draw("same");
  
  h->Draw( "histepsame" ); // draw hist errors polymarker on-top
  //  gPad->SaveAs( "one.pdf" );// causes double prints o_O
  //fill tree
  ntuple->Fill(col,row,thr,ethr,sig,esig,status,chisq);

  // done:


  //gPad->SaveAs( (pname + ".pdf").c_str() );
  delete myMinuit;
  delete pl0;
  //delete fa;

  }
  gPad->Print( "minscurve.ps]" ); 
  system( "ps2pdf minscurve.ps" );
  //system( "rm -f minscurve.ps" );
  string filename(gDirectory->GetName());
  ntuple->SaveAs(("fit_results_"+filename).c_str());
}
