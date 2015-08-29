
// BOF preprocessor bug prevent - insert me on top of your arduino-code
// From: http://www.a-control.de/arduino-fehler/?lang=en
#if 1
__asm volatile ("nop");
#endif

// Output debug messages to serial
#define DEBUG
#define SDLOGON
#define CLOCK

#include <SPI.h>
#include <Timer.h>
#include <Event.h>
#include <EasyTransferVirtualWire.h>
#include <SarahCommProtocol.h>
#include <EEPROM.h> 
#include <VirtualWire.h>
#include <RCSwitch.h>
#include <Wire.h>
#include <Time.h>
#include <DS3231.h>
#include <ncutils.h>
#include <MemoryFree.h>
#ifdef SDLOGON
#include <SD.h>
#endif

RCSwitch rf = RCSwitch();

scp comm;

String str;
int i;
unsigned int fm_interval;
long fm_duration;

DS3231 Clock;
bool Century=false;
bool h12;
bool PM;

// SD card

//Sd2Card card;
//SdVolume volume;
File root;
const int chipSelect = 4;    

#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xC0, 0xB0, 0xC0, 0xA8, 0x01, 0xC8
};
IPAddress ip(192, 168, 1, 200);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// --------------------------- SETUP ---------------------------------

void setup()
{
	
	// Start the I2C interface
Wire.begin();

pinMode(53, OUTPUT);
// Clock.setDateTime(2015, 8, 22, 4, 14, 30);


// Start the serial interface

#ifdef DEBUG
	Serial.begin(115200);
	while (!Serial) ; // wait until Arduino Serial Monitor opens
#endif

	comm.init(&Clock, 2, 6); // default pins: receiver = D2, transmitter = D4. clock pins on Mega2560	20 (SDA), 21 (SCL)
	comm.enable_control_blinking();  // default blinking on pin 13 with 10000 millisec interval

	rf.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2

	digitalWrite(10, HIGH);
	pinMode(10, OUTPUT);
	
	Ethernet.begin(mac, ip);
	server.begin();
	Serial.print("server is at ");
	Serial.println(Ethernet.localIP());
}

void loop()
{

	comm.received();

//  if (digitalRead(6) == HIGH) {
//    Serial.println("Button pressed");
//	comm.send(B_ALL, SQC_PING, 0, 0);
//	delay(500);
//  }

  if (rf.available()) {
	comm.enable_control_blinking();
    int value = rf.getReceivedValue();
    if (value == 0) {
      Serial.print("Unknown");
    } else {
      Serial.print("Received ");
      Serial.print( rf.getReceivedValue() );
      Serial.print(" / ");
      Serial.print( rf.getReceivedBitlength() );
      Serial.print("bit ");
      Serial.print("P: ");
      Serial.println( rf.getReceivedProtocol() );
#ifdef DEBUG

		Serial.print("t: ");
		Serial.print(Clock.readTemperature(), DEC);
		Serial.println(" C");
	Serial.print("GET /objects/?object=ArduinoMother&op=m&m=Msg&msg=");
	Serial.print(rf.getReceivedValue());
    Serial.println(" HTTP/1.0");  // Call method AurduinoMother.Msg in MajorDoMo via arduino_gw.exe
	Serial.print("Mem: ");
	Serial.println(freeMemory());
	String lstr = "Received RF signal ";
	lstr += rf.getReceivedValue();
	comm.sd_log(lstr, true);

#endif
    }


	rf.resetAvailable();
  }

  // Ethernet
  
  
    // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  
  // Cat feeding machine control
  if (Serial.available() > 0) {
	str = Serial.readStringUntil('\n');
	i = Serial.parseInt();
	if (str == "interval")	{
		fm_interval = i;
		Serial.print("I: ");
		Serial.println(fm_interval);
	}
	if (str == "duration")	{
		fm_duration = i;
		Serial.print("D: ");
		Serial.println(fm_duration);
	}
	if (str == "send")	{
		Serial.print("Send i: ");
		Serial.print(fm_interval);
		Serial.print(", d: ");
		Serial.println(fm_duration);
		comm.send(B_CFM, SQD_SERVO_INTERVAL, fm_interval, fm_duration);
	}
  }

}


	

