#define FIRMWARE_VERSION 219

// device_id, numer used a position in array to get last octet of MAC and static IP
// prototype 0, unit 1, unit 2... unit 7.

// ****************
#define DEVICE_ID 0
// ****************


// Enable/Disable modules
#define SERIAL_DEBUGING

// pins definition
#define LED_PIN 6

// Parameters
#define SERIAL_SPEED 115200


//------------------------------------------------------------------------------
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
// #include <OSCBundle.h>
#include <OSCMessage.h>

#include <Wire.h>
#include <i2cEncoderLibV2.h>

#include <AccelStepper.h>


//------------------------------ Stepper motors --------------------------------

// 28BYJ-48 motor runs in full step mode, each step corresponds to a rotation of 11.25°.
// That means there are 32 steps per revolution (360°/11.25° = 32). What this means is that
// there are actually 32*63.68395 steps per revolution = 2037.8864 ~ 2038 steps!

// Define a stepper and the pins it will use
// AccelStepper stepper; // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5


// Motor one bipolar, converted 28BYJ-48 with DRV8834 driver

// Focus
int motor1DirPin = 33;
int motor1StepPin = 34;

// Aperture
int motor2DirPin = 39;
int motor2StepPin = 40;

// Zoom
int motor3DirPin = 20;
int motor3StepPin = 21;

//set up the accelStepper intance
//the "1" tells it we are using a driver
AccelStepper stepper1(AccelStepper::DRIVER, motor1StepPin, motor1DirPin);
AccelStepper stepper2(AccelStepper::DRIVER, motor2StepPin, motor2DirPin);
AccelStepper stepper3(AccelStepper::DRIVER, motor3StepPin, motor3DirPin);

// int position1 = 0;
// int position2 = 0;
// int position3 = 0;

//------------------------------ I2C encoders ----------------------------------
// Connections:
// - -> GND
// + -> 5V
// SDA -> A4
// SCL -> A5
// INT -> 3 temporary for tests

#define ENCODER_N 3 //Number limit of the encoder
const int IntPin = 17; // Definition of the interrupt pin. You can change according to your board

//Class initialization with the I2C addresses
i2cEncoderLibV2 RGBEncoder[ENCODER_N] = { i2cEncoderLibV2(0x01),
                                          i2cEncoderLibV2(0x02),
                                          i2cEncoderLibV2(0x03),
                                        };
uint8_t encoder_status, i;


//---------------------------- MAC & IP list ----------------------------------
// Change #define DEVICE_ID to a number from 0 to 7 on top of the code to
// assign MAC and IP for device, they mus be unique within the netowrk

byte MAC_ARRAY[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
int IP_ARRAY[] = {240, 241, 242, 243, 244, 245, 246, 247};
//-----------------------------------------------------------------------------

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, MAC_ARRAY[DEVICE_ID]
};
IPAddress ip(10, 0, 10, IP_ARRAY[DEVICE_ID]);

bool isLANconnected = false;
// bool isUDPconnected = false;

//----------------------------- Setup for OSC ----------------------------------
EthernetUDP Udp;

// OSC destination address, 255 broadcast
IPAddress targetIP(10, 0, 10, 101);   // Isadora machine IP address
const unsigned int destPort = 9999;          // remote port to receive OSC
const unsigned int localPort = 8888;        // local port to listen for OSC packets

unsigned long previousMillis = 0;
const long interval = 1000;
long uptime = 0;

char osc_prefix[16];                  // device OSC prefix message, i.e /camera1


//***************************** Functions *************************************

void moveMotorToPosition(uint8_t motor, int position){
  switch(motor) {
    case 1:
      Serial.print("move motor to position:");
      Serial.println(position);
      stepper1.moveTo(position);
      break;
    case 2:
      stepper2.moveTo(position);
      break;
    case 3:
      stepper3.moveTo(position);
      break;
  }
}

//Callback when the encoder is rotated
void encoder_rotated(i2cEncoderLibV2* obj) {
  if (obj->readStatus(i2cEncoderLibV2::RINC))
    Serial.print("Increment ");
  else
    Serial.print("Decrement ");
  int motorID = (obj->id) + 1;
  Serial.print(motorID);
  int position =obj->readCounterInt();
  Serial.print(": ");
  Serial.println(position);

  obj->writeRGBCode(0x00FF00);

  moveMotorToPosition(motorID, position);
}

void encoder_click(i2cEncoderLibV2* obj) {
  Serial.print("Push: ");
  Serial.println(obj->id);
  obj->writeRGBCode(0x0000FF);
}

void encoder_thresholds(i2cEncoderLibV2* obj) {
  if (obj->readStatus(i2cEncoderLibV2::RMAX))
    Serial.print("Max: ");
  else
    Serial.print("Min: ");
  Serial.println(obj->id);
  obj->writeRGBCode(0xFF0000);
}

void encoder_fade(i2cEncoderLibV2* obj) {
  obj->writeRGBCode(0x000000);
}


int uptimeInSecs(){
  return (int)(millis()/1000);
}

void servo1_OSCHandler(OSCMessage &msg, int addrOffset) {
  int inValue = msg.getFloat(0);
  #ifdef SERIAL_DEBUGING
    Serial.print("osc servo 1 update: ");
    Serial.println(inValue);
  #endif
}

void servo2_OSCHandler(OSCMessage &msg, int addrOffset) {
  int inValue = msg.getFloat(0);
  #ifdef SERIAL_DEBUGING
    Serial.print("osc servo 2 update: ");
    Serial.println(inValue);
  #endif
}

void servo3_OSCHandler(OSCMessage &msg, int addrOffset) {
  int inValue = msg.getFloat(0);
  #ifdef SERIAL_DEBUGING
    Serial.print("osc servo 3 update: ");
    Serial.println(inValue);
  #endif
}

void receiveOSCsingle(){
  // read incoming udp packets
  OSCMessage msgIn;
  int size;

  if( (size = Udp.parsePacket())>0)
  {
    //while((size = Udp.available()) > 0)
    while(size--)
      msgIn.fill(Udp.read());

    // route messages
    if(!msgIn.hasError()) {
      // TODO add dynamic device number based on setting
      msgIn.route("/device1/servo/1", servo1_OSCHandler);
      msgIn.route("/device1/servo/2", servo2_OSCHandler);
      msgIn.route("/device1/servo/3", servo3_OSCHandler);
      // msgIn.route("/device1/localise", localise_OSCHandler);
    }

    //finish reading this packet:
    Udp.flush();
    //restart UDP connection to receive packets from other clients
    Udp.stop();
    Udp.begin(localPort);
  }
}

void sendOSCmessage(char* name, int value){
  char message_osc_header[32];
  message_osc_header[0] = {0};
  strcat(message_osc_header, osc_prefix);
  strcat(message_osc_header, name);
  OSCMessage message(message_osc_header);
  message.add(value);
  Udp.beginPacket(targetIP, destPort);
  message.send(Udp);
  Udp.endPacket();
  message.empty();
}

void sendOSCreport(){
  #ifdef SERIAL_DEBUGING
    Serial.print("Sending OSC raport ");
  #endif
  sendOSCmessage("/ver", FIRMWARE_VERSION);
  sendOSCmessage("/uptime", uptimeInSecs());
  sendOSCmessage("/motor1/position", stepper1.currentPosition());
  sendOSCmessage("/motor2/position", stepper2.currentPosition());
  sendOSCmessage("/motor3/position", stepper3.currentPosition());
  #ifdef SERIAL_DEBUGING
    Serial.println(" *");
  #endif
}

bool checkEthernetConnection(){
  // // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    #ifdef SERIAL_DEBUGING
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    #endif
    digitalWrite(LED_PIN, LOW);
    return false;
  }
  else if (Ethernet.linkStatus() == LinkOFF) {
    #ifdef SERIAL_DEBUGING
      Serial.println("Ethernet cable is not connected.");
    #endif
    digitalWrite(LED_PIN, HIGH);
    return false;
  }
  else if (Ethernet.linkStatus() == LinkON) {
    #ifdef SERIAL_DEBUGING
      Serial.println("Ethernet cable is connected.");
    #endif
    digitalWrite(LED_PIN, LOW);
    return true;
  }
}

//******************************************************************************

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  //NOTE on boot the led inidicate power, once connects with ethernet goes off

  #ifdef SERIAL_DEBUGING
    Serial.begin(SERIAL_SPEED);
    // while (!Serial) {
    //   ; //TODO remove for production, debuging only, wait for serial port to connect. Needed for native USB port only
    // }
  #endif

  #ifdef SERIAL_DEBUGING
    Serial.print("\r\nFirmware Ver: "); Serial.print(FIRMWARE_VERSION);
    Serial.println(" written by Grzegorz Zajac");
    Serial.println("Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
    Serial.println();
  #endif

//-------------------------- Initializing encoders -----------------------------
  #ifdef SERIAL_DEBUGING
    Serial.println("initializing encoders");
  #endif
  uint8_t enc_cnt;

  pinMode(IntPin, INPUT);
  Wire.begin();
  //Reset of all the encoder ìs
  for (enc_cnt = 0; enc_cnt < ENCODER_N; enc_cnt++) {
    RGBEncoder[enc_cnt].reset();
  }

  // Initialization of the encoders
  for (enc_cnt = 0; enc_cnt < ENCODER_N; enc_cnt++) {
    RGBEncoder[enc_cnt].begin(
      i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE
      | i2cEncoderLibV2::DIRE_RIGHT
      | i2cEncoderLibV2::IPUP_ENABLE
      | i2cEncoderLibV2::RMOD_X1
      | i2cEncoderLibV2::RGB_ENCODER);
    RGBEncoder[enc_cnt].writeCounter((int32_t) 0); //Reset of the CVAL register
    RGBEncoder[enc_cnt].writeMax((int32_t) 400); //Set the maximum threshold to 50
    RGBEncoder[enc_cnt].writeMin((int32_t) 0); //Set the minimum threshold to 0
    RGBEncoder[enc_cnt].writeStep((int32_t) 10); //The step at every encoder click is 1
    RGBEncoder[enc_cnt].writeRGBCode(0);
    RGBEncoder[enc_cnt].writeFadeRGB(3); //Fade enabled with 3ms step
    RGBEncoder[enc_cnt].writeAntibouncingPeriod(25); //250ms of debouncing
    RGBEncoder[enc_cnt].writeDoublePushPeriod(0); //Set the double push period to 500ms

    /* Configure the events */
    RGBEncoder[enc_cnt].onChange = encoder_rotated;
    RGBEncoder[enc_cnt].onButtonRelease = encoder_click;
    RGBEncoder[enc_cnt].onMinMax = encoder_thresholds;
    RGBEncoder[enc_cnt].onFadeProcess = encoder_fade;

    /* Enable the I2C Encoder V2 interrupts according to the previus attached callback */
    RGBEncoder[enc_cnt].autoconfigInterrupt();
    RGBEncoder[enc_cnt].id = enc_cnt;
  }

  RGBEncoder[0].writeCounter((int32_t) 100); //Reset of the CVAL register

//-------------------------- Initializing steppers -----------------------------
  #ifdef SERIAL_DEBUGING
    Serial.println("initializing steppers");
  #endif

  stepper1.setMaxSpeed(500);
  stepper1.setAcceleration(200);

//-------------------------- Initializing ethernet -----------------------------
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);    // begin reset the WIZ820io
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);  // de-select WIZ820io
  digitalWrite(9, HIGH);   // end reset pulse

  Ethernet.init(10);

  // start the Ethernet connection
  Ethernet.begin(mac, ip);

  //Create OSC message header with unit number
  osc_prefix[0] = {0};
  strcat(osc_prefix, "/camera");

  char buf[8];
  sprintf(buf, "%d", DEVICE_ID);
  strcat(osc_prefix, buf);


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
    Serial.print("OSC prefix: ");
    Serial.println(osc_prefix);
    Serial.println();
    Serial.println("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
  #endif

  //TODO test osc after loosing connection and reconnecting
  Udp.begin(localPort);
}

void loop() {
  receiveOSCsingle();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    isLANconnected = checkEthernetConnection();
    if (isLANconnected){
      sendOSCreport();
    }
    Serial.println(stepper1.currentPosition());
  }

  // check pots
  uint8_t enc_cnt;
  if (digitalRead(IntPin) == LOW) {
    //Interrupt from the encoders, start to scan the encoder matrix
    for (enc_cnt = 0; enc_cnt < ENCODER_N; enc_cnt++) {
      if (digitalRead(IntPin) == HIGH) { //If the interrupt pin return high, exit from the encoder scan
        break;
      }
      RGBEncoder[enc_cnt].updateStatus();
    }
  }


  // If at the end of travel go to the other end
  // if (stepper1.distanceToGo() == 0)
  //   stepper1.moveTo(-stepper1.currentPosition());

  stepper1.run();
}
