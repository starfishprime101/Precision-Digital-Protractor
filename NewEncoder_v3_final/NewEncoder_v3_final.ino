
//----------------------------------------------------------
//This code is a simplified version of the excellent program
//  by Tom Herbst (www.http://facetingbook.com/). Tom's books are 
//  highly recommended.
//  Much of his functionality has been removed here for simplicity
//  and the code has been adapted to drive a wavehare 1.5" colour 
//  OLED display
//  The code is specifically written for a US Digital E6 incremental encoder
//    with 3600 cpr using quadrature encoding to deliver an angular resolution
//      of 1/40 degree. This should be easily adapted to other similar encoders.
//
//
//  Any errors are my own....
//  Ian Robinson  www.starfishprime.co.uk
//                www.precisioncutgems.co.uk
//----------------------------------------------------------

///New Encoder v3 Final   October 2020


//================================================
//set up the screen for the Waveshare 1.5inch OLED
//================================================
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>


#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>


// -=--- set up display pins
#define DC_PIN   8
#define RST_PIN  9
#define CS_PIN   10
#define MOSI_PIN 11
#define SCLK_PIN 13

#define Rotation 1  //set to 0,1,2 to control screen rotation

// Color definitions
const uint16_t  OLED_Color_Black        = 0x0000;
const uint16_t  OLED_Color_Blue         = 0x001F;
const uint16_t  OLED_Color_Red          = 0xF800;
const uint16_t  OLED_Color_Green        = 0x07E0;
const uint16_t  OLED_Color_Cyan         = 0x07FF;
const uint16_t  OLED_Color_Magenta      = 0xF81F;
const uint16_t  OLED_Color_Yellow       = 0xFFE0;
const uint16_t  OLED_Color_White        = 0xFFFF;

uint16_t        Angle_Text_Color = OLED_Color_Yellow ;
uint16_t        encPos_Text_Color = OLED_Color_Green ;
uint16_t        OLED_Backround_Color = OLED_Color_Black;

// Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

int angleX = 5;          //  X-Location of the angle display line
int angleY = 40;           //  Y-Location of the angle display line
int encPosX = 70;         //  X-Location of the encoder value display line
int encPosY = 125;        //  Y-Location of the encoder value display line



GFXcanvas1 canvas1(128, 128); // 128x32 pixel canvas

Adafruit_SSD1351 oled = 
  Adafruit_SSD1351(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    CS_PIN,
    DC_PIN, RST_PIN
  );

//-----------------------------------


//================================================
//  ----  Set up Rotary Encoder
//================================================

// A on pin 2 (Interrupt)
// B on pin 3 (Interrupt)
// Index on pin 1
// NOTE: The value enc0 is the angle
//       for the index pulse location of the encoder.
//       suggest having the index at approx 45 degrees above horizontal
//       the arm has to be raised to this at start to trigger display

#define pinI 4   //this recieves the index mark signal
#define pinA 2   // 2-3 are the interrupt-enabled inputs
#define pinB 3

volatile int encPos;    // This is the current encoder count
volatile int encPosOld; // previous encoder count - used to clear display

int enc0 = 2000;       // Encoder counts at index pulse
//================================================
//  ----  enc90 needs to be measured and set here
// this is the encoder pulse reading when the arm is horizontal. You will need 
// to have the system installed. Set the faceter arm to 90 degree and read off the encoder count
//  from the display .
int enc90 = 3748;
//================================================

float theAng = 0.000;                    // The cutting angle in degrees

boolean Aset;                    // Whether we just had a positive going edge on A
boolean Bset;                    //   and on B. These keep track of where we are
boolean newCal;                  // This gets set when we have a new calibration (index pulse)
boolean newPos;                  // This gets set when position changes

float degPerStep = 0.025;        // The number of degrees per encoder step (US Digital E6 Device, 3600 c.p.r.)


//================================================

//==== Sampling Rate Stuff ====

unsigned long cur = 0;      // Current time in milliseconds - to control display rewrite
unsigned long last = 0;     // Last sample time in milliseconds
unsigned long rate = 100;   // ms update time for display

//================================================
// this code only runs once
//================================================
void setup()
{
  //

  // Encoder stuff
  pinMode(pinA, INPUT);      // Define these as digital inputs
  pinMode(pinB, INPUT);
  pinMode(pinI, INPUT);

  attachInterrupt(0, doA, CHANGE);     // Interrupt 0: react to RISING/FALLING on pinA
  attachInterrupt(1, doB, CHANGE);     // Interrupt 1: react to RISING/FALLING on pinB

  interrupts();              // Enabled by default, but just in case


  //============================   Draw Boot Screen  =======================================

  oled.begin();
  oled.setRotation(0);  //allowed value 0,1,2,3 -- 3-places connector at bottom
  oled.fillScreen(OLED_Color_Yellow);
  delay(500);
  oled.fillRect(0, 0, 128, 128, OLED_Color_Black);
  oled.println("Ian Robinson");
  oled.println("Encoder Software v4.0");
  oled.println("Booting");
  oled.print(".");
  delay(100);
  oled.print(".");
  delay(100);
  oled.print(".");
  delay(100);
  oled.print(".");
  delay(100);
  oled.print(".");
  delay (500);
  oled.setFont(&FreeMono9pt7b);
  oled.fillScreen(OLED_Color_Black);
  oled.setCursor(0, 50);
  oled.setTextColor(OLED_Color_Red);
  oled.println("Index req");
  encPos = 0;
  encPosOld = encPos;

}


void loop() {

  cur = millis();          // Grab number of milliseconds since startup
  if (cur > last + rate) { // We have waited long enough - update
    
    last = cur;                                // Reset last to current for next check


  //========================================================================================
  // Check for a new calibration and flash Screen if it happened
  //========================================================================================
  if (newCal) {
    oled.fillScreen(OLED_Color_Red);
    newCal = false;
    oled.fillScreen(OLED_Backround_Color);
  }
  //----------------------------------------------------------------------------------------

  if (newPos) {

    // Now print out encoder and angle info;
    theAng = 90 - ((encPos - enc90) * degPerStep); // The actual angle in degrees

    if (theAng > 99.9) {
      theAng = 99.9;
    }

    oled.fillRect(0, 0, 128, 128, OLED_Backround_Color);
    oled.setFont(&FreeMono24pt7b);
    oled.setCursor(angleX, angleY);
    oled.setTextColor(Angle_Text_Color, OLED_Backround_Color);
    oled.println(theAng, 3);
    
    oled.setFont(&FreeMono9pt7b);
    oled.setCursor(encPosX, encPosY);
    oled.setTextColor(encPos_Text_Color, OLED_Backround_Color);
    oled.println(encPos);
    
    newPos = false;
  }
  }
}
//================end main loop

 







//===================================== INTERRUPT SERVICE =====================================

// These routines are called when a voltage transition, either upgoing or downgoing, on line A
// or B of the rotary encoder triggers an interrupt.
// We first check whether the index pulse has also happened. If it has, we restore Aset and Bset to
// their initial values and set encPos to the measured reference value. If an index pulse did not
// occur, we update the status of the line by setting Aset in doA() or Bset in doB().
// We then check the status of the other line (i.e. check Bset in doA() and Aset in doB()).
// The direction of the voltage transition, coupled with the last measured state of the other line,
// lets us determine which way the encoder has moved (see text for an explanation). We then
// either increment or decrement encPos and return control to the loop() routine.

void doA() {     // CHANGE transition on pinA - either upgoing or downgoing

  if (digitalRead(pinI) == HIGH) {  // Check for index pulse
    Aset = false;                   // Reset these to have the same starting condition
    Bset = false;
    newCal = true;                  // A new calibration has happened - Set flag
    newPos = true;
    encPos = enc0;                  // Reset encoder counts to reference value
  }

  else {                              // No Index. Change on A - check which type and react

    if (digitalRead(pinA) == HIGH) {  // UP going transition on A
      Aset = true;                    // We just had an upgoing transition
      newPos = true;
      if (Bset)
      {
        encPos = encPos + 1; // Increment
      }
      else
      {
        encPos = encPos - 1; // Decrement
      }
    }

    else {                            // DOWN going transition on A
      Aset = false;                   // We just had a falling transition
      newPos = true;
      if (Bset)
      {
        encPos = encPos - 1; // Increment
      }
      else
      {
        encPos = encPos + 1; // decrement
      }

    }
  }
}

//======= B Interrupt ======
//
// See explanation in the doA() routine
//
void doB() {     // CHANGE transition on pinB

  if (digitalRead(pinI) == HIGH) {  // Check for index bit
    Aset = false;                   // Reset these to the same starting condition
    Bset = false;
    newCal = true;                  // Set flag
    newPos = true;
    encPos = enc0;                  // Reset
  }

  else {                              // Change on B - check which type and react

    if (digitalRead(pinB) == HIGH) {  // UP going transition
      Bset = true;                    // We just had an upgoing transition
      newPos = true;
      if (Aset)
      {
        encPos = encPos - 1; // Increment
      }
      else
      {
        encPos = encPos + 1; // decrement
      }
    }

    else {                            // DOWN going transition
      Bset = false;                   // We just had a falling transition
      newPos = true;
      if (Aset)
      {
        encPos = encPos + 1; // Increment
      }
      else
      {
        encPos = encPos - 1; // decrement
      }

    }
  }
}

//===================================== That's all Folks =====================================
