/*
  Web Server

 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 modified 02 Sept 2015
 by Arturo Guadalupi
 
 */

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xA8, 0x61, 0x0A, 0xAE, 0x25, 0x13
};
IPAddress ip(192, 168, 1, 11);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// Focuser state
volatile int position = 0;
bool moving = false;
int target_steps = 0;
unsigned long lastStepTime = 0;
const int stepDelay = 100; // ms per step
const int up_limit = 100;
const int down_limit = -100;

void setup() {
  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Ethernet WebServer Example");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {
  // listen for incoming clients
  updatePosition();
  EthernetClient client = server.available();
  if (client) {

    handleClient(client);
    // Serial.println("new client");
    // // an http request ends with a blank line
    // boolean currentLineIsBlank = true;
    // while (client.connected()) {
    //   if (client.available()) {
    //     char c = client.read();
    //     Serial.write(c);
    //     // if you've gotten to the end of the line (received a newline
    //     // character) and the line is blank, the http request has ended,
    //     // so you can send a reply
    //     if (c == '\n' && currentLineIsBlank) {
    //       // send a standard http response header
    //       client.println("HTTP/1.1 200 OK");
    //       client.println("Content-Type: text/html");
    //       client.println("Connection: close");  // the connection will be closed after completion of the response
    //       client.println("Refresh: 5");  // refresh the page automatically every 5 sec
    //       client.println();
    //       client.println("<!DOCTYPE HTML>");
    //       client.println("<html>");
    //       // output the value of each analog input pin
    //       for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
    //         int sensorReading = analogRead(analogChannel);
    //         client.print("analog input ");
    //         client.print(analogChannel);
    //         client.print(" is ");
    //         client.print(sensorReading);
    //         client.println("<br />");
    //       }
    //       client.println("</html>");
    //       break;
    //     }
    //     if (c == '\n') {
    //       // you're starting a new line
    //       currentLineIsBlank = true;
    //     } else if (c != '\r') {
    //       // you've gotten a character on the current line
    //       currentLineIsBlank = false;
    //     }
    //   }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    //Serial.println("client disconnected");
  }

  void handleClient(EthernetClient client) {
    char request[128];
    int i = 0;
  
    while (client.connected() && i < 127) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' || c == '\r') break;
        request[i++] = c;
      }
    }
    request[i] = '\0';

  // To access: 192.168.1.11/status
  // or curl -X GET http://192.168.1.11/status in terminal
  if (strstr(request, "GET /status")) {
    sendJSONResponse(client, 200, 
      "\"moving\":" + String(moving) + 
      ",\"step\":" + String(position) + 
      ",\"limit\":" + String(checkLimits()));
  }

  // curl -X POST http://192.168.1.11/move?steps=50
  else if (strstr(request, "POST /move?steps")) {
    char* stepsParam = strstr(request, "steps=");
    if (stepsParam) {
      int steps = atoi(stepsParam + 6);
      Serial.print("moving ");
      Serial.println(steps);
      startMovement(steps);
    }
    sendJSONResponse(client, 200, "\"code\":200");
  }

  // curl -X POST http://192.168.1.11/abort
  else if (strstr(request, "POST /abort")) {
    moving = false;
    sendJSONResponse(client, 300, "\"code\":300");
    Serial.println("Aborting movement");
  }
}

void sendJSONResponse(EthernetClient client, int code, String body) {
  client.println("HTTP/1.1 " + String(code) + " OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{" + body + "}");
}

void updatePosition() {
  if (moving && (millis() - lastStepTime > stepDelay)) {
    position += (target_steps > 0) ? 1 : -1;
    target_steps -= (target_steps > 0) ? 1 : -1;
    lastStepTime = millis();
    
    if (target_steps == 0 || checkLimits()) {
      moving = false;
      position = constrain(position, down_limit, up_limit);
    }
  }
}

bool checkLimits() {
  return (position >= up_limit) || (position <= down_limit);
}

void startMovement(int steps) {
  target_steps = steps;
  moving = true;
}

