/*
        Websocketduino, a websocket implementation for webduino
	Copyright 2010 Oliver Smith

	Based on a previous implementation:
	Copyright 2010 Randall Brewer
	
        Some code and concept based off of Webduino library
        Copyright 2009 Ben Combee, Ran Talbott

        Permission is hereby granted, free of charge, to any person obtaining a copy
        of this software and associated documentation files (the "Software"), to deal
        in the Software without restriction, including without limitation the rights
        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the Software is
        furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in
        all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
        OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
        THE SOFTWARE.
	
        -------------
	
        Currently based off of "The Web Socket protocol" draft (v 75):
        http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol-75
 */

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_
// CRLF characters to terminate lines/handshakes in headers.
#define CRLF "\r\n"
// Include '#define DEBUGGING true' before '#include <WebSocket.h>' in code to
// enable serial debugging output.
#ifndef DEBUGGING
#define DEBUGGING false
#endif
// Amount of time (in ms) a user may be connected before getting disconnected 
// for timing out (i.e. not sending any data to the server).
#define TIMEOUT_IN_MS 30000
#define BUFFER 32
// ACTION_SPACE is how many actions are allowed in a program. Defaults to 
// 5 unless overwritten by user.
#ifndef ACTION_SPACE
#define ACTION_SPACE 5
#endif
#define SIZE(array) (sizeof(array) / sizeof(*array))
#include <stdlib.h>

class WebSocket {
public:
    // Constructor for websocket class.
    WebSocket(uint8_t *ip, const char *urlPrefix = "/", int port = 8080);
    // Processor prototype. Processors allow the websocket server to
    // respond to input from client based on what the client supplies.
    typedef void Action(WebSocket &socket, String &socketString);
    // Start the socket listening for connections.
    void begin();
    // Handle connection requests to validate and process/refuse
    // connections.
    void connectionRequest();
    // Loop to read information from the user. Returns false if user
    // disconnects, server must disconnect, or an error occurs.
    void socketStream(int socketBufferLink);
    // Adds each action to the list of actions for the program to run.
    void addAction(Action *socketAction);
    // Custom write for actions.
    void actionWrite(const char *str);
private:
    Server socket_server;
    Client socket_client;

    const char *socket_urlPrefix;
    byte ipAddress[4];
    int port;

    bool socket_reading;

    struct ActionPack {
        Action *socketAction;
        // String *socketString;
    } socket_actions[ACTION_SPACE];
    int socket_actions_population;

    // Discovers if the client's header is requesting an upgrade to a
    // websocket connection.
    bool analyzeRequest(int bufferLength);
    // Disconnect user gracefully.
    void disconnectStream();
    // Send handshake header to the client to establish websocket
    // connection.
    void sendHandshake();
    // Essentially a panic button to close all sockets currently open.
    // Ideal for use with an actual button or as a safetey measure.
    // void socketReset();
    // Returns true if the action was executed. It is up to the user to
    // write the logic of the action.
    void executeActions(String socketString);
};

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
    int bufferLength = BUFFER;
    int socketBufferLength = BUFFER;

    // If there is a connected client.
    if (socket_client.connected()) {
        // Check and see what kind of request is being sent. If an upgrade
        // field is found in this function, the function sendHanshake(); will
        // be called.
#if DEBUGGING
        Serial.println("*** Client connected. ***");
#endif
        if (analyzeRequest(bufferLength)) {
            // Websocket listening stuff.
            // Might not work as intended since it may execute the stream
            // continuously rather than calling the function once. We'll see.
#if DEBUGGING
            Serial.println("*** Analyzing request. ***");
#endif
            // while(socket_reading) {
#if DEBUGGING
            Serial.println("*** START STREAMING. ***");
#endif
            socketStream(socketBufferLength);
#if DEBUGGING
            Serial.println("*** DONE STREAMING. ***");
#endif
            // }
        } else {
            // Might just need to break until out of socket_client loop.
#if DEBUGGING
            Serial.println("*** Stopping client connection. ***");
#endif
            disconnectStream();
        }
    }
}

bool WebSocket::analyzeRequest(int bufferLength) {
    // Use TextString ("String") library to do some sort of read() magic here.
    String headerString = String(bufferLength);
    char bite;

#if DEBUGGING
    Serial.println("*** Building header. ***");
#endif
    while ((bite = socket_client.read()) != -1) {
        headerString += bite;
    }

#if DEBUGGING
    Serial.println("*** DUMPING HEADER ***");
    Serial.println(headerString);
    Serial.println("*** END OF HEADER ***");
#endif

    if(strstr(&headerString[0], "Upgrade: WebSocket")) {
#if DEBUGGING
    Serial.println("*** Upgrade connection! ***");
#endif
    sendHandshake();
#if DEBUGGING
    Serial.println("*** SETTING SOCKET READ TO TRUE! ***");
#endif
    socket_reading = true;
    return true;
    }
    //else {
    //	#if DEBUGGING
    //		Serial.println("Header did not match expected headers. Disconnecting client.");
    //	#endif
    //	return false;
    //}
}

// This will probably have args eventually to facilitate different needs.

void WebSocket::sendHandshake() {

	//convert ip address for sending to socket, not used loop as too much of a faff
  	char ipadd[7];
	
	ipadd[0]=ipAddress[0];
	ipadd[1]='.';
	ipadd[2]=ipAddress[1];
	ipadd[3]='.';
	ipadd[4]=ipAddress[2];
	ipadd[5]='.';
	ipadd[6]=ipAddress[3];

#if DEBUGGING  
    Serial.println("*** Sending handshake. ***");
#endif
    socket_client.write("HTTP/1.1 101 Web Socket Protocol Handshake");
    socket_client.write(CRLF);
    socket_client.write("Upgrade: WebSocket");
    socket_client.write(CRLF);
    socket_client.write("Connection: Upgrade");
    socket_client.write(CRLF);
    socket_client.write("WebSocket-Origin: null");
    socket_client.write(CRLF);
    socket_client.write("WebSocket-Location: ws://");
    
    //Assign buffer for conversions
    char buf5[10];
	
    //Write the ip address
    for(int i=0; i<4; i++)
    {
	socket_client.write(itoa(ipAddress[i],buf5,10));
	if(i!=3)
	{
	    socket_client.write('.');
	}
    }
    socket_client.write(":");
    socket_client.write(itoa(port,buf5,10));
    socket_client.write("/");
    socket_client.write(CRLF);
    socket_client.write(CRLF);
#if DEBUGGING
    Serial.println("*** Handshake done. ***");
#endif
}

void WebSocket::socketStream(int socketBufferLength) {
    while (socket_reading) {
        char bite;
        // String to hold bytes sent by client to server.
        String socketString = String(socketBufferLength);
        // Timeout timeframe variable.
        unsigned long timeoutTime = millis() + TIMEOUT_IN_MS;

        // While there is a client stream to read...
        while ((bite = socket_client.read()) && socket_reading) {
            // Append everything that's not a 0xFF byte to socketString.
            if ((uint8_t) bite != 0xFF) {
                socketString += bite;
            } else {
                // Timeout check.
                unsigned long currentTime = millis();
                if ((currentTime > timeoutTime) && !socket_client.connected()) {
#if DEBUGGING
                    Serial.println("*** CONNECTION TIMEOUT! ***");
#endif
                    disconnectStream();
                }
            }
        }
        // Assuming that the client sent 0xFF, we need to process the String.
        // NOTE: Removed streamWrite() in favor of executeActions(socketString);
        executeActions(socketString);
    }
}

void WebSocket::addAction(Action *socketAction) {
#if DEBUGGING
    Serial.println("*** ADDING ACTIONS***");
#endif
    if (socket_actions_population <= SIZE(socket_actions)) {
        socket_actions[socket_actions_population++].socketAction = socketAction;
    }
}

void WebSocket::disconnectStream() {
#if DEBUGGING
    Serial.println("*** TERMINATING SOCKET ***");
#endif
    socket_reading = false;
    socket_client.flush();
    socket_client.stop();
    socket_client = false;
#if DEBUGGING
    Serial.println("*** SOCKET TERMINATED! ***");
#endif
}

void WebSocket::executeActions(String socketString) {
    int i;
#if DEBUGGING
    Serial.print("*** EXECUTING ACTIONS ***");
    Serial.print(socket_actions_population);
    Serial.print("\n");
#endif
    for (i = 0; i < socket_actions_population; ++i) {
#if DEBUGGING
        Serial.print("* Action ");
        Serial.print(i);
        Serial.print("\n");
#endif
        //trim data from before start of message
        String substring = socketString.substring(2);
        socket_actions[i].socketAction(*this, substring);
    }
}

void WebSocket::actionWrite(const char *str) {
#if DEBUGGING
    Serial.println(str);
#endif
    if (socket_client.connected()) {
        socket_client.write((uint8_t) 0x00);
        socket_client.write(str);
        socket_client.write((uint8_t) 0xFF);
    }

}

#endif
