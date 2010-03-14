#include <Ethernet.h>
#include <Streaming.h>
#include <WString.h>
#include <WebSocket.h>

#define PREFIX "/"
#define PORT 8080

byte mac[] = { 0x53, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 1, 170 };

WebSocket websocket(PREFIX, PORT);

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  websocket.begin();
}

void loop() {
  websocket.connectionRequest();
}