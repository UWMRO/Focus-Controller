Currently, The telescope's secondary mirror controls its focus position and is driven by a stepper motor. This motor is connected to a PK2 Stepper Motor Drive, which itself is controlled from the Heimdall I/O board, which processes user requests from the Evora client. Limit switches stop the mirror from moving too far in either direction, and are directly wired back to the I/O board. 

The purpose of this project is to create an Arduino that will receive focusing-related commands from the client and send telescope focus controls to the PK2 stepper motor drive.  

Link to the Engineering Notebook for the project: https://docs.google.com/document/d/16X8c-PVCSMUchpS4MnRiA4OjP7H_lG1Q/edit?usp=sharing&ouid=109139938162474682100&rtpof=true&sd=true
