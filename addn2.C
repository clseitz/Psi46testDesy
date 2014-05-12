//
// D. Pitzl, Feb 2011
// add all 1D and 2D histos from several files
// copy old 1 to new 0
// reopen 0 as 3
// add old 2 and 3 into 4
//
// root -l addn2.C

#include "iomanip.h"
#include "TH1D.h"
#include "TH2D.h"

void addn2() {

  // open existing f1:

  char* fn[99];
  int ni;

  // SR90 maps, mod D003

  ni = -1;
  ni++; fn[ni] = "SR90-map-00a.root";
  ni++; fn[ni] = "SR90-map-00b.root";
  ni++; fn[ni] = "SR90-map-01a.root";
  ni++; fn[ni] = "SR90-map-01b.root";
  ni++; fn[ni] = "SR90-map-02.root";
  ni++; fn[ni] = "SR90-map-03.root";
  ni++; fn[ni] = "SR90-map-05a.root";
  ni++; fn[ni] = "SR90-map-05b.root";
  ni++; fn[ni] = "SR90-map-07a.root";
  ni++; fn[ni] = "SR90-map-07b.root";
  ni++; fn[ni] = "SR90-map-07c.root";
  ni++; fn[ni] = "SR90-map-07d.root";

  // direct X-rays 9.4.2014 module D0003

  ni = -1;
  ni++; fn[ni] = "X-ray-20-modtd40000.root";
  ni++; fn[ni] = "X-ray-20-vthr60-modtd1000.root";
  ni++; fn[ni] = "X-ray-27-vthr60-modtd1000.root";
  ni++; fn[ni] = "X-ray-35-modtd40000.root";
  ni++; fn[ni] = "X-ray-35-vthr60-close-modtd65000.root";
  ni++; fn[ni] = "X-ray-35-vthr60-midpos-0p1mA-modtd10000.root";
  ni++; fn[ni] = "X-ray-35-vthr60-midpos-halfcur-modtd40000.root";
  ni++; fn[ni] = "X-ray-35-vthr60-modtd40000.root";

  int nmax = ni;

  TFile f1(fn[0]);

  if( f1.IsZombie() ) {
    cout << "Error opening " << fn[0] << endl;
    return;
  }
  cout << "opened " << fn[0] << endl;

  //--------------------------------------------------------------------
  // create f0:

  TFile f0("fileA.root", "RECREATE");
  cout << "created ";
  gDirectory->pwd();

  /*
  TFile options:  
  NEW or CREATE   create a new file and open it for writing,
                  if the file already exists the file is
                  not opened.
  RECREATE        create a new file, if the file already
                  exists it will be overwritten.
  UPDATE          open an existing file for writing.
                  if no file exists, it is created.
  READ            open an existing file for reading (default).
  NET             used by derived remote file access
                  classes, not a user callable option
  WEB             used by derived remote http access
                  class, not a user callable option
  "" (default), READ is assumed.
  */

  //--------------------------------------------------------------------
  // copy f1 to f0:

  f1.cd();

  cout << "keys:\n";
  f1.GetListOfKeys()->Print();

  cout << "pwd: ";
  f1.pwd();

  cout << "ls: \n";
  f1.ls();

  // f1 has sub-dir:

  cout << "First: " << f1.GetListOfKeys()->First()->GetName() << endl;
  cout << "First: " << f1.GetListOfKeys()->First()->ClassName() << endl;
  char* dir1 = f1.GetListOfKeys()->First()->GetName();
  cout << "cd to " << dir1 << endl;
  f1.cd( dir1 );
  cout << "we are in ";
  gDirectory->pwd();

  gDirectory->ReadAll(); // load histos

  TList * lst = gDirectory->GetList();
  cout << lst->GetName() << endl;
  cout << lst->GetTitle() << endl;
  cout << "size    " << lst->GetSize() << endl;
  cout << "entries " << lst->GetEntries() << endl;
  cout << "last    " << lst->LastIndex() << endl;

  TIterator *iter = lst->MakeIterator();
  int ii = 0;
  TObject *obj;
  TH1D *h;
  TH1D *h0;
  TH2D *H;
  TH2D *H0;

  while( obj = iter->Next() ){
    ii++;
    cout << setw(4) << ii << ": ";
    cout << obj->ClassName() << " ";
    cout << obj->InheritsFrom("TH1D") << " ";
    cout << obj->GetName() << " \"";
    cout << obj->GetTitle() << "\"";
    cout << endl;
    //    if( obj->ClassName() == "TH1D" ){
    if( obj->InheritsFrom("TH1D") ){
      h = (TH1D*) obj;
      cout << "       1D";
      cout << h->GetNbinsX() << " bins, ";
      cout << h->GetEntries() << " entries, ";
      cout << h->GetSumOfWeights() << " inside, ";
      cout << h->GetBinContent(0) << " under, ";
      cout << h->GetBinContent(h->GetNbinsX()+1) << " over";
      cout << endl;

      f0.cd(); // output file

      //      TH1D* h0 = (TH1D*) h->Clone();
      h0 = h; // copy
      h0->Write(); // write to file f0
      
      f1.cd(); // back to file 1 for the loop
    }
    else{

      if( obj->InheritsFrom("TH2D") ){

	H = (TH2D*) obj;
	cout << "       2D";
	cout << H->GetNbinsX() << " bins, ";
	cout << H->GetEntries() << " entries, ";
	cout << H->GetSumOfWeights() << " inside, ";
	cout << H->GetBinContent(0) << " under, ";
	cout << H->GetBinContent(H->GetNbinsX()+1) << " over";
	cout << endl;
	
	f0.cd(); // output file
	
	H0 = H; // copy
	H0->Write(); // write to file f0
	
	f1.cd(); // back to file 1 for the loop
      }
      else cout << "other class " << obj->ClassName() << endl;
    }
  }
  cout << "copied " << ii << endl;
  cout << "f1 " << f1.GetName() << " close = " << f1.Close() << endl;
    
  f0.cd();
  cout << "we are in ";
  gDirectory->pwd();

  cout << "f0 " << f0.GetName() << " size  = " << f0.GetSize() << endl;

  cout << "f0 " << f0.GetName() << " write = " << f0.Write() << endl;

  cout << "f0 " << f0.GetName() << " size  = " << f0.GetSize() << endl;

  cout << "f0 " << f0.GetName() << " close = " << f0.Close() << endl;

  f0.Delete();

  //--------------------------------------------------------------------
  // list of files 2:

  bool lAB = true;

  for( int nn = 1; nn <= nmax; ++nn ){

    cout << "\n\n";
    cout << "loop " << nn << ": fn = " << fn[nn] << endl;
    cout << "lAB = " << lAB << endl;

    if( lAB ) { // A+2 -> B
      char* fn3 = "fileA.root";
      char* fn4 = "fileB.root";
    }
    else{ // B+2 -> A
      char* fn3 = "fileB.root";
      char* fn4 = "fileA.root";
    }

    // create f4:

    TFile f4( fn4, "recreate" );
    if( f4.IsZombie() ) {
      cout << "Error creating f4\n";
      return;
    }
    cout << "created f4   = " << f4.GetName() << endl;

    // re-open as f3:

    TFile f3( fn3 );
    if( f3.IsZombie() ) {
      cout << "Error opening f3\n";
      return;
    }
    cout << "re-opened f3 = " << f3.GetName() << endl;

    lAB = !lAB;
    cout << "lAB = " << lAB << " for next loop\n";

    cout << "f4 = " << f4.GetName() << endl;
    cout << "f3 = " << f3.GetName() << endl;

    f3.cd();
    gDirectory->ReadAll(); // load histos into f3 memory

    //    cout << "f3 list size = " << gDirectory->GetList()->GetSize() << endl;
    cout << "f3 list size = " << gDirectory->GetList()->GetSize() << endl;

    TFile f2(fn[nn]);
    //    TFile *f2 = new TFile(fn[nn]);

    if( f2.IsZombie() ) {
    //    if( f2 == NULL ) {
      cout << "Error opening " << fn[nn] << endl;
      return;
    }
    cout << "opened " << fn[nn] << endl;

    // f2 has sub-dir:

    f2.cd( f2.GetListOfKeys()->First()->GetName() );
    cout << "we are in ";
    gDirectory->pwd();

    gDirectory->ReadAll(); // load histos into f2 memory

    // loop over f2:

    cout << "f2 list size = " << gDirectory->GetList()->GetSize() << endl;

    int jj = 0;
    TObject *ob2;
    TH1D *h2;
    TH1D *h3;
    TH2D *H2;
    TH2D *H3;

    TIterator *ite2 = gDirectory->GetList()->MakeIterator();

    while( ob2 = ite2->Next() ){

      jj++;

      //      if( jj > 9 ) continue;

      cout << jj << ". ";
      cout << "ob2 is ";
      cout << ob2->GetName() << " ";
      cout << ob2->ClassName() << " ";
      cout << ob2->GetTitle();
      cout << endl;

      if( ob2->InheritsFrom("TH1D") ) {

	h2 = (TH1D*) ob2;
	cout << "h2 " << h2->GetName() << "  " << h2->GetNbinsX() << " bins\n";
	cout << "h2 " << h2->GetName() << "  " << h2->GetEntries() << " entries\n";
	char* hnm2 = h2->GetName();

	// search in f3:

	cout << "search for " << hnm2 << " in f3\n";
	f3.cd();
	cout << "we are in ";
	gDirectory->pwd();
	
	h3 = (TH1D*) gDirectory->GetList()->FindObject(hnm2);
	if( h3 == NULL ) {
	  cout << "h3 is null\n" ;
	  continue;
	}
	
	cout << "found h3 = ";
	cout << h3->GetName() << "  ";
	cout << h3->ClassName() << "  ";
	cout << h3->GetTitle();
	cout << endl;
	cout << "h3  " << h3->GetName() << "  " << h3->GetNbinsX() << " bins\n";
	cout << "h3  " << h3->GetName() << "  " << h3->GetEntries() << " entries\n";

	// add:

	f4.cd();
	cout << "we are in ";
	gDirectory->pwd();
	//TH1D h4 = *h3 + *h2;
	TH1D* h4 = (TH1D*) h3->Clone();
	h4->Add(h2);
	
	cout << "h4  " << h4->GetEntries() << " entries\n";
	cout << "h4  ";
	cout << h4->GetName() << " ";
	cout << h4->ClassName() << " ";
	cout << h4->GetTitle();
	cout << endl;
	cout << "h4 dir " << h4->GetDirectory()->GetName() << endl;
	cout << "f4 size " << f4.GetSize() << endl;

	// back to f2 for next iter:

	f2.cd( f2.GetListOfKeys()->First()->GetName() );

      }//1D

      if( ob2->InheritsFrom("TH2D") ) {

	H2 = (TH2D*) ob2;
	cout << "H2 " << H2->GetName() << "  " << H2->GetNbinsX() << " bins\n";
	cout << "H2 " << H2->GetName() << "  " << H2->GetEntries() << " entries\n";
	char* Hnm2 = H2->GetName();

	// search in f3:

	cout << "search for " << Hnm2 << " in f3\n";
	f3.cd();
	cout << "we are in ";
	gDirectory->pwd();
	
	H3 = (TH2D*) gDirectory->GetList()->FindObject(Hnm2);
	if( H3 == NULL ) {
	  cout << "H3 is null\n" ;
	  continue;
	}
	
	cout << "found H3 = ";
	cout << H3->GetName() << "  ";
	cout << H3->ClassName() << "  ";
	cout << H3->GetTitle();
	cout << endl;
	cout << "H3  " << H3->GetName() << "  " << H3->GetNbinsX() << " bins\n";
	cout << "H3  " << H3->GetName() << "  " << H3->GetEntries() << " entries\n";

	// add:

	f4.cd();
	cout << "we are in ";
	gDirectory->pwd();

	TH2D* H4 = (TH2D*) H3->Clone();
	H4->Add(H2);
	
	cout << "H4  " << H4->GetEntries() << " entries\n";
	cout << "H4  ";
	cout << H4->GetName() << " ";
	cout << H4->ClassName() << " ";
	cout << H4->GetTitle();
	cout << endl;
	cout << "H4 dir " << H4->GetDirectory()->GetName() << endl;
	cout << "f4 size " << f4.GetSize() << endl;

	// back to f2 for next iter:

	f2.cd( f2.GetListOfKeys()->First()->GetName() );
      }//2D

    } //while

    cout << "processed " << jj << endl;
    cout << "f4 " << f4.GetName() << " size " << f4.GetSize() << endl;

    //  cout << "f4 map:\n";
    //  f4.Map();

    cout << "f4 " << f4.GetName() << " write = " << f4.Write() << endl;

    cout << "f4 " << f4.GetName() << " size  = " << f4.GetSize() << endl;
    
  }// loop over files 2

  cout << endl;
  cout << "combined " << nmax + 1 << " files\n";
  cout << "Final file is " << f4.GetName() << endl;

  f2.Close();
  f3.Close();
  f4.Close();

}
