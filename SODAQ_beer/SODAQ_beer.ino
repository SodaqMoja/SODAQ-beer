#include <OneWire.h>
#include <Wire.h>
#include <SeeedOLED.h>
#include <GPRSbee.h>


// Fill in your mobile number here. Start with + and country code
#define TELNO           "+999999998"

#define GPRSBEE_PWRPIN 7
#define XBEECTS_PIN    8
#define ONEWIRE_PIN    2


OneWire ds(ONEWIRE_PIN);

bool sent_sms;  // we only want to send an sms once

//############ forward declare #################
// This is only needed to make avr-eclipse happy
float OWtemp(void);

void setup()            /*----( SETUP: RUNS ONCE )----*/
{
  Serial.begin(9600);      // Serial1 is connected to SIM900 GPRSbee
  gprsbee.init(Serial, XBEECTS_PIN, GPRSBEE_PWRPIN);
  gprsbee.off();
  Wire.begin();
  SeeedOled.init();                  //initialize SEEED OLED display
  SeeedOled.clearDisplay();          //clear the screen and set start position to top left corner
  SeeedOled.setNormalDisplay();      //Set display to normal mode (i.e non-inverse mode)
  SeeedOled.setPageMode();           //Set addressing mode to Page Mode
}                       /*--(end setup )---*/

void loop()             /*----( LOOP: RUNS CONSTANTLY )----*/
{
  SeeedOled.setTextXY(0,0);          //Set the cursor to Xth Page, Yth Column
  SeeedOled.putString("Beer(oC): "); //Print the String
  float beertemp = OWtemp();
  SeeedOled.putFloat(beertemp,1);
  if (beertemp <= -100) {
    // Probably something's wrong with the sensor
    return;
  }
  if (beertemp < 8 && !sent_sms) {
      sent_sms = true;               // only send sms once
      SeeedOled.setTextXY(4,0);
      SeeedOled.putString("Sending SMS");
      SeeedOled.setTextXY(5,0);
      bool smsSent = gprsbee.sendSMS(TELNO, "The beer is cold");
      SeeedOled.setTextXY(6, 0);
      if (smsSent) {
        SeeedOled.putString("SMS sent OK");
      } else {
        SeeedOled.putString("SMS not sent");
      }
  }
}                       /* --(end main loop )-- */




float OWtemp(void)
{
  byte i;
  byte present;
  byte type_s;
  byte data[12];
  byte addr[8] = "";
  float celsius;

  ds.search(addr);
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);                    // start conversion, with parasite power on at the end

  delay(1000);                          // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  if (!present) {
    // No device present
    return -273;
  }
  ds.select(addr);
  ds.write(0xBE);                       // Read Scratchpad

  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      //Serial.println("Device is not a DS18x20 family device.");
      return -273;
  }

  for (i = 0; i < 9; i++) {             // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3;                     // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00)
      raw = raw & ~7;                   // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20)
      raw = raw & ~3;                   // 10 bit res, 187.5 ms
    else if (cfg == 0x40)
      raw = raw & ~1;                   // 11 bit res, 375 ms
    else {
      //// default is 12 bit resolution, 750 ms conversion time
    }
  }
  celsius = (float) raw / 16.0;
  return celsius;
}

/* ( THE END ) */
