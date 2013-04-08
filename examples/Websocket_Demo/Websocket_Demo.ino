#include <SPI.h>
#include <Ethernet.h>

// Enabe debug tracing to Serial port.
#define DEBUG

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

#include <WebSocket.h>

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 1 , 77 };

// Create a Websocket server
WebSocket wsServer;

void onConnect(WebSocket &socket) {
  Serial.println("onConnect called");
}


// You must have at least one function with the following signature.
// It will be called by the server when a data frame is received.
void onData(WebSocket &socket, char* dataString, byte frameLength) {
  
#ifdef DEBUG
  Serial.print("Got data: ");
  Serial.write((unsigned char*)dataString, frameLength);
  Serial.println();
#endif
  
  // Just echo back data for fun.
  socket.send(dataString, strlen(dataString));
}

void onDisconnect(WebSocket &socket) {
  Serial.println("onDisconnect called");
}

void setup() {
#ifdef DEBUG  
  Serial.begin(57600);
#endif
  Ethernet.begin(mac, ip);
  
  wsServer.registerConnectCallback(&onConnect);
  wsServer.registerDataCallback(&onData);
  wsServer.registerDisconnectCallback(&onDisconnect);  
  wsServer.begin();
  
  delay(100); // Give Ethernet time to get ready
}

void loop() {
  // Should be called for each loop.
  wsServer.listen();
  
  // Do other stuff here, but don't hang or cause long delays.
  delay(100);
  if (wsServer.isConnected()) {
    wsServer.send("abc123", 6);
  }
}
