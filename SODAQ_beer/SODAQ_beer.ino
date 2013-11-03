#include <OneWire.h>
#include <Wire.h>
#include <SeeedOLED.h>
#include <GPRSbee.h>


#define GPRSBEE_PWRPIN 7
#define XBEECTS_PIN 8


OneWire ds(2);  // on pin 2

int sent_sms=0 ;  // we only want to send an sms once
float beertemp; 
bool smsSent;
int line=1;



void setup()   /*----( SETUP: RUNS ONCE )----*/
{
  Serial.begin(9600);      // Serial1 is connected to SIM900 GPRSbee
  gprsbee.init(Serial, XBEECTS_PIN, GPRSBEE_PWRPIN);
  gprsbee.off();
  Wire.begin();
  SeeedOled.init();  //initialze SEEED OLED display
  SeeedOled.clearDisplay();          //clear the screen and set start position to top left corner
  SeeedOled.setNormalDisplay();      //Set display to normal mode (i.e non-inverse mode)
  SeeedOled.setPageMode();           //Set addressing mode to Page Mode
  
}/*--(end setup )---*/
 
void loop()   /*----( LOOP: RUNS CONSTANTLY )----*/
{
  SeeedOled.setTextXY(0,0);          //Set the cursor to Xth Page, Yth Column   
  SeeedOled.putString("Beer(oC): "); //Print the String         
  beertemp = OWtemp(); 
  SeeedOled.putFloat(beertemp,1);
  if (beertemp < 8 && sent_sms==0){
      sent_sms =1;  // only send sms once
      SeeedOled.setTextXY(4,0);         
      SeeedOled.putString("Sending SMS");
      SeeedOled.setTextXY(5,0);
      smsSent = sendSMS("The beer is cold", "+31681517710"); 
      delay(2000);
      gprsbee.off();
      if (smsSent){
        SeeedOled.setTextXY(6,0);         
        SeeedOled.putString("SMS sent OK");
      } else {
        SeeedOled.setTextXY(6,0);         
        SeeedOled.putString("SMS not sent");
      }  
      
  }  
        
  

}/* --(end main loop )-- */
 


float OWtemp(void){

  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8]="";
  float celsius;

  ds.search(addr);
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad


  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();

  }


  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  return celsius;
}


static bool sendSMS(char *message, char *number ){
  char cmdbuf[60];  
  gprsbee.on();
  if (!gprsbee.sendCommandWaitForOK("ATE0")) {
    return false;
  }
  if (!gprsbee.sendCommandWaitForOK("AT+CMGF=1")) {
    return false;
  }    
  
  strcpy(cmdbuf, "AT+CMGS=\"");
  strcat(cmdbuf, number);
  strcat(cmdbuf, "\"");  
  gprsbee.sendCommand(cmdbuf);
  if (!gprsbee.waitForPrompt(">",500)) {
    return false;
  }  
  gprsbee.sendCommand(message);//the message itself
  delay(200);
  Serial.write(26);//the ASCII code of ctrl+z is 26, this is needed to end the send modus and send the message.
  if (!gprsbee.waitForOK(10000)) {
      return false;
  }
  return true;
}
 
/* ( THE END ) */

