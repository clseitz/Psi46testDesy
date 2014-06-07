
#ifndef PSI46TEST_H
#define PSI46TEST_H

#define ROOT

#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <TCanvas.h>

#include "config.h"
#include "pixel_dtb.h"
#include "settings.h"
#include "prober.h"
#include "protocol.h"
#include "pixelmap.h"
#include "test.h"
#include "chipdatabase.h"

#define VERSIONINFO TITLE " " VERSION " (" TIMESTAMP ")" // from config.h

// global variables:
extern int nEntry; // counts the entries in the log file

extern CTestboard tb;
extern CSettings settings;  // global settings
extern CProber prober; // prober
extern CProtocol Log;  // log file

extern CChip g_chipdata;

extern int delayAdjust;
extern int deserAdjust;

void cmd();

#ifdef ROOT

class MyMainFrame : public TGMainFrame {

private:
  TGMainFrame *fMain;
  TRootEmbeddedCanvas *fEcanvas;
public:
  MyMainFrame( const TGWindow *p, UInt_t w, UInt_t h );
  virtual ~MyMainFrame();
  TCanvas * GetCanvas();
};

#endif

#endif
