//#define DEBUGGING
//#define SUPPORT_HIXIE_76

#include "global.h"
#include "WebSocket.h"

#ifdef SUPPORT_HIXIE_76
#include "MD5.c"
#endif

#include "sha1.h"
#include "base64.h"

WebSocket::WebSocket(const char *urlPrefix) :
    socket_actions_population(0),
    socket_urlPrefix(urlPrefix)
{

}


void WebSocket::connectionRequest(Client &client) {

    socket_client = &client;

    // If there is a connected client->
    if (socket_client->connected()) {
        // Check request and look for websocket handshake
#ifdef DEBUGGING
            Serial.println(F("Client connected"));
#endif
        if (analyzeRequest(BUFFER_LENGTH)) {
#ifdef DEBUGGING
                Serial.println(F("Websocket established"));
#endif

            if (hixie76style) {

#ifdef SUPPORT_HIXIE_76
                handleHixie76Stream(BUFFER_LENGTH);
#endif
            } else {
                handleStream(BUFFER_LENGTH);
                delay(20);

            }

#ifdef DEBUGGING
            Serial.println(F("Websocket dropped"));
#endif
        } else {
            // Might just need to break until out of socket_client loop.
#ifdef DEBUGGING
            Serial.println(F("Disconnecting client"));
#endif
            disconnectStream();
        }
    }
}

bool WebSocket::analyzeRequest(int bufferLength) {
    // Use String library to do some sort of read() magic here.
    String temp;

    int bite;
    bool foundupgrade = false;
    String oldkey[2];
    unsigned long intkey[2];
    String newkey;

    hixie76style = false;
    
#ifdef DEBUGGING
    Serial.println(F("Analyzing request headers"));
#endif

    // TODO: More robust string extraction
    while ((bite = socket_client->read()) != -1) {

        temp += (char)bite;

        if ((char)bite == '\n') {
#ifdef DEBUGGING
            Serial.print("Got Line: " + temp);
#endif
            // TODO: Should ignore case when comparing and allow 0-n whitespace after ':'. See the spec:
            // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
            if (!foundupgrade && temp.startsWith("Upgrade: WebSocket")) {
                // OK, it's a websockets handshake for sure
                foundupgrade = true;
                hixie76style = true;
            } else if (!foundupgrade && temp.startsWith("Upgrade: websocket")) {
                foundupgrade = true;
                hixie76style = false;
            } else if (temp.startsWith("Origin: ")) {
                origin = temp.substring(8,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Host: ")) {
                host = temp.substring(6,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Sec-WebSocket-Key1: ")) {
                oldkey[0]=temp.substring(20,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Sec-WebSocket-Key2: ")) {
                oldkey[1]=temp.substring(20,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Sec-WebSocket-Key: ")) {
                newkey=temp.substring(19,temp.length() - 2); // Don't save last CR+LF
            }
            temp = "";		
        }

        if (!socket_client->available()) {
          delay(20);
        }
    }

    Serial.println("DONE WITH HEADERS!");

    if (!socket_client->connected()) {
        return false;
    }

    temp += 0; // Terminate string

    // Assert that we have all headers that are needed. If so, go ahead and
    // send response headers.
    if (foundupgrade == true) {

    Serial.println("FOUND UPGRADE!");

#ifdef SUPPORT_HIXIE_76
        if (hixie76style && host.length() > 0 && oldkey[0].length() > 0 && oldkey[1].length() > 0) {
            // All ok, proceed with challenge and MD5 digest
            char key3[9] = {0};
            // What now is in temp should be the third key
            temp.toCharArray(key3, 9);

            // Process keys
            for (int i = 0; i <= 1; i++) {
                unsigned int spaces =0;
                String numbers;
                
                for (int c = 0; c < oldkey[i].length(); c++) {
                    char ac = oldkey[i].charAt(c);
                    if (ac >= '0' && ac <= '9') {
                        numbers += ac;
                    }
                    if (ac == ' ') {
                        spaces++;
                    }
                }
                char numberschar[numbers.length() + 1];
                numbers.toCharArray(numberschar, numbers.length()+1);
                intkey[i] = strtoul(numberschar, NULL, 10) / spaces;		
            }
            
            unsigned char challenge[16] = {0};
            challenge[0] = (unsigned char) ((intkey[0] >> 24) & 0xFF);
            challenge[1] = (unsigned char) ((intkey[0] >> 16) & 0xFF);
            challenge[2] = (unsigned char) ((intkey[0] >>  8) & 0xFF);
            challenge[3] = (unsigned char) ((intkey[0]      ) & 0xFF);	
            challenge[4] = (unsigned char) ((intkey[1] >> 24) & 0xFF);
            challenge[5] = (unsigned char) ((intkey[1] >> 16) & 0xFF);
            challenge[6] = (unsigned char) ((intkey[1] >>  8) & 0xFF);
            challenge[7] = (unsigned char) ((intkey[1]      ) & 0xFF);
            
            memcpy(challenge + 8, key3, 8);
            
            unsigned char md5Digest[16];
            MD5(challenge, md5Digest, 16);
            
            socket_client->print(F("HTTP/1.1 101 Web Socket Protocol Handshake\r\n"));
            socket_client->print(F("Upgrade: WebSocket\r\n"));
            socket_client->print(F("Connection: Upgrade\r\n"));
            socket_client->print(F("Sec-WebSocket-Origin: "));        
            socket_client->print(origin);
            socket_client->print(CRLF);
            
            // The "Host:" value should be used as location
            socket_client->print(F("Sec-WebSocket-Location: ws://"));
            socket_client->print(host);
            socket_client->print(socket_urlPrefix);
            socket_client->print(CRLF);
            socket_client->print(CRLF);
            
            socket_client->write(md5Digest, 16);

            return true;
        }
#endif

        if (!hixie76style && newkey.length() > 0) {

            // add the magic string
            newkey += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

            uint8_t *hash;
            char result[21];
            char b64Result[30];

            Sha1.init();
            Sha1.print(newkey);
            hash = Sha1.result();

            for (int i=0; i<20; ++i) {
                result[i] = (char)hash[i];
            }
            result[20] = '\0';

            base64_encode(b64Result, result, 20);

            socket_client->print(F("HTTP/1.1 101 Web Socket Protocol Handshake\r\n"));
            socket_client->print(F("Upgrade: websocket\r\n"));
            socket_client->print(F("Connection: Upgrade\r\n"));
            socket_client->print(F("Sec-WebSocket-Accept: "));
            socket_client->print(b64Result);
            socket_client->print(CRLF);
            socket_client->print(CRLF);

            return true;
        } else {
            Serial.println("WHOA!");
            Serial.println(hixie76style);
            Serial.println(newkey.length());

            return false;
        }
    } else {
        // Nope, failed handshake. Disconnect
#ifdef DEBUGGING
        Serial.println(F("Header mismatch"));
#endif
        return false;
    }
}

#ifdef SUPPORT_HIXIE_76
void WebSocket::handleHixie76Stream(int socketBufferLength) {
    int bite;
    int frameLength = 0;
    // String to hold bytes sent by client to server.
    String socketString;

    while (socket_client->connected()) {
        bite = timedRead();

        if (bite != -1) {
            if (bite == 0)
                continue; // Frame start, don't save

            if ((uint8_t) bite == 0xFF) {
                // Frame end. Process what we got.
                executeActions(socketString);
                // Reset buffer
                socketString = "";
            } else {
                socketString += (char)bite;
                frameLength++;            

                if (frameLength > MAX_FRAME_LENGTH) {
                    // Too big to handle! Abort and disconnect.
                    #ifdef DEBUGGING
                    Serial.print("Client send frame exceeding ");
                    Serial.print(MAX_FRAME_LENGTH);
                    Serial.println(" bytes");
                    #endif
                    return;
                }  
            }           
        }
    }
}

#endif

void WebSocket::handleStream(int socketBufferLength) {
    uint8_t msgtype;
    uint8_t bite;
    unsigned int length;
    uint8_t mask[4];
    uint8_t index;
    unsigned int i;

    // String to hold bytes sent by client to server.
    String socketString;

    while (socket_client->connected()) {

        if (socket_client->available()) {

            msgtype = timedRead();
            if (!socket_client->connected()) {
                return;
            }

            length = timedRead() & 127;
            if (!socket_client->connected()) {
                return;
            }

            index = 6;

            if (length == 126) {
                length = timedRead() << 8;
                if (!socket_client->connected()) {
                    return;
                }
                
                length |= timedRead();
                if (!socket_client->connected()) {
                    return;
                }   

            } else if (length == 127) {
#ifdef DEBUGGING
                Serial.println(F("No support for over 16 bit sized messages"));
#endif
                while(1) {
                    // halt, can't handle this case
                }
            }

            // get the mask
            mask[0] = timedRead();
            if (!socket_client->connected()) {
                return;
            }

            mask[1] = timedRead();
            if (!socket_client->connected()) {

                return;
            }

            mask[2] = timedRead();
            if (!socket_client->connected()) {
                return;
            }

            mask[3] = timedRead();
            if (!socket_client->connected()) {
                return;
            }

            for (i=0; i<length; ++i) {
                socketString += (char) (timedRead() ^ mask[i % 4]);
                if (!socket_client->connected()) {
                    return;
                }
            }

            executeActions(socketString);
            socketString = "";
        }

        // need this wait to prevent hanging
    }
}

void WebSocket::addAction(Action *socketAction) {
#ifdef DEBUGGING
    Serial.println(F("Adding actions"));
#endif
    if (socket_actions_population <= SIZE(socket_actions)) {
        socket_actions[socket_actions_population++].socketAction = socketAction;
    }
}

void WebSocket::disconnectStream() {
#ifdef DEBUGGING
    Serial.println(F("Terminating socket"));
#endif
    // Should send 0xFF00 to client to tell it I'm quitting here.
    // TODO: Check if I understood this properly
    socket_client->write((uint8_t) 0xFF);
    socket_client->write((uint8_t) 0x00);
    
    socket_client->flush();
    delay(10);
    socket_client->stop();
}

void WebSocket::executeActions(String socketString) {
    for (int i = 0; i < socket_actions_population; ++i) {
#ifdef DEBUGGING
        Serial.print(F("Executing Action "));
        Serial.println(i + 1);
#endif
        socket_actions[i].socketAction(*this, socketString);
    }
}

void WebSocket::sendData(const char *str) {
#ifdef DEBUGGING
    Serial.print(F("Sending data: "));
    Serial.println(str);
#endif
    if (socket_client->connected()) {
        if (hixie76style) {
            socket_client->write(0x00); // Frame start
            socket_client->print(str);
            socket_client->write(0xFF); // Frame end            
        } else {
            sendEncodedData(str);
        }         
    }
}

void WebSocket::sendData(String str) {
#ifdef DEBUGGING
    Serial.print(F("Sending data: "));
    Serial.println(str);
#endif
    if (socket_client->connected()) {
        if (hixie76style) {
            socket_client->write(0x00); // Frame start
            socket_client->print(str);
            socket_client->write(0xFF); // Frame end        
        } else {
            sendEncodedData(str);
        }
    }
}

int WebSocket::timedRead() {
  while (!socket_client->available()) {
    delay(20);  
  }

  return socket_client->read();
}

void WebSocket::sendEncodedData(char *str) {
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

void WebSocket::sendEncodedData(String str) {
    int size = str.length() + 1;
    char cstr[size];

    str.toCharArray(cstr, size);

    sendEncodedData(cstr);
}
