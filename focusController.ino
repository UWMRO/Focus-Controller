// Define driver output pins
int engPin = 7;
int dirPin = 6;
int clkPin = 5;

void setup() {
  pinMode(clkPin, OUTPUT);
  pinMode(engPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  
  Serial.begin(9600);

  // Wait for Serial connection to initialize before proceeding
  while (!Serial) ;
  Serial.println("Input 1 to Turn LED on and 2 to turn off");
}

void loop() {
  if (Serial.available())
  {
    int state = Serial.parseInt();
    switch (state) {
      case 1:
        Serial.println("Move Forward");

        digitalWrite(engPin, LOW);  // Motor is enabled when eng = 0
        digitalWrite(dirPin, LOW);

        moveMotor(clkPin, 250);
        break;

      case 2:
        Serial.println("Move Backward");

        digitalWrite(engPin, LOW);
        digitalWrite(dirPin, HIGH);

        moveMotor(clkPin, 250);
        break;

      case 3:
        Serial.println("Motor Off");

        digitalWrite(engPin, HIGH);
        break;

      case 4:
        Serial.println("Basic Test");

        digitalWrite(engPin, LOW);
        digitalWrite(dirPin, HIGH);

        int period = 25;
        for (int i = 0; i < 100; i++)
        {
          digitalWrite(clkPin, HIGH);
          delay(period);
          digitalWrite(clkPin, LOW);
          delay(period);
        }
        break;
      default:
        // statements
        break;
    } 
    


    if (state == 4)
    {
      Serial.println("Basic Test");

      digitalWrite(engPin, LOW);
      digitalWrite(dirPin, HIGH);      
      int period = 25;
      for (int i = 0; i < 10; i++)
      {
        digitalWrite(clkPin, HIGH);
        delay(period);
        digitalWrite(clkPin, LOW);
        delay(period);
      }
    }
  }
}

void testMotor(int clkPin)
{
  int testPeriod = 500;
  int numSpin = 2;
  for (int i = 0; i < numSpin; i++)
  {
    digitalWrite(clkPin, HIGH);
    delay(testPeriod);
    digitalWrite(clkPin, LOW);
    delay(testPeriod);
  }
}

void moveMotor(int clkPin, int targetNumSteps) // Linear ramp to stop speed then
{
  int slowPeriod = 20; // 20 Hz, could be changed to whatever
  int fastPeriod = 10; // 500 Hz (Measured clock speed at MRO was 770 Hz)
  int accelTime = slowPeriod - fastPeriod; //Number of steps to reach max clk speed, in the case of linear ramp it is just slow - fast
  int numSteps= 0;

  // Start Motor
  // Non linear example: for ( int i = slowPeriod, i>= fastPeriod; i = int(i/1.33) ) 

  //Linear Ramp:
  int clkPeriod = slowPeriod;
  while(clkPeriod >= fastPeriod && numSteps < targetNumSteps - accelTime)
  {
    digitalWrite(clkPin, HIGH);
    delay(clkPeriod);
    digitalWrite(clkPin, LOW);
    delay(clkPeriod);

    Serial.print("Accelerating, Clkperiod = "); Serial.println(clkPeriod);
    numSteps++;
    clkPeriod--;
  }

  //Stay at max speed until we are ready to decelerate
  while (numSteps < targetNumSteps - accelTime)
  {
    digitalWrite(clkPin, HIGH);
    delay(fastPeriod);
    digitalWrite(clkPin, LOW);
    delay(fastPeriod);

    Serial.print("Max speed, Clkperiod = "); Serial.println(clkPeriod);
    numSteps++;
  }

  //Linear ramp down to slowPeriod
  clkPeriod = fastPeriod;
  while(clkPeriod < slowPeriod && numSteps < targetNumSteps)
  {
    digitalWrite(clkPin, HIGH);
    delay(clkPeriod);
    digitalWrite(clkPin, LOW);
    delay(clkPeriod);
    
    Serial.print("Deccelerating, Clkperiod = "); Serial.println(clkPeriod);
    Serial.println(clkPeriod);
    numSteps++;
    clkPeriod++;
  }
  
  Serial.println("Motor has stopped...hopefully ._.");
}

void moveMotorSmooth(int motorPin)
{
  float freq = 150;           // Initial frequency of the clock signal (2 Hz)
  float targetFreq = 3080;   // Target frequency of the clock signal (770 Hz)
  float k = 0.05;                // Rate of frequency change
  unsigned long prevMillis = 0;  // Used to time frequency changes
  unsigned long interval = 1;      // Interval for frequency updates (in milliseconds)
  bool moveCancelled = false;

  while(!moveCancelled) 
  {
    // Calculate the current frequency using asymptotic formula from chat gpt
    // Asymptotic: 
    //freq = freq - (freq - targetFreq) / (1 + exp(-k * ((millis() / 1000.0) - 1000)));
    freq++;
    // Update the frequency every 'interval' milliseconds
    unsigned long currentMillis = millis();
    if (currentMillis - prevMillis >= interval) {
      prevMillis = currentMillis;
      
      // Update the tone frequency
      tone(motorPin, freq); // Generate the clock signal with the calculated frequency
    }
    
    // Optionally print the current frequency to the Serial Monitor
    Serial.println(freq);

      //If the frequency reaches or exceeds the target, stop generating the clock signal
    if (freq >= targetFreq - 1) {
      //noTone(motorPin);  // Stop the clock signal when target is reached
      Serial.println("Target frequency reached.");
      moveCancelled = true;
    }
  }

  delay(800);

  while(freq > 0)
  {
    // Calculate the current frequency using asymptotic formula from chat gpt
    // Asymptotic: freq = freq - (freq - targetFreq) / (1 + exp(-k * (millis() / 1000.0  - 1000)));
    freq--;
    // Update the frequency every 'interval' milliseconds
    unsigned long currentMillis = millis();
    if (currentMillis - prevMillis >= interval) {
      prevMillis = currentMillis;
      
      // Update the tone frequency
      tone(motorPin, freq); // Generate the clock signal with the calculated frequency
    }
    
    // Optionally print the current frequency to the Serial Monitor
    Serial.println(freq);

      // If the frequency reaches or exceeds the target, stop generating the clock signal
    if (freq <= 150) {
      freq = 0;
      noTone(motorPin);  // Stop the clock signal when target is reached
      Serial.println("Target frequency reached.");
    }
  }

}
