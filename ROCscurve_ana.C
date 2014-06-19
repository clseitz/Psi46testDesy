//
// Claudia Seitz, June 2014
// reads in the file/tree produced by ROCscurve_fit.C
// root -l  fit_results_scurves-c405-trim36.root
// .x ROCscurve_ana.C

#define ROCscurve_ana_cxx
#include <TH2.h>
#include <TTree.h>
#include <TFile.h>
#include <TStyle.h>
#include <TCanvas.h>
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

void ROCscurve_ana(){
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

  gStyle->SetOptStat(1111);
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

 
  TTree *t1 = (TTree*) _file0->Get("fitTree");
  
  //read in tree
  Float_t col, row, status, thr, ethr, sig, esig, chisq;
  t1->SetBranchAddress("col",&col);
  t1->SetBranchAddress("row",&row);
  t1->SetBranchAddress("thr",&thr);
  t1->SetBranchAddress("status",&status);
  t1->SetBranchAddress("ethr",&ethr);
  t1->SetBranchAddress("sig",&sig);
  t1->SetBranchAddress("esig",&esig);
  t1->SetBranchAddress("chisq",&chisq);
  
  //create histograms
  TH1F *hthr   = new TH1F("hthr","threshold distribution;threshold;number of pixels",500,0,100);
  TH1F *hsig   = new TH1F("hsig","sigma distribution;sigma;number of pixels",200,0,40);
  TH2F *hthr_map   = new TH2F("hthr_map","threshold map;col;row",52,0,52,80,0,80);
  TH2F *hsig_map   = new TH2F("hsig_map","sigma map;col;row",52,0,52,80,0,80);

  //read entries and fill the histograms
  Int_t nentries = (Int_t)t1->GetEntries();
  for (Int_t i=0; i<nentries; i++) {
    t1->GetEntry(i);
    hthr->Fill(thr);
    hsig->Fill(sig);
    hthr_map->Fill(col, row, thr);
    hsig_map->Fill(col, row, sig);
  }
  //We do not close the file. We want to keep the generated histograms
  //we open a browser and the TreeViewer

  hthr->Draw();
  gPad->Print( "threshold_scurve.ps[" );
  gPad->Print( "threshold_scurve.ps" );
  hsig->Draw();
  gPad->Print( "threshold_scurve.ps" );
  hthr_map->Draw("colz");
  gPad->Print( "threshold_scurve.ps" );
  hsig_map->Draw("colz");
  gPad->Print( "threshold_scurve.ps" );
  gPad->Print( "threshold_scurve.ps]" );

  

  
}

