
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
//
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

void bbtestAnalysis(string rootFileName, int nTrig, string testName) 
{  
  // 
  // Macro to Analyze the histograms coming from 
  // - Bump Bounding
  // or
  // - pixel alive test
  // Input parameters:
  //  - rootFileName (produced by Psi46testDesy)
  //  - nTrig (used in the test)
  //  - testName 
  //    testName = "bump"  for Bump Bounding - bbtest
  //             = "alive" for Pixel Alive Test - effmap 
  // Run as:
  // root [0] .L bbtestAnalysis.C
  // root [1] bbtestAnalysis("Test_s404_noOpt.root",10,"bump");
  // or from the shell
  // root -b -q 'bbtestAnalysis.c("Test_s404_noOpt.root",10,"bump")'

  analysisStyle->cd();
  gROOT->ForceStyle();

  TFile *_file0 = TFile::Open(rootFileName.c_str());
  TH2D *_hist_analyze;
  
  // choose the wished test

  std::size_t foundBump = testName.find("bump");
  std::size_t foundAlive = testName.find("alive");

  //cout << "Bump " << foundBump << endl;
  //cout << "Alive " << foundAlive << endl;

  // Load proper histogram:
  if (foundBump == 0)
    {
      (TH2D*)_hist_analyze = _file0->Get("BBTest");
    }
  else if (foundAlive == 0)
    {
      (TH2D*)_hist_analyze = _file0->Get("AliveMap_Vcal222_CR4");
    }
  else
    {
      cout << testName.c_str() << " is not a valid input parameter" << endl;
    }

  // Looping over the histogram to find 
  // missing bumps or pixels (depending used test )
  // active & inefficient bumps 
  // perfect bumps

  int nbinx = _hist_analyze->GetNbinsX();
  int nbiny = _hist_analyze->GetNbinsY();
  float response;
  response=0;

  int nMissing;
  int nActive;
  int nPerfect;
  int nIneff;

  nMissing = 0;
  nActive = 0;
  nPerfect = 0;

  if (foundBump == 0)
    {
      cout << "Bump Bounding Statistic " << endl;
    }
  else if (foundAlive == 0)
    {
      cout << "Pixel Alive Test Statistic "  << endl;
    }
  
  // Fill a summary histogram
  TH1D *h_sumTrig = new TH1D( "summary", "summary", nTrig+1, -0.5, float(nTrig+0.5) );
  h_sumTrig->GetXaxis()->SetTitle("nTrig");
  h_sumTrig->GetYaxis()->SetTitle("Number of Pixels");

  for ( int ibin=1; ibin <= nbinx ; ibin++) 
    {
      for (int jbin=1; jbin <= nbiny ; jbin++)
        {    
	  response = _hist_analyze->GetBinContent(ibin,jbin);	  
	  h_sumTrig->Fill(response,1.);
	  // missing
	  if (response <= 0.)
	    {
	      nMissing++;
	      cout << "Missing at col, row: " << _hist_analyze->GetXaxis()->GetBinCenter(ibin) << " "  << _hist_analyze->GetYaxis()->GetBinCenter(jbin) << " response: "  << response << endl;
	    }
	  // active 
	  else if (response >= nTrig/2)
	    {
	      nActive++;
	    }
	  else
	    {
	      nIneff++;
	      cout << "Inefficient at col, row: " << _hist_analyze->GetXaxis()->GetBinCenter(ibin) << " "  << _hist_analyze->GetYaxis()->GetBinCenter(jbin) << " response: "  << response << endl;
	    }	  
	  // perfect
	  if (response == nTrig)
	    {
	      nPerfect++;
	    }
	}
    }
  
  if (foundBump == 0)
    {
      cout << "Number of Missing Bumps: " << nMissing << endl;
      cout << "Number of Active Bump: " << nActive << endl;
      cout << "Number of Inefficient Bump: " << nIneff << endl;
      cout << "Number of Perfect Bump: " << nPerfect << endl;
    }
  else if (foundAlive == 0)
    {
      cout << "Number of Missing/Dead Pixel: " << nMissing << endl;
      cout << "Number of Active Pixel: " << nActive << endl;
      cout << "Number of Inefficient Pixel: " << nIneff << endl;
      cout << "Number of Perfect Pixel: " << nPerfect << endl;      
    }

  TCanvas *c = new TCanvas("c","c",1200,400);      
  c->Divide(2,1);
  c->cd(1);
  _hist_analyze->Draw("colz");
  c->cd(2);
  gPad->SetLogy();
  h_sumTrig->Draw();
  //  _hist_analyze->Draw("colz");

  string pdfNameFile;
  pdfNameFile = "test_"+testName+".pdf";
  cout << pdfNameFile.c_str() << endl;
  c->Print(pdfNameFile.c_str());

}
