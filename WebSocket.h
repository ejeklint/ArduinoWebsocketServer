/*
Websocket-Arduino, a websocket implementation for Arduino
Copyright 2011 Per Ejeklint

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
Now based off
http://www.whatwg.org/specs/web-socket-protocol/

- OLD -
Currently based off of "The Web Socket protocol" draft (v 75):
http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol-75
*/

#if ARDUINO >= 100
  #include <Arduino.h> // Arduino 1.0
#else
  #include <WProgram.h> // Arduino 0022
#endif
#include <stdlib.h>

#include <SPI.h>
#include <Ethernet.h>

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

// CRLF characters to terminate lines/handshakes in headers.
#define CRLF "\r\n"

// Amount of time (in ms) a user may be connected before getting disconnected 
// for timing out (i.e. not sending any data to the server).
#define TIMEOUT_IN_MS 10000
#define BUFFER_LENGTH 32

// ACTION_SPACE is how many actions are allowed in a program. Defaults to 
// 5 unless overwritten by user.
#ifndef CALLBACK_FUNCTIONS
#define CALLBACK_FUNCTIONS 1
#endif

// Don't allow the client to send big frames of data. This will flood the Arduinos
// memory and might even crash it.
#ifndef MAX_FRAME_LENGTH
#define MAX_FRAME_LENGTH 256
#endif

#define SIZE(array) (sizeof(array) / sizeof(*array))

class WebSocket {
public:
    // Constructor for websocket class.
    WebSocket(const char *urlPrefix = "/", int inPort = 80);
    
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
    void sendData(const char *str);

private:
    Server socket_server;
    Client socket_client;

    const char *socket_urlPrefix;

    String origin;
    String host;

    struct ActionPack {
        Action *socketAction;
        // String *socketString;
    } socket_actions[CALLBACK_FUNCTIONS];

    int socket_actions_population;

    // Discovers if the client's header is requesting an upgrade to a
    // websocket connection.
    bool analyzeRequest(int bufferLength);
    
    // Disconnect user gracefully.
    void disconnectStream();
    
    // Returns true if the action was executed. It is up to the user to
    // write the logic of the action.
    void executeActions(String socketString);
};



#endif