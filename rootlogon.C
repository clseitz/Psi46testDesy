void rootlogon(){

  using namespace std;
  TStyle* analysisStyle = new  TStyle("analysisStyle", "Style");
  gROOT->Time();

  // set background to white:
  analysisStyle->SetFillColor(10);
  analysisStyle->SetFrameFillColor(10);
  analysisStyle->SetCanvasColor(10);
  analysisStyle->SetPadColor(10);
  analysisStyle->SetTitleFillColor(0);
  analysisStyle->SetStatColor(10);
  //dont put a colored frame around the plots
  analysisStyle->SetFrameBorderMode(0);
  analysisStyle->SetCanvasBorderMode(0);
  analysisStyle->SetPadBorderMode(0);
  analysisStyle->SetLegendBorderSize(0);

  //
  
  analysisStyle->SetTextFont(62); // 62 = Helvetica bold LaTeX
  analysisStyle->SetTextAlign(11);

  analysisStyle->SetTickLength( -0.02, "x" ); // tick marks outside
  analysisStyle->SetTickLength( -0.02, "y" );
  analysisStyle->SetTickLength( -0.02, "z" );

  analysisStyle->SetLabelOffset( 0.022, "x" );
  analysisStyle->SetLabelOffset( 0.022, "y" );
  analysisStyle->SetLabelOffset( 0.022, "z" );
  analysisStyle->SetLabelFont( 62, "X" );
  analysisStyle->SetLabelFont( 62, "Y" );
  analysisStyle->SetLabelFont( 62, "Z" );

  analysisStyle->SetTitleOffset( 1.3, "x" );
  analysisStyle->SetTitleOffset( 1.5, "y" );
  analysisStyle->SetTitleOffset( 1.9, "z" );
  analysisStyle->SetTitleFont( 62, "X" );
  analysisStyle->SetTitleFont( 62, "Y" );
  analysisStyle->SetTitleFont( 62, "Z" );

  analysisStyle->SetTitleBorderSize(0); // no frame around global title
  analysisStyle->SetTitleX( 0.20 ); // global title
  analysisStyle->SetTitleY( 0.98 ); // global title
  analysisStyle->SetTitleAlign(13); // 13 = left top align

  analysisStyle->SetHistLineColor(4); // 4=blau
  analysisStyle->SetHistLineWidth(2);
  analysisStyle->SetHistFillColor(5); // 5 = gelb
  //  analysisStyle->SetHistFillStyle(4050); // 4050 = half transparent
  analysisStyle->SetHistFillStyle(1001); // 1001 = solid
  analysisStyle->SetFrameLineWidth(1);

  // statistics box:

  analysisStyle->SetOptStat(11);
  analysisStyle->SetStatFormat( "8.6g" ); // more digits, default is 6.4g
  analysisStyle->SetStatFont(42); // 42 = Helvetica normal
  //  analysisStyle->SetStatFont(62); // 62 = Helvetica bold
  analysisStyle->SetStatBorderSize(1); // no 'shadow'

  analysisStyle->SetStatX(0.80); // cvsq
  analysisStyle->SetStatY(0.90);

  analysisStyle->SetPalette(1); // rainbow colors

  analysisStyle->SetHistMinimumZero(); // no zero suppression

  analysisStyle->SetOptDate();

  gROOT->ForceStyle();


}
