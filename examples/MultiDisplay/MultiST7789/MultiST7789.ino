/***************************************************
  Demonstrate use of:
  * /CS control by function rather than pin number
  * multiple ST7789 displays
  * phased initialisation
 ****************************************************/
#include <TFT_eSPI.h> // using Setup404_Teensy_ST7789

// *************** Change to your pin numbers etc. ***************
#define MUX_CS 0
#define MUX_A  1
#define MUX_B  2

#define GBL_RST 22
#define GBL_BLK  3 // active high backlight

#define TFT_ROTATE 3
#define TFT_INVERT true

//========================================================================
/*
 * Use 74LVC138 decoder to provide /CS signal to one of
 * 4 displays, using only 3 Teensy outputs (I only have 
 * 4 displays, but we could address up to 8).
 */
 static void CSfn(int which, bool negate)
{
  if (negate)
    digitalWriteFast(MUX_CS,1);
  else
  {
    digitalWriteFast(MUX_A,(which&1)!=0);
    digitalWriteFast(MUX_B,(which&2)!=0);
    digitalWriteFast(MUX_CS,0);
  }
}

/*
 * Use a lambda function for each display to tell the library
 * how to assert / negate its /CS signal. Hence we can deal
 * with multiple displays with differing wiring fairly easily.
 */
TFT_eSPI tft1 = TFT_eSPI(240, 240, SPI, -1, [](bool negate) { CSfn(3, negate); } );  // Invoke custom library
TFT_eSPI tft2 = TFT_eSPI(240, 240, SPI, -1, [](bool negate) { CSfn(2, negate); } );  
TFT_eSPI tft3 = TFT_eSPI(240, 240, SPI, -1, [](bool negate) { CSfn(1, negate); } );

/*
 * If you have separate pins for each display's /CS, you could do this instead:
 */
/*
#define TFT1_CS  0 // pick ... 
#define TFT2_CS 17 // .. some random ...
#define TFT3_CS 42 // ... pins!

TFT_eSPI tft1 = TFT_eSPI(240, 240, SPI, -1, [](bool negate) { digitalWrite(TFT1_CS, negate); } );
TFT_eSPI tft2 = TFT_eSPI(240, 240, SPI, -1, [](bool negate) { digitalWrite(TFT2_CS, negate); } );  
TFT_eSPI tft3 = TFT_eSPI(240, 240, SPI, -1, [](bool negate) { digitalWrite(TFT3_CS, negate); } );
// or of course
TFT_eSPI tft1 = TFT_eSPI(240, 240, SPI, TFT1_CS);
TFT_eSPI tft2 = TFT_eSPI(240, 240, SPI, TFT2_CS);
TFT_eSPI tft3 = TFT_eSPI(240, 240, SPI, TFT3_CS);
*/

TFT_eSPI* tfts[] = {&tft1, &tft2, &tft3};

/*
 * Negate /CS for all displays
 */
void safeCSpins(void)
{
  pinMode(MUX_A,OUTPUT);
  pinMode(MUX_B,OUTPUT);
  pinMode(MUX_CS,OUTPUT);

  CSfn(0,HIGH);
}

/*
 * Generic N-display interleaved initialisation code
 * Re-use in your sketch!
 */
void initTFTs(TFT_eSPI* tfts[], int n)
{
  int phases[n]{0}; // start them all in phase 0
  bool finished = false;

  while (!finished)
  {
    finished = true;
    for (int i=0;i<n;i++)
    {
      int phase = tfts[i]->phasedInit(0, phases[i]); // dummy tab colour
      phases[i] = phase;
      if (phase >= 0) // at least one display hasn't finished
        finished = false;
    }
    yield(); // do other stuff here, if you want
  }
}

//========================================================================
void setup() {
  safeCSpins();

  /* reset is common, so do it in the sketch */
  pinMode(GBL_RST, OUTPUT);
  digitalWriteFast(GBL_RST,1); delay(1);
  digitalWriteFast(GBL_RST,0); delay(1);
  digitalWriteFast(GBL_RST,1); 
  // no delay() needed, TFT_RST is set to -1 so 
  // a software reset will be done, then the required
  // delay inserted

  while (!Serial && millis() < 5000) ; // wait for Arduino Serial Monitor
  Serial.begin(9600);
  Serial.println("\n\nMulti-display test!");

  /*
   * Phased initialisation of all displays in parallel
   */
  initTFTs(tfts,3);
  
  /*
   * Project-specific further initialisations
   */
  for (int i=0;i<3;i++)
  {
    tfts[i]->setRotation(TFT_ROTATE);
    tfts[i]->invertDisplay(TFT_INVERT);
    tfts[i]->setFreeFont(&FreeSans18pt7b);
  }

  // do our own backlight enable here, after
  // we're sure all screens have initialised
  pinMode(GBL_BLK, OUTPUT);
  digitalWriteFast(GBL_BLK,HIGH);
}


//========================================================================
//const char* txts[]{"TFT1", "TFT 2", "TFT 3"};
const char* txts[]{"Use", "many", "screens!"};
void loop(void) 
{
  for (int i=0;i<3;i++) 
  {
    uint16_t colour = random(65536);
    uint16_t bkgnd  = random(65536);

    tfts[i]->setTextColor(colour, bkgnd);
    tfts[i]->fillScreen(bkgnd);
    tfts[i]->drawString(txts[i],20,20);
  }
  delay(500);
}