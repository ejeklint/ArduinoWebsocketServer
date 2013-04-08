#include "WebSocket.h"
#include "sha1.h"
#include "Base64.h"

//#define DEBUG



struct Frame {
    bool isMasked;
    bool isFinal;
    byte opcode;
    byte mask[4];
    byte length;
    char data[64];
} frame;


WebSocket::WebSocket(const char *urlPrefix, int inPort) :
    server(inPort),
//    client(255),
    socket_urlPrefix(urlPrefix)
{
    state = DISCONNECTED;
    onConnect = NULL;
    onData = NULL;
    onDisconnect = NULL;
}


void WebSocket::begin() {
    server.begin();
}


void WebSocket::listen() {
	EthernetClient cli;
    if (cli = server.available()) {
        if (cli == true) {
            if (state == DISCONNECTED ) {
				client = cli;
                if (doHandshake() == true) {
                    state = CONNECTED;
                    if (onConnect) {
                        onConnect(*this);
                    }
                }
            } else {
                if (getFrame() == false) {
                    // Got unhandled frame, disconnect
	            	#ifdef DEBUG
	                	Serial.println("Disconnecting");
	            	#endif
                    disconnectStream();
                    state = DISCONNECTED;
                    if (onDisconnect) {
                        onDisconnect(*this);
                    }
                }
            }
        }
    }
}


bool WebSocket::isConnected() {
	return (state == CONNECTED) ? true : false;
}


void WebSocket::disconnectStream() {
    client.flush();
    delay(1);
    client.stop();
}


bool WebSocket::doHandshake() {
    char temp[60];
    char key[80];
    char bite;
    
    bool hasUpgrade = false;
    bool hasConnection = false;
    bool isSupportedVersion = false;
    bool hasHost = false;
    bool hasOrigin = false;
    bool hasKey = false;

    byte counter = 0;
    while ((bite = client.read()) != -1) {
        temp[counter++] = bite;

        if (bite == '\n') { // EOL got, temp should now contain a header string
            temp[counter - 2] = 0; // Terminate string before CRLF
            
            #ifdef DEBUG
                Serial.print("Got header: ");
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
        Sha1.init();
        Sha1.print(key);
        uint8_t *hash = Sha1.result();
        base64_encode(temp, (char*)hash, 20);
        client.print("HTTP/1.1 101 Switching Protocols\r\n");
        client.print("Upgrade: websocket\r\n");
        client.print("Connection: Upgrade\r\n");
        client.print("Sec-WebSocket-Accept: ");
        client.print(temp);
        client.print(CRLF);
        client.print(CRLF);
    } else {
        // Nope, failed handshake. Disconnect
        return false;
    }
    
    return true;
}


bool WebSocket::getFrame() {
    byte bite;
    
    // Get opcode
    bite = client.read();
        
    frame.opcode = bite & 0xf; // Opcode
    frame.isFinal = bite & 0x80; // Final frame?
    // Determine length (only accept <= 64 for now)
    bite = client.read();
    frame.length = bite & 0x7f; // Length of payload
    if (frame.length > 64) {
        #ifdef DEBUG
            Serial.print("Too big frame to handle. Length: ");
            Serial.println(frame.length);
        #endif
        client.write((uint8_t) 0x08);
        client.write((uint8_t) 0x02);
        client.write((uint8_t) 0x03);
        client.write((uint8_t) 0xf1);
        return false;
    }
    // Client should always send mask, but check just to be sure
    frame.isMasked = bite & 0x80;
    if (frame.isMasked) {
        frame.mask[0] = client.read();
        frame.mask[1] = client.read();
        frame.mask[2] = client.read();
        frame.mask[3] = client.read();
    }
    
    // Get message bytes and unmask them if necessary
    for (int i = 0; i < frame.length; i++) {
        if (frame.isMasked) {
            frame.data[i] = client.read() ^ frame.mask[i % 4];
        } else {
            frame.data[i] = client.read();
        }
    }
    
    //
    // Frame complete!
    //
    
    if (!frame.isFinal) {
        // We don't handle fragments! Close and disconnect.
        #ifdef DEBUG
            Serial.println("Non-final frame, doesn't handle that.");
        #endif
        client.print((uint8_t) 0x08);
        client.write((uint8_t) 0x02);
        client.write((uint8_t) 0x03);
        client.write((uint8_t) 0xf1);
        return false;
    }

    switch (frame.opcode) {
        case 0x01: // Txt frame
            // Call the user provided function
            if (onData)
                onData(*this, frame.data, frame.length);
            break;
            
        case 0x08:
            // Close frame. Answer with close and terminate tcp connection
            // TODO: Receive all bytes the client might send before closing? No?
            #ifdef DEBUG
                Serial.println("Close frame received. Closing in answer.");
            #endif
            client.write((uint8_t) 0x08);
            return false;
            break;
            
        default:
            // Unexpected. Ignore. Probably should blow up entire universe here, but who cares.
    		#ifdef DEBUG
        		Serial.println("Unhandled frame ignored.");
    		#endif
			return false;
            break;
    }
    return true;
}


void WebSocket::registerConnectCallback(Callback *callback) {
    onConnect = callback;
}
void WebSocket::registerDataCallback(DataCallback *callback) {
    onData = callback;
}
void WebSocket::registerDisconnectCallback(Callback *callback) {
    onDisconnect = callback;
}


bool WebSocket::send(char *data, byte length) {
	if (state == CONNECTED) {
        server.write((uint8_t) 0x81); // Txt frame opcode
        server.write((uint8_t) length); // Length of data
        for (int i = 0; i < length ; i++) {
            server.write(data[i]);
        }
		delay(1);
        return true;
    }
#ifdef DEBUG
    Serial.println("No connection to client, no data sent.");
#endif
	
    return false;
}
