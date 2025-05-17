/*
  MRO Focus Controller 

  An arduino that sends focussing based commands to the Manastash Ridge Observatory
  
  The arduino hosts a simple Arduino web server, and recieves commands via
  web server actions sent over it's ethernet port

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Clock Motor Output - pin 9
 * Direction Motor Output  - pin 8
 * Enable Motor Output - pin 7
 * Limit pin 1 - pin 6
 * Limit pin 2 - pin 5
 * Potentiometer - TinkerKit connection "IN3"

 Code written by AUEG Arduino Team 2025! 
 */

#include <SPI.h>
#include <Ethernet.h>
// #include <Thread.h>
// #include <ThreadController.h>
// #include <StaticThreadController.h>

// MAC address and IP address for controller
byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x25, 0x13};
IPAddress ip(192, 168, 1, 11);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// Multithread for handling motor movement in background
// Thread moveMotor = Thread();

// Focuser fields
volatile int position = 0;
bool moving = false;
int target_steps = 0;
unsigned long lastStepTime = 0;
const int stepDelay = 100; // ms per step
const int up_limit = 100;
const int down_limit = -100;

int moveSteps = 0;
float clkFreq = 50; // in Hz
bool abortMovement = false;

int clkPin = 9;
int dirPin = 8;
int enPin = 7;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Ethernet WebServer Booting");

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
  Serial.print("Server starting at ");
  Serial.println(Ethernet.localIP());

  // initialize thread
  // moveMotor.onRun(startMovement);
  // moveMotor.enabled = true;
}

// Code for formatting json return strings
void sendJSONResponse(EthernetClient client, int code, String body) {
  client.println("HTTP/1.1 " + String(code) + " OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{" + body + "}");
}

// Emulating the potentiometer position
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

void startMovement() {
  //Disable thread so another movement call won't overwrite the current movement
  moving = true;

  // Period is 1/f, we want to convert to ms then divide by 2 since pulseTime should be half of a clock cycle
  long pulseTime = long(float(1)*500000.0/clkFreq);

  int i = 0;
  while (i < moveSteps && !abortMovement) {
    long pastTime = micros();
    digitalWrite(clkPin, HIGH);
    Serial.print("clkHigh");

    // delay until halfway through period designated by clk_frequency
    while (micros() - pastTime < pulseTime) {}
    
    pastTime = micros();
    digitalWrite(clkPin, LOW);
    Serial.print("clkLow");

    // delay until reach the end of period
    while (micros() - pastTime < pulseTime) {}

    i++;
  }
  Serial.println();
  Serial.println("Movement Finished");

  moving = false;
  // moveMotor.enabled = true;
}

void handleClient(EthernetClient client) {
    char request[128];
    int i = 0;

    // Retrieve client command
    while (client.connected() && i < 127) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' || c == '\r') break;
        request[i++] = c;
      }
    }
    request[i] = '\0';


  /*
    To test the following commands enter something like "192.168.1.11/status" in web browser
    can also test with "curl -X GET http://192.168.1.11/status" in terminal like powershell
    ^ Make sure ethernet port IP of test computer is configured 
    (E.g. access ethernet settings of computer, change from DHCP to manual, use ip like 192.168.1.9 and basic mask like 255.255.0.0 )
  */

  /*
    Command: move?steps
    Paramters: steps: <int32>
    Returns: {code: <int32>}
              ^ Descriptions of each json field are described in Focuser Arduino Specification Google Doc
    Test code: "curl -X POST http://192.168.1.11/move?steps=500"
  */
  if (strstr(request, "POST /move?steps")) {
    char* stepsParam = strstr(request, "steps=");
    if (stepsParam) { // moveMotor.shouldRun()
      // if the request contains '-', then switch to negative direction
      char* negative = strstr(request, "-");
      if (negative > stepsParam) {
        digitalWrite(8, LOW); // is this the right value to write?
        moveSteps = atoi(stepsParam + 7); // jump to the value after - sign
      }
      else {
        digitalWrite(8, HIGH);
        moveSteps = atoi(stepsParam + 6); // jump to the value after = sign
      }
        
      // now tell them we are moving
      Serial.print("moving ");
      Serial.println(moveSteps);

      // moveMotor.run();
      startMovement();
    sendJSONResponse(client, 200, "\"code\":200");
  }
  
  /*
  Command: status
  Paramters: None
  Returns: {moving: <bool>, voltage: <int32>, upper_limit: <bool>, lower_limit: <bool>, step: <int32>}
            ^ Descriptions of each json field are described in Focuser Arduino Specification Google Doc
  Test code: "curl -X GET http://192.168.1.11/status"
  */
  if (strstr(request, "GET /status")) {
    sendJSONResponse(client, 200, 
      "\"moving\":" + String(moving) + 
      ",\"step\":" + String(position) + 
      ",\"limit\":" + String(checkLimits()));

    Serial.println("Status Message Sent! :D");
  }

  /*
  Command: abort
  Paramters: none
  Returns: {code: <int32>}
            ^ Descriptions of each json field are described in Focuser Arduino Specification Google Doc
  Test code: "curl -X POST http://192.168.1.11/abort"
  */
  else if (strstr(request, "POST /abort")) {
    if (moving) {
      abortMovement = true;
      Serial.println("Aborting movement");
    }
    else {
      Serial.println("No Movement to Abort");
    }

    sendJSONResponse(client, 300, "\"code\":300");
  }
 } 
}

void loop() {
  // listen for incoming clients
  updatePosition();
  EthernetClient client = server.available();
  if (client) {

    handleClient(client);

    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    // Serial.println("client disconnected");
  }
}
