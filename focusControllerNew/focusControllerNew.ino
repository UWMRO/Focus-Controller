/*
  MRO Focus Controller (NON THREADED VERSION)

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

// MAC address and IP address for controller
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x25, 0x13 };
IPAddress ip(72, 233, 250, 86);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);


// Focuser fields
volatile int position = 0;
bool moving = false;
int target_steps = 0;
unsigned long lastStepTime = 0;
const int stepDelay = 100;  // ms per step
const int up_limit = 100;
const int down_limit = -100;

int moveSteps = 0;
bool abortMovement = false;

// Movement Tuning parameters
float maxClkFreq = 770;  // in Hz
float minClkFreq = 400;   // in Hz
float clkRamp = 2;       // in Hz

// Limit Conditionals
bool atTop;
bool atBottom;

// Pin Numbers
int clkPin = 9; // yellow
int dirPin = 8; // brown
int enPin = 7;  // green
int topLimPin = 3;
int botLimPin = 2;
// ground is black

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Ethernet WebServer Booting");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1);  // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // Define pins
  pinMode(clkPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enPin, OUTPUT);
  pinMode(topLimPin, INPUT);
  pinMode(botLimPin, INPUT);

  // Check Limits Before starting
  atTop = digitalRead(topLimPin) == LOW;  // Based on Measurements from fall, 0V means limit hit and 3.3V means limit not hit
  atBottom = digitalRead(botLimPin) == LOW;
  digitalWrite(enPin, HIGH);

  // start the server
  server.begin();
  Serial.print("Server starting at ");
  Serial.println(Ethernet.localIP());
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
  }
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
    bool positiveDirection;
    if (stepsParam) {  // moveMotor.shouldRun()
      // if the request contains '-', then switch to negative direction
      char* negative = strstr(request, "-");
      if (negative > stepsParam) {
        digitalWrite(dirPin, LOW);         // is this the right value to write? (SWAP DIGITAL WRITE AT OBSERVATORY IF REVERSED)
        moveSteps = atoi(stepsParam + 7);  // jump to the value after - sign
        positiveDirection = false;
        Serial.println("Going to move backward " + String(moveSteps) + " steps");
      } else {
        digitalWrite(dirPin, HIGH);
        moveSteps = atoi(stepsParam + 6);  // jump to the value after = sign
        positiveDirection = true;
        Serial.println("Going to move forward " + String(moveSteps) + " steps");
      }

      if (moving) {
        sendJSONResponse(client, 523, "\"code\":523");
        Serial.println("MOVEMENT CANCELLED: Motor Already Running");
      } else if (moveSteps > 10000 or moveSteps < 10) {
        sendJSONResponse(client, 522, "\"code\":522");
        Serial.println("MOVEMENT CANCELLED: Incorrect step size! Try int32 that is >=10 and <=10000");
      } else {
        startMovement(client, positiveDirection);
      }
    }
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
                     "\"moving\":" + String(moving) + ",\"step\":" + String(position) + ",\"limit\":" + String(checkLimits()));

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
      sendJSONResponse(client, 200, "\"code\":200");
      Serial.println("Movement Aborted");
    } else {
      sendJSONResponse(client, 530, "\"code\":530");
      Serial.println("ABORT CANCELLED: Motor is not currently moving");
    }
  }
}


// Code for formatting json return strings
void sendJSONResponse(EthernetClient client, int code, String body) {
  client.println("HTTP/1.1 " + String(code) + " OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{" + body + "}");
}

bool checkLimits() {
  return (digitalRead(topLimPin) == LOW || digitalRead(botLimPin) == LOW);
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

void startMovement(EthernetClient client, bool positiveDirection) {
  moving = true;
  int maxFreq = maxClkFreq;


  // Period is 1/f, we want to convert to ms then divide by 2 since pulseTime should be half of a clock cycle
  long minPeriod = long(float(1) * 500000.0 / maxClkFreq);
  long currPeriod = long(float(1) * 500000.0 / minClkFreq);

  int rampDistance = (maxClkFreq-minClkFreq) / clkRamp;
  Serial.println("RAMP DISTANCE" + String(rampDistance));

  //Pre Define Stages
  int rampUpEnd = rampDistance;
  int rampDownStart = moveSteps - rampDistance;
  if(rampDistance > int(moveSteps/2)) {
    rampUpEnd = int(moveSteps/2);
    rampDownStart = int(moveSteps/2) + 1;
    maxFreq = int(minClkFreq+rampUpEnd*clkRamp);
    minPeriod = long(float(1) * 500000.0 / maxFreq);
  }

  int i = 0;
  while (i < moveSteps) {
    long pastTime = micros();

    // Check all conditions before sending clock signal
    if (abortMovement) {
      abortMovement = false;
      return;
    } else if (digitalRead(topLimPin) == LOW) {
      sendJSONResponse(client, 520, "\"code\":520");
      position = position + i;
      Serial.println();
      Serial.println("MOVEMENT CANCELLED: Top limit hit!");
      return;
    } else if (digitalRead(botLimPin) == LOW) {
      position = position - i;
      sendJSONResponse(client, 521, "\"code\":521");
      Serial.println();
      Serial.println("MOVEMENT CANCELLED: Bottom limit hit!");
      return;
    } else {
      //Positive edge of clk
      digitalWrite(clkPin, HIGH);
      Serial.print("clkHigh");

      // delay until halfway through period designated by clk_frequency
      while (micros() - pastTime < currPeriod) {Serial.println("WAITING");}

      pastTime = micros();
      digitalWrite(clkPin, LOW);
      Serial.print("clkLow");

      i++;

      // CODE FOR DETERMINING THE NEXT CLOCK PERIOD LENGTH
      if(i <= rampUpEnd && currPeriod != minPeriod){
          long newPeriod = long(float(1) * 500000.0 / (minClkFreq+i*clkRamp)); // If at i = 1 and clkRamp = 2, sets clk Freq to 1*2 + startClkFreq (in Hz)
          if(newPeriod > minPeriod) {
            currPeriod = newPeriod;
          }
      }
      else if (i >= rampDownStart){
          currPeriod = long(float(1) * 500000.0 / (maxFreq - (i-rampDownStart)*clkRamp));
      }
    }

    // delay until reach the end of period
      while (micros() - pastTime < currPeriod) {Serial.println("WAITING");}
  }

  Serial.println();
  Serial.println("Movement successfully finished (Huge Dub)");
  moving = false;
  if (positiveDirection) {
    position = position + moveSteps;
  }
  else {
    position = position - moveSteps;
  }

  sendJSONResponse(client, 200, "\"code\":200");
}