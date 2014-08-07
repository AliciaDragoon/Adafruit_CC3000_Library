/*************************************************** 
 * This is an example for the DFRobot Wido - Wifi Integrated IoT lite sensor and control node
 * 
 * Designed specifically to work with the DFRobot Wido products:
 * 
 * 
 * The main library is forked from Adafruit
 * 
 * Written by Lauren
 * BSD license, all text above must be included in any redistribution
 * 
 ****************************************************/
/*
This example code is used to connect the Xively cloud service.

The device required is just:

1. LM35 low cost temperature sensor or any device you used to upload data
2. And Wido

*/


#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <avr/wdt.h>

#define Wido_IRQ   7
#define Wido_VBAT  5
#define Wido_CS    10
Adafruit_CC3000 Wido = Adafruit_CC3000(Wido_CS, Wido_IRQ, Wido_VBAT,
SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "myNetwork"           // cannot be longer than 32 characters!
#define WLAN_PASS       "myPassword"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  2000
#define TCP_TIMEOUT      3000

#define CC3000_TINY_DRIVER

#define WEBSITE  "api.xively.com"
#define API_key  "Nm8vxZaYtkCreW9oBL74VIxY93ONHsvNlpizj6QkIM8hxxxx"  // Update Your API Key
#define feedID  "180220xxxx"                                         // Update Your own feedID

void setup(){

  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 

  displayDriverMode();

  /* Initialise the module */
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!Wido.begin())
  {
    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    while(1);
  }

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); 
  Serial.println(ssid);

  /* NOTE: Secure connections are not available in 'Tiny' mode!
   By default connectToAP will retry indefinitely, however you can pass an
   optional maximum number of retries (greater than zero) as the fourth parameter.
   */
  if (!Wido.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }

  Serial.println(F("Connected!"));

  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!Wido.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  
}

uint32_t ip = 0;
float temp = 0;


void loop(){

  static Adafruit_CC3000_Client WidoClient;
  static unsigned long RetryMillis = 0;
  static unsigned long uploadtStamp = 0;
  static unsigned long sensortStamp = 0;

  if(!WidoClient.connected() && millis() - RetryMillis > TCP_TIMEOUT){
    // Update the time stamp
    RetryMillis = millis();


    Serial.println(F("Try to connect the cloud server"));
    WidoClient.close();

    //Get DFRobot IOT Server IP    
    ip = Wido.IP2U32(216,52,233,120);  //uint32_t ip = cc3000.IP2U32(216,52,233,120);
    WidoClient = Wido.connectTCP(ip, 80);
  }
  else if(WidoClient.connected() && millis() - uploadtStamp > 2000){
    uploadtStamp = millis();
    // If the device is connected to the cloud server, upload the data every 2000ms.
    
    wdt_enable(WDTO_8S);  
    // Prepare JSON for Xively & get length
    int length = 0;
    // JSON beginning
    String data_start = "";
    data_start = data_start + "\n" 
      + "{\"version\":\"1.0.0\",\"datastreams\" : [ ";
    // JSON for temperature & humidity
    String data_temperature = "{\"id\" : \"Temperature\",\"current_value\" : \"" 
      + String(int(temp)) + "\"}]}";
    // Get length
    length = data_start.length() + data_temperature.length();
    wdt_reset();
    
    Serial.println(F("Connected to Xively server."));
    
    // Send headers
    Serial.print(F("Sending headers"));
    WidoClient.fastrprint(F("PUT /v2/feeds/"));
    WidoClient.fastrprint(feedID);
    WidoClient.fastrprintln(F(".json HTTP/1.0"));
    Serial.print(F("."));
    WidoClient.fastrprintln(F("Host: api.xively.com"));
    Serial.print(F("."));
    WidoClient.fastrprint(F("X-ApiKey: "));
    WidoClient.fastrprintln(API_key);
    Serial.print(F("."));
    WidoClient.fastrprint(F("Content-Length: "));
    WidoClient.println(length);
    Serial.print(F("."));
    WidoClient.fastrprint(F("Connection: close"));
    Serial.println(F(" done."));
    
    // Reset watchdog
    wdt_reset();
    
    // Send data
    Serial.print(F("Sending data"));
    WidoClient.fastrprintln(F(""));    
    WidoClient.print(data_start);
    Serial.print(F("."));
    wdt_reset();
    WidoClient.print(data_temperature);
    Serial.print(F("."));
    WidoClient.fastrprintln(F(""));
    Serial.println(F(" done."));
    
    // Reset watchdog
    wdt_reset();
    
    /* Get the http page info
    Serial.println(F("Reading answer..."));
    while (WidoClient.connected()) {
      while (WidoClient.available()) {
        char c = WidoClient.read();
        Serial.print(c);
      }
    }
    */
  }

  if(millis() - sensortStamp > 100){
    sensortStamp = millis();
    // read the LM35 sensor value and convert to the degrees every 100ms.
    
    int reading = analogRead(0);
    temp = reading *0.0048828125*100;
    Serial.print(F("Real Time Temp: ")); 
    Serial.println(temp); 
  }
}


/**************************************************************************/
/*!
 @brief  Displays the driver mode (tiny of normal), and the buffer
 size if tiny mode is not being used
 
 @note   The buffer size and driver mode are defined in cc3000_common.h
 */
/**************************************************************************/
void displayDriverMode(void)
{
#ifdef CC3000_TINY_DRIVER
  Serial.println(F("CC3000 is configure in 'Tiny' mode"));
#else
  Serial.print(F("RX Buffer : "));
  Serial.print(CC3000_RX_BUFFER_SIZE);
  Serial.println(F(" bytes"));
  Serial.print(F("TX Buffer : "));
  Serial.print(CC3000_TX_BUFFER_SIZE);
  Serial.println(F(" bytes"));
#endif
}




