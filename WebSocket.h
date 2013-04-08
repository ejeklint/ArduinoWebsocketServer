/*
Websocket-Arduino, a simple websocket implementation for Arduino
Copyright 2012 Per Ejeklint

Based on previous implementations by
Copyright 2010 Ben Swanson
and
Copyright 2010 Randall Brewer
and
Copyright 2010 Oliver Smith

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
Now based off version 13
http://datatracker.ietf.org/doc/rfc6455/?include_text=1

Modified by Alexandros Baltas, 2013
www.codebender.cc

*/

#include <Arduino.h> // Arduino 1.0 or greater is required
#include <stdlib.h>

#include <SPI.h>
#include <Ethernet.h>

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

// CRLF characters to terminate lines/handshakes in headers.
#define CRLF "\r\n"

class WebSocket {
public:
    // Constructor.
    WebSocket(const char *urlPrefix = "/", int inPort = 80);
    
    // Callback functions definition.
    typedef void DataCallback(WebSocket &socket, char* socketString, byte frameLength);
    typedef void Callback(WebSocket &socket);
    
    // Start tlistening for connections.
    void begin();
    
    // Main listener for incoming data. Should be called from the loop.
    void listen();
    
    // Callbacks
    void registerDataCallback(DataCallback *callback);
    void registerConnectCallback(Callback *callback);
    void registerDisconnectCallback(Callback *callback);
    
	// Are we connected?
	bool isConnected();
	
    // Embeds data in frame and sends to client.
    bool send(char *str, byte length);

private:
    EthernetServer server;
    EthernetClient client;
    enum State {DISCONNECTED, CONNECTED} state;

    const char *socket_urlPrefix;

    // Pointer to the callback function the user should provide
    DataCallback *onData;
    Callback *onConnect;
    Callback *onDisconnect;

    // Discovers if the client's header is requesting an upgrade to a
    // websocket connection.
    bool doHandshake();
    
    // Reads a frame from client. Returns false if user disconnects, 
    // or unhandled frame is received. Server must then disconnect, or an error occurs.
    bool getFrame();

    // Disconnect user gracefully.
    void disconnectStream();
};

#endif