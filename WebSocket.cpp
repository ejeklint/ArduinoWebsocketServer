#include "WebSocket.h"
#include "MD5.c"

#define DEBUGGING

WebSocket::WebSocket(byte ip[], const char *urlPrefix, int inPort) :
    socket_server(inPort),
    socket_client(255),
    socket_actions_population(0),
    socket_urlPrefix(urlPrefix)
{
    ipAddress[0]=ip[0];
    ipAddress[1]=ip[1];
    ipAddress[2]=ip[2];
    ipAddress[3]=ip[3];
    port=inPort;
}

void WebSocket::begin() {
    socket_server.begin();
}

void WebSocket::connectionRequest() {
    // This pulls any connected client into an active stream.
    socket_client = socket_server.available();

    // If there is a connected client.
    if (socket_client.connected()) {
        // Check request and look for websocket handshake
        #ifdef DEBUGGING
            Serial.println("Client connected");
        #endif
        if (analyzeRequest(BUFFER_LENGTH)) {
            #ifdef DEBUGGING
                Serial.println("Websocket established");
            #endif
            socketStream(BUFFER_LENGTH);
            #ifdef DEBUGGING
                Serial.println("Websocket dropped");
            #endif
        } else {
            // Might just need to break until out of socket_client loop.
            #ifdef DEBUGGING
                Serial.println("Disconnecting client");
            #endif
            disconnectStream();
        }
    }
}

bool WebSocket::analyzeRequest(int bufferLength) {
    // Use String library to do some sort of read() magic here.
    String temp = String(60);

    char bite;
    bool foundupgrade = false;
    String key[2];
    unsigned long intkey[2];
    
#ifdef DEBUGGING
    Serial.println("Analyzing request headers");
#endif
    
    // TODO: More robust string extraction
    while ((bite = socket_client.read()) != -1) {
        temp += bite;

        if (bite == '\n') {
        #ifdef DEBUGGING
            Serial.print("Got Line: " + temp);
        #endif
            // TODO: Should ignore case when comparing and allow 0-n whitespace after ':'. See the spec:
            // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
            if (!foundupgrade && temp.startsWith("Upgrade: WebSocket")) {
                // OK, it's a websockets handshake for sure
                foundupgrade = true;	
            } else if (temp.startsWith("Origin: ")) {
                origin = temp.substring(8,temp.length());
            } else if (temp.startsWith("Host: ")) {
                host = temp.substring(6,temp.length());
            } else if (temp.startsWith("Sec-WebSocket-Key1")) {
                key[0]=temp.substring(20,temp.length());
            } else if (temp.startsWith("Sec-WebSocket-Key2")) {
                key[1]=temp.substring(20,temp.length());
            }
            temp = "";		
        }
    }

    temp += 0; // Terminate string
    
    // Assert that we have all headers that are needed. If so, go ahead and
    // send response headers.
    if (foundupgrade == true && host.length() > 0 && key[0].length() > 0 && key[1].length() > 0) {
        // All ok, proceed with challenge and MD5 digest
        char key3[9] = {0};
        // The last 8 bytes of temp should contain the third key
        temp.toCharArray(key3, 9); // TODO: 8 only?
        
        // Process keys
        for (int i = 0; i <= 1; i++) {
            unsigned int spaces =0;
            String numbers;
            
            for (int c = 0; c < key[i].length(); c++) {
                char ac = key[i].charAt(c);
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
        
        // Big Endian
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
        
#ifdef DEBUGGING  
        Serial.println("Sending response header");
#endif
        socket_client.write("HTTP/1.1 101 Web Socket Protocol Handshake");
        socket_client.write(CRLF);
        socket_client.write("Upgrade: WebSocket");
        socket_client.write(CRLF);
        socket_client.write("Connection: Upgrade");
        socket_client.write(CRLF);
        socket_client.write("Sec-WebSocket-Origin: ");
        
        // TODO: Fix this mess
        char corigin[origin.length() - 1];
        origin.toCharArray(corigin, origin.length() - 1);		
        socket_client.write(corigin);
        socket_client.write(CRLF);
        
        // TODO: Clean up this mess!
        
        //Assign buffer for conversions
        char buf[10];
        
        // The "Host:" value should be used as location
        socket_client.write("Sec-WebSocket-Location: ws://");
        char cHost[host.length() - 1];
        host.toCharArray(cHost, host.length() - 1);		
        socket_client.write(cHost);
        socket_client.write(socket_urlPrefix);
        socket_client.write(CRLF);
        socket_client.write(CRLF);
        
        socket_client.write(md5Digest, 16);

        
        // Need to keep global state
        socket_reading = true;
        return true;
    } else {
        // Nope, failed handshake. Disconnect
#ifdef DEBUGGING
        Serial.println("Header mismatch");
#endif
        return false;
    }
}

void WebSocket::socketStream(int socketBufferLength) {
    char bite;
    int frameLength = 0;
    // String to hold bytes sent by client to server.
    String socketString = String(socketBufferLength);

    while (socket_client.connected()) {
        if (socket_client.available()) {
            bite = socket_client.read();
            if (bite == 0)
                continue; // Frame start, don't save
            if ((uint8_t) bite == 0xFF) {
                // Frame end. Process what we got.
                executeActions(socketString);
                // Reset buffer
                socketString = "";
            }
            
            socketString += bite;
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

/*
void WebSocket::socketStream(int socketBufferLength) {
    while (socket_reading) {
        char bite;
        int frameLength = 0;
        // String to hold bytes sent by client to server.
        String socketString = String(socketBufferLength);
        // Timeout timeframe variable.
        unsigned long timeoutTime = millis() + TIMEOUT_IN_MS;

        if (!socket_client.connected()) {
#ifdef DEBUGGING
            Serial.println("No connection!");
#endif
            socket_reading = false;
            return;
        }

        // While there is a client stream to read...
        while ((bite = socket_client.read()) && socket_reading) {
            if (bite == 0)
                continue; // Don't save this byte
            if ((uint8_t) bite == 0xFF) {
                // Frame end
                // Timeout check.
                unsigned long currentTime = millis();
                if ((currentTime > timeoutTime) && !socket_client.connected()) {
#ifdef DEBUGGING
                    Serial.println("Connection timed out");
#endif
                    disconnectStream();
                    return;
                } else {
                    break; // Break out and process the frame
                }
            }

            // Append everything but Frame Start and Frame End to socketString
            socketString += bite;
            if (++frameLength > MAX_FRAME_LENGTH) {
                // Too big to handle! Abort and disconnect.
#ifdef DEBUGGING
                Serial.print("Client send frame exceeding ");
                Serial.print(MAX_FRAME_LENGTH);
                Serial.print("bytes");
#endif
                disconnectStream();
                socket_reading = false;
                return;
            }
        }
        
        // Process the String.
        executeActions(socketString);
    }
}
*/

void WebSocket::addAction(Action *socketAction) {
#ifdef DEBUGGING
    Serial.println("Adding actions");
#endif
    if (socket_actions_population <= SIZE(socket_actions)) {
        socket_actions[socket_actions_population++].socketAction = socketAction;
    }
}

void WebSocket::disconnectStream() {
#ifdef DEBUGGING
    Serial.println("Terminating socket");
#endif
    // Should send 0xFF00 to client to tell it I'm quitting here.
    socket_client.write((uint8_t) 0xFF);
    socket_client.write((uint8_t) 0x00);
    
    socket_reading = false;
    socket_client.flush();
    delay(1);
    socket_client.stop();
}

void WebSocket::executeActions(String socketString) {
    for (int i = 0; i < socket_actions_population; ++i) {
#ifdef DEBUGGING
        Serial.print("Executing Action ");
        Serial.println(i + 1);
#endif
        socket_actions[i].socketAction(*this, socketString);
    }
}

void WebSocket::sendData(const char *str) {
#ifdef DEBUGGING
    Serial.print("Sending data: ");
    Serial.println(str);
#endif
    if (socket_client.connected()) {
        socket_client.write((uint8_t) 0x00); // Frame start
        socket_client.write(str);
        socket_client.write((uint8_t) 0xFF); // Frame end
    }
}
