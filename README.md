Telescope focus is achieved by driving the secondary mirror in and out using a stepper motor, which itself is driven by a PK2 Stepper Motor Drive. This Arduino code creates an HTTP server to receive focusing-related commands from the Evora client, which is used to command the PK2 stepper motor drive.

Limit switches stop the mirror from moving too far in either direction, and (will soon be) monitored by the Arduino. 

Future ideas:

- Monitor the variable potentiometer behind the secondary (ex: https://docs.arduino.cc/learn/electronics/potentiometer-basics/)
- Monitor telescope temperature via an RTD sensor: https://www.adafruit.com/product/3984.

Link to the Engineering Notebook for the project: https://docs.google.com/document/d/16X8c-PVCSMUchpS4MnRiA4OjP7H_lG1Q/edit?usp=sharing&ouid=109139938162474682100&rtpof=true&sd=true
