#include <SPI.h>
#include <Ethernet.h>
#include <Streaming.h>
#include <WebSocket.h>

#define PREFIX "/"
#define PORT 8080

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 1, 170 };

WebSocket websocket(ip, PREFIX, PORT);

void helloAction(WebSocket &socket, String &socketString) {

	if(socketString.substring(0)=="Hello") {
		socket.actionWrite("Hi!");
	}
}
void clackAction(WebSocket &socket, String &socketString) {
	
	if(socketString.substring(0)=="Click!") {
		socket.actionWrite("Clack!");
	}
}

void setup() {
	Serial.begin(9600);
	Ethernet.begin(mac, ip);
	websocket.begin();
	websocket.addAction(&clackAction);
	websocket.addAction(&helloAction);
}

void loop() {
  websocket.connectionRequest();
}
