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
*/

#include <Arduino.h> // Arduino 1.0
#include <stdlib.h>

#include <SPI.h>
#include <Ethernet.h>

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

// CRLF characters to terminate lines/handshakes in headers.
#define CRLF "\r\n"

// Don't allow the client to send big frames of data. This will flood the Arduinos
// memory and might even crash it.
#ifndef MAX_FRAME_LENGTH
#define MAX_FRAME_LENGTH 125
#endif

class WebSocket {
public:
    // Constructor for websocket class.
    WebSocket(const char *urlPrefix = "/", int inPort = 80);
    
    // Callback function definition.
    typedef void Callback(WebSocket &socket, char* socketString, byte frameLength);
    
    // Start the socket listening for connections.
    void begin();
    
    // Handle connection requests to validate and process/refuse
    // connections.
    void connectionRequest();
    
    // Loop to read information from the user. Returns false if user
    // disconnects, server must disconnect, or an error occurs.
    void socketStream();
    
    // Adds each action to the list of actions for the program to run.
    void registerCallback(Callback *socketAction);
    
    // Custom write for actions.
    void send(char *str, byte length);

private:
    EthernetServer socket_server;
    EthernetClient socket_client;

    const char *socket_urlPrefix;

    // Pointer to the callback function the user should provide
    Callback *callback;

    // Discovers if the client's header is requesting an upgrade to a
    // websocket connection.
    bool analyzeRequest();
    
    // Disconnect user gracefully.
    void disconnectStream();
};

#endif