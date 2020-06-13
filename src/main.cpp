#define FIRMWARE_VERSION 204

// device_id, numer used a position in array to get last octet of MAC and static IP
// prototype 0, unit 1, unit 2... unit 7.
#define DEVICE_ID 0

// Enable/Disable modules
#define SERIAL_DEBUGING
#define SERIAL_SPEED 115200

// pins definition
#define LED_PIN 2


//------------------------------------------------------------------------------
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>


//---------------------------- MAC & IP list ----------------------------------
byte MAC_ARRAY[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
int IP_ARRAY[] = {240, 241, 242, 243, 244, 245, 246, 247};
//-----------------------------------------------------------------------------

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, MAC_ARRAY[DEVICE_ID]
};
IPAddress ip(10, 0, 10, IP_ARRAY[DEVICE_ID]);


//***************************** Functions *************************************
void checkConnectin(){
  if (Ethernet.linkStatus() == Unknown) {
    Serial.println("Link status unknown. Link status detection is only available with W5200 and W5500.");
    digitalWrite(LED_PIN, HIGH);
  }
  else if (Ethernet.linkStatus() == LinkON) {
    Serial.print("Link status: On, connected with IP: ");
    Serial.println(Ethernet.localIP());
    digitalWrite(LED_PIN, LOW);
  }
  else if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Link status: Off");
    digitalWrite(LED_PIN, HIGH);
  }
}

//******************************************************************************

void setup() {
  #ifdef SERIAL_DEBUGING
    Serial.begin(SERIAL_SPEED);
    while (!Serial) {
      ; //TODO remove for production, debuging only, wait for serial port to connect. Needed for native USB port only
    }
  #endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  //NOTE on boot the led inidicate power, once connects with ethernet goes off

//-------------------------- Initializing ethernet -----------------------------
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);    // begin reset the WIZ820io
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);  // de-select WIZ820io
  digitalWrite(9, HIGH);   // end reset pulse

  Ethernet.init(10);

  // Initialize the Ethernet server library
  // with the IP address and port you want to use
  // (port 80 is default for HTTP):
  // EthernetServer server(80);

  // start the Ethernet connection
  Ethernet.begin(mac, ip);

  // // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    #ifdef SERIAL_DEBUGING
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    #endif
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    #ifdef SERIAL_DEBUGING
      Serial.println("Ethernet cable is not connected.");
    #endif
    digitalWrite(LED_PIN, HIGH);
  }

  #ifdef SERIAL_DEBUGING
    Serial.println("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
    Serial.print("IP Address        : ");
    Serial.println(Ethernet.localIP());
    Serial.print("Subnet Mask       : ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Default Gateway IP: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS Server IP     : ");
    Serial.println(Ethernet.dnsServerIP());
    Serial.println();
    Serial.println("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
  #endif

  // start the server
  // server.begin();
  // Serial.print("server is at ");
  // Serial.println(Ethernet.localIP());
}

void loop() {
  checkConnectin();
  delay(500);
}
