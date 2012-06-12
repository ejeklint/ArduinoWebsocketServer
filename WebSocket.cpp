#include "WebSocket.h"
#include "sha1.h"
#include "Base64.h"

//#define DEBUG

struct {
    bool isMasked;
    bool isFinal;
    byte opcode;
    byte mask[4];
    byte length;
    char data[64];
} frame;

WebSocket::WebSocket(const char *urlPrefix, int inPort) :
    socket_server(inPort),
    socket_client(255),
    socket_urlPrefix(urlPrefix)
{
}

void WebSocket::begin() {
    socket_server.begin();
    Sha1.init();
}


void WebSocket::connectionRequest() {
    // This pulls any connected client into an active stream.
    socket_client = socket_server.available();

    // If there is a connected client.
    if (socket_client.connected()) {
        // Check request and look for websocket handshake
        #ifdef DEBUG
            Serial.println("Client connected");
        #endif
        if (analyzeRequest()) {
            #ifdef DEBUG
                Serial.println("Websocket established");
            #endif
            socketStream();
            #ifdef DEBUG
                Serial.println("Websocket dropped");
            #endif
            // Always disconnect if socket is closed for some reason
            disconnectStream();
        } else {
            // Might just need to break until out of socket_client loop.
            #ifdef DEBUG
                Serial.println("Disconnecting client");
            #endif
            disconnectStream();
        }
    }
}


bool WebSocket::analyzeRequest() {
    // Use String library to do some sort of read() magic here.
    char temp[60];
    char key[60];
    char bite;
    
    bool hasUpgrade = false;
    bool hasConnection = false;
    bool isSupportedVersion = false;
    bool hasHost = false;
    bool hasOrigin = false;
    bool hasKey = false;
    
#ifdef DEBUG
    Serial.println("Analyzing request headers");
#endif
    
    byte counter = 0;
    while ((bite = socket_client.read()) != -1) {
        temp[counter++] = bite;

        if (bite == '\n') { // EOL got, temp should now contain a header string
            temp[counter - 2] = 0; // Terminate string before CRLF
            
        #ifdef DEBUG
            Serial.print("Got Line: ");
            Serial.println(temp);
        #endif
            // Ignore case when comparing and allow 0-n whitespace after ':'. See the spec:
            // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
            if (!hasUpgrade && strstr(temp, "Upgrade:")) {
                // OK, it's a websockets handshake for sure
                hasUpgrade = true;	
            } else if (!hasConnection && strstr(temp, "Connection: ")) {
                hasConnection = true;
            } else if (!hasOrigin && strstr(temp, "Origin:")) {
                hasOrigin = true;
            } else if (!hasHost && strstr(temp, "Host: ")) {
                hasHost = true;
            } else if (!hasKey && strstr(temp, "Sec-WebSocket-Key: ")) {
                hasKey = true;
                strtok(temp, " ");
                strcpy(key, strtok(NULL, " "));
            } else if (!isSupportedVersion && strstr(temp, "Sec-WebSocket-Version: ") && strstr(temp, "13")) {
                isSupportedVersion = true;
            }
            
            counter = 0; // Start saving new header string
        }
    }

    // Assert that we have all headers that are needed. If so, go ahead and
    // send response headers.
    if (hasUpgrade && hasConnection && isSupportedVersion && hasHost && hasOrigin && hasKey) {
        strcat(key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"); // Add the omni-valid GUID
        Sha1.print(key);
        uint8_t *hash = Sha1.result();
        char acceptHash[30];
        base64_encode(acceptHash, (char*)hash, 20);
        
#ifdef DEBUG  
        Serial.println("Sending response header");
        Serial.print("Sec-WebSocket-Accept: ");
        Serial.println(acceptHash);
#endif
        socket_client.print("HTTP/1.1 101 Switching Protocols\r\n");
        socket_client.print("Upgrade: websocket\r\n");
        socket_client.print("Connection: Upgrade\r\n");
        socket_client.print("Sec-WebSocket-Accept: ");        
        socket_client.print(acceptHash);
        socket_client.print(CRLF);
        socket_client.print(CRLF);
        
        return true;
    } else {
        // Nope, failed handshake. Disconnect
#ifdef DEBUG
        Serial.println("Handshake failed");
#endif
        return false;
    }
}


void WebSocket::socketStream() {
    byte bite;
    
    while (socket_client.connected()) {
        if (socket_client.available()) {
            //
            // Extract the complete frame
            //
            
            // Get opcode
            bite = socket_client.read();
            frame.opcode = bite & 0xf; // Opcode
            frame.isFinal = bite & 0x80; // Final frame?
            // Determine length (only accept <= 64 for now)
            bite = socket_client.read();
            frame.length = bite & 0x7f; // Length of payload
            if (frame.length > 64) {
                socket_client.write((uint8_t) 0x08);
                socket_client.write((uint8_t) 0x02);
                socket_client.write((uint8_t) 0x03);
                socket_client.write((uint8_t) 0xf1);
                return;
            }
            // Client should always send mask, but check just to be sure
            frame.isMasked = bite & 0x80;
            if (frame.isMasked) {
                frame.mask[0] = socket_client.read();
                frame.mask[1] = socket_client.read();
                frame.mask[2] = socket_client.read();
                frame.mask[3] = socket_client.read();
            }
            
            // Get message bytes and unmask them if necessary
            for (int i = 0; i < frame.length; i++) {
                if (frame.isMasked) {
                    frame.data[i] = socket_client.read() ^ frame.mask[i % 4];
                } else {
                    frame.data[i] = socket_client.read();
                }
            }
            
            //
            // Frame complete!
            //
            
            if (!frame.isFinal) {
                // We don't handle fragments! Close and disconnect.
                socket_client.print((uint8_t) 0x08);
                socket_client.write((uint8_t) 0x02);
                socket_client.write((uint8_t) 0x03);
                socket_client.write((uint8_t) 0xf1);
                return;
            }

            switch (frame.opcode) {
                case 0x01: // Txt frame
                    // Call the user provided function
                    callback(*this, frame.data, frame.length);
                    break;
                    
                case 0x08:
                    // Close frame. Answer with close and terminate tcp connection
                    // TODO: Receive all bytes the client might send before closing? No?
                    socket_client.write((uint8_t) 0x08);
                    return;
                    break;
                    
                default:
                    // Unexpected. Ignore. Probably should blow up entire universe here, but who cares.
                    break;
            }
        }
    }
}


void WebSocket::registerCallback(Callback *aCallback) {
    callback = aCallback;
}


void WebSocket::disconnectStream() {
    socket_client.flush();
    delay(1);
    socket_client.stop();
}


void WebSocket::send(char *data, byte length) {
    if (socket_client.connected()) {
        socket_client.write((uint8_t) 0x81); // Txt frame opcode
        socket_client.write((uint8_t) length); // Length of data
        for (int i = 0; i < length ; i++) {
            socket_client.write(data[i]);
        }
    }
}
