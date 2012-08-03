//#define DEBUGGING

#include "global.h"
#include "WebSocketClient.h"

#include "sha1.h"
#include "base64.h"


bool WebSocketClient::handshake(Client &client) {

    socket_client = &client;

    // If there is a connected client->
    if (socket_client->connected()) {
        // Check request and look for websocket handshake
#ifdef DEBUGGING
            Serial.println(F("Client connected"));
#endif
        if (analyzeRequest()) {
#ifdef DEBUGGING
                Serial.println(F("Websocket established"));
#endif

                return true;

        } else {
            // Might just need to break until out of socket_client loop.
#ifdef DEBUGGING
            Serial.println(F("Invalid handshake"));
#endif
            disconnectStream();

            return false;
        }
    } else {
        return false;
    }
}

bool WebSocketClient::analyzeRequest() {
    String temp;

    int bite;
    bool foundupgrade = false;
    unsigned long intkey[2];
    String serverKey;
    char keyStart[17];
    char b64Key[25];
    String key = "------------------------";

    randomSeed(analogRead(0));

    for (int i=0; i<16; ++i) {
        keyStart[i] = (char)random(1, 256);
    }

    base64_encode(b64Key, keyStart, 16);

    for (int i=0; i<24; ++i) {
        key[i] = b64Key[i];
    }

#ifdef DEBUGGING
    Serial.println(F("Sending websocket upgrade headers"));
#endif    

    socket_client->print(F("GET "));
    socket_client->print(path);
    socket_client->print(F(" HTTP/1.1\r\n"));
    socket_client->print(F("Upgrade: websocket\r\n"));
    socket_client->print(F("Connection: Upgrade\r\n"));
    socket_client->print(F("Host: "));
    socket_client->print(host);
    socket_client->print(CRLF); 
    socket_client->print(F("Sec-WebSocket-Key: "));
    socket_client->print(key);
    socket_client->print(CRLF);
    socket_client->print(F("Sec-WebSocket-Version: 13\r\n"));
    socket_client->print(CRLF);

#ifdef DEBUGGING
    Serial.println(F("Analyzing response headers"));
#endif    

    while (socket_client->connected() && !socket_client->available()) {
        delay(100);
        Serial.println("Waiting...");
    }

    // TODO: More robust string extraction
    while ((bite = socket_client->read()) != -1) {

        temp += (char)bite;

        if ((char)bite == '\n') {
#ifdef DEBUGGING
            Serial.print("Got Header: " + temp);
#endif
            if (!foundupgrade && temp.startsWith("Upgrade: websocket")) {
                foundupgrade = true;
            } else if (temp.startsWith("Sec-WebSocket-Accept: ")) {
                serverKey = temp.substring(22,temp.length() - 2); // Don't save last CR+LF
            }
            temp = "";		
        }

        if (!socket_client->available()) {
          delay(20);
        }
    }

    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    uint8_t *hash;
    char result[21];
    char b64Result[30];

    Sha1.init();
    Sha1.print(key);
    hash = Sha1.result();

    for (int i=0; i<20; ++i) {
        result[i] = (char)hash[i];
    }
    result[20] = '\0';

    base64_encode(b64Result, result, 20);

    // if the keys match, good to go
    return serverKey.equals(String(b64Result));
}


String WebSocketClient::handleStream() {
    uint8_t msgtype;
    uint8_t bite;
    unsigned int length;
    uint8_t mask[4];
    uint8_t index;
    unsigned int i;
    bool hasMask = false;

    // String to hold bytes sent by server to client
    String socketString;

    if (socket_client->connected() && socket_client->available()) {

        msgtype = timedRead();
        if (!socket_client->connected()) {
            return socketString;
        }

        length = timedRead();

        if (length > 127) {
            hasMask = true;
            length = length & 127;
        }


        if (!socket_client->connected()) {
            return socketString;
        }

        index = 6;

        if (length == 126) {
            length = timedRead() << 8;
            if (!socket_client->connected()) {
                return socketString;
            }
            
            length |= timedRead();
            if (!socket_client->connected()) {
                return socketString;
            }   

        } else if (length == 127) {
#ifdef DEBUGGING
            Serial.println(F("No support for over 16 bit sized messages"));
#endif
            while(1) {
                // halt, can't handle this case
            }
        }

        if (hasMask) {
            // get the mask
            mask[0] = timedRead();
            if (!socket_client->connected()) {
                return socketString;
            }

            mask[1] = timedRead();
            if (!socket_client->connected()) {

                return socketString;
            }

            mask[2] = timedRead();
            if (!socket_client->connected()) {
                return socketString;
            }

            mask[3] = timedRead();
            if (!socket_client->connected()) {
                return socketString;
            }
        }

        if (hasMask) {
            for (i=0; i<length; ++i) {
                socketString += (char) (timedRead() ^ mask[i % 4]);
                if (!socket_client->connected()) {
                    return socketString;
                }
            }
        } else {
            for (i=0; i<length; ++i) {
                socketString += (char) timedRead();
                if (!socket_client->connected()) {
                    return socketString;
                }
            }            
        }

    }

    return socketString;
}

void WebSocketClient::disconnectStream() {
#ifdef DEBUGGING
    Serial.println(F("Terminating socket"));
#endif
    // Should send 0x8700 to server to tell it I'm quitting here.
    socket_client->write((uint8_t) 0x87);
    socket_client->write((uint8_t) 0x00);
    
    socket_client->flush();
    delay(10);
    socket_client->stop();
}

String WebSocketClient::getData() {
    String data;

    data = handleStream();

    return data;
}

void WebSocketClient::sendData(const char *str) {
#ifdef DEBUGGING
    Serial.print(F("Sending data: "));
    Serial.println(str);
#endif
    if (socket_client->connected()) {
        sendEncodedData(str);       
    }
}

void WebSocketClient::sendData(String str) {
#ifdef DEBUGGING
    Serial.print(F("Sending data: "));
    Serial.println(str);
#endif
    if (socket_client->connected()) {
        sendEncodedData(str);
    }
}

int WebSocketClient::timedRead() {
  while (!socket_client->available()) {
    delay(20);  
  }

  return socket_client->read();
}

void WebSocketClient::sendEncodedData(char *str) {
    int size = strlen(str);

    // string type
    socket_client->write(0x81);

    // NOTE: no support for > 16-bit sized messages
    if (size > 125) {
        socket_client->write(126);
        socket_client->write((uint8_t) (size >> 8));
        socket_client->write((uint8_t) (size && 0xFF));
    } else {
        socket_client->write((uint8_t) size);
    }

    for (int i=0; i<size; ++i) {
        socket_client->write(str[i]);
    }
}

void WebSocketClient::sendEncodedData(String str) {
    int size = str.length() + 1;
    char cstr[size];

    str.toCharArray(cstr, size);

    sendEncodedData(cstr);
}
