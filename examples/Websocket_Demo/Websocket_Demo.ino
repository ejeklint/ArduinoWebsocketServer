#include <SPI.h>
#include <Ethernet.h>

// Enabe debug tracing to Serial port.
#define DEBUG
// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64
// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1

#include <WebSocket.h>

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 10, 0, 1, 68 };

// Create a Websocket server
WebSocket websocketServer;

// You must have at least one function with the following signature.
// It will be called by the server when a data frame is received.
void dataReceivedAction(WebSocket &socket, char* dataString, byte frameLength) {
  
#ifdef DEBUG
  Serial.print("Got data: ");
  Serial.write((unsigned char*)dataString, frameLength);
  Serial.println();
#endif
  
  // Just echo back data for fun.
  socket.send(dataString, strlen(dataString));
}

void setup() {
#ifdef DEBUG  
  Serial.begin(57600);
#endif
  Ethernet.begin(mac, ip);
  websocketServer.begin();
  
  // Add the callback function to the server.
  websocketServer.registerCallback(&dataReceivedAction);
  
  delay(1000); // Give Ethernet time to get ready
}

void loop() {
  // Don't add any code after next line. It will never be called.
  websocketServer.connectionRequest();
}
