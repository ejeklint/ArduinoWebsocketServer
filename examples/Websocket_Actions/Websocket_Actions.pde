#include <SPI.h>
#include <Ethernet.h>

// Enabe debug tracing to Serial port.
#define DEBUGGING
// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64
// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1

#include <WebSocket.h>

#define PREFIX "/ws"
#define PORT 8080

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 4, 2 };

// Create a Websocket server listening to http://192.168.4.2:8080/ws/
WebSocket websocketServer(PREFIX, PORT);

// You must have at least one function with the following signature.
// It will be called by the server when a data frame is received.
void dataReceivedAction(WebSocket &socket, String &dataString) {
    // I just had a LED on pin 9
    if (dataString == "1") {
      digitalWrite(9, HIGH);
    } else {
      digitalWrite(9, LOW);
    }
  
    socket.sendData("Ok");
}

void setup() {
#ifdef DEBUGGING  
  Serial.begin(57600);
#endif
    pinMode(9, OUTPUT);
	Ethernet.begin(mac, ip);
	websocketServer.begin();
	// Add the callback function to the server. You can have several callback functions
	// if you like, they will be called with the same data and in the same order as you
	// add them to the server. If you have more than one, define CALLBACK_FUNCTIONS before including
	// WebSocket.h
	websocketServer.addAction(&dataReceivedAction);
  delay(1000); // Give Ethernet time to get ready
}

void loop() {
	// Don't add any code after next line. It will never be called.
	websocketServer.connectionRequest();
}
