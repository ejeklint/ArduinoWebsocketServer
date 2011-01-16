
#include <SPI.h>
#include <Ethernet.h>
#include <Streaming.h>
#include <WebSocket.h>

#define PREFIX "/"
#define PORT 8080

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 4, 2 };

WebSocket websocket(ip, PREFIX, PORT);

void helloAction(WebSocket &socket, String &socketString) {
  
    //I just had a LED on pin 9
  
    if(socketString == "1"){
      digitalWrite(9, HIGH);
    }else{
      digitalWrite(9, LOW);
    }
  
    socket.actionWrite("Ok");
}

void setup() {
	Serial.begin(9600);
       pinMode(9, OUTPUT);
	Ethernet.begin(mac, ip);
	websocket.begin();
	//websocket.addAction(&clackAction);
	websocket.addAction(&helloAction);
}

void loop() {
  websocket.connectionRequest();
}
