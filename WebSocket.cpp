#include "WebSocket.h"
#include "MD5.c"

#define DEBUGGING

WebSocket::WebSocket(const char *urlPrefix, int inPort) :
    socket_server(inPort),
    socket_client(255),
    socket_actions_population(0),
    socket_urlPrefix(urlPrefix)
{
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
                origin = temp.substring(8,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Host: ")) {
                host = temp.substring(6,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Sec-WebSocket-Key1")) {
                key[0]=temp.substring(20,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Sec-WebSocket-Key2")) {
                key[1]=temp.substring(20,temp.length() - 2); // Don't save last CR+LF
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
        // What now is in temp should be the third key
        temp.toCharArray(key3, 9);
        
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
        socket_client.print("HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
        socket_client.print("Upgrade: WebSocket\r\n");
        socket_client.print("Connection: Upgrade\r\n");
        socket_client.print("Sec-WebSocket-Origin: ");        
        socket_client.print(origin);
        socket_client.print(CRLF);
        
        // The "Host:" value should be used as location
        socket_client.print("Sec-WebSocket-Location: ws://");
        socket_client.print(host);
        socket_client.print(socket_urlPrefix);
        socket_client.print(CRLF);
        socket_client.print(CRLF);
        
        socket_client.write(md5Digest, 16);

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
    // TODO: Check if I understood this properly
    socket_client.write((uint8_t) 0xFF);
    socket_client.write((uint8_t) 0x00);
    
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
        socket_client.print((uint8_t) 0x00); // Frame start
        socket_client.print(str);
        socket_client.print((uint8_t) 0xFF); // Frame end
    }
}
