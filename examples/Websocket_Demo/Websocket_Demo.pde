#include <SPI.h>
#include <Ethernet.h>
#include <Streaming.h>
#define DEBUGGING true
#include <WebSocket.h>

#define PREFIX "/"
#define PORT 8080

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 1, 170 };

WebSocket websocket(PREFIX, PORT);

void echoAction(WebSocket &socket, String &socketString) {
	socket.actionWrite(&socketString[2]);
}

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  websocket.begin();
  websocket.addAction(&echoAction);
  Serial.println("connected");
}

void loop() {
  websocket.connectionRequest();
}
