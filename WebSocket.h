/*
	Websocketduino, a websocket implementation for webduino
	Copyright 2010 Randall Brewer
	
	Concept based off of Webduino library
	by Ben Combee and Ran Talbott
	(Copyright 2009)

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
*/

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_
// CRLF characters to terminate lines/handshakes in headers.
#define CRLF "\r\n"
// Include '#define DEBUGGING true' before '#include <Websocket.h>' in code to
// enable serial debugging output.
#ifndef DEBUGGING
	#define DEBUGGING false
#endif
// Amount of time (in ms) a user may be connected before getting disconnected 
// for timing out (i.e. not sending any data to the server).
#define TIMEOUT_IN_MS 30000

#include <string.h>
#include <stdlib.h>

class WebSocket {
	public:
		// Constructor for websocket class.
		WebSocket(const char *urlPrefix = "/", int port = 8080);
		// Start the socket listening for connections.
		void begin();
		// Handle connection requests to validate and process/refuse
		// connections.
		void connectionRequest();
		// Loop to read information from the user. Returns false if user
		// disconnects, server must disconnect, or an error occurs.
		void socketStream(int socketBufferLink);
	private:
		Server socket_server;
		Client socket_client;
		
		const char *socket_urlPrefix;
		// bool socket_connected;
		bool socket_reading;
		bool socket_initialized;
		
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
		// For writing data from the server to the client.
		void streamWrite(String socketString);
};

WebSocket::WebSocket(const char *urlPrefix, int port) :
	socket_server(port),
	socket_client(255),
	socket_urlPrefix(urlPrefix)
{}

void WebSocket::begin() {
	socket_server.begin();
}

void WebSocket::connectionRequest () {
	// This pulls any connected client into an active stream.
	socket_client = socket_server.available();
	int bufferLength = 32;
	int socketBufferLength = 32;
	
	// If there is a connected client.
	if(socket_client.connected()) {
		// Check and see what kind of request is being sent. If an upgrade
		// field is found in this function, the function sendHanshake(); will
		// be called.
		#if DEBUGGING
		Serial.println("*** Client connected. ***");
		#endif
		if(analyzeRequest(bufferLength)) {
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
	while((bite = socket_client.read()) != -1) {
		headerString.append(bite);
	}
	
	#if DEBUGGING
		Serial.println("*** DUMPING HEADER ***");
		Serial.println(headerString.getChars());
		Serial.println("*** END OF HEADER ***");
	#endif
	
	if(headerString.contains("Upgrade: WebSocket")) {
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
	else {
		#if DEBUGGING
			Serial.println("Header did not match expected headers. Disconnecting client.");
		#endif
		return false;
	}
}

// This will probably have args eventually to facilitate different needs.
void WebSocket::sendHandshake() {
	#if DEBUGGING
		Serial.println("*** Sending handshake. ***");
	#endif
	socket_client.write("HTTP/1.1 101 Web Socket Protocol Handshake");
	socket_client.write(CRLF);
	socket_client.write("Upgrade: WebSocket");
	socket_client.write(CRLF);
	socket_client.write("Connection: Upgrade");
	socket_client.write(CRLF);
	socket_client.write("WebSocket-Origin: file://");
	socket_client.write(CRLF);
	socket_client.write("WebSocket-Location: ws://192.168.1.170:8080/");
	socket_client.write(CRLF);
	socket_client.write(CRLF);
	#if DEBUGGING
		Serial.println("*** Handshake done. ***");
	#endif
}

void WebSocket::socketStream(int socketBufferLength) {
	while(socket_reading) {
		char bite;
		// String to hold bytes sent by client to server.
		String socketString = String(socketBufferLength);
		unsigned long timeoutTime = millis() + TIMEOUT_IN_MS;
	
		// While there is a client stream to read...
		while((bite = socket_client.read()) && socket_reading) {
			// Append everything that's not a 0xFF byte to socketString.
			if((uint8_t)bite != 0xFF) {
				socketString.append(bite);
			} else {
				unsigned long currentTime = millis();
				if(currentTime > timeoutTime) {
					#if DEBUGGING
						Serial.println("*** CONNECTION TIMEOUT! ***");
					#endif
					disconnectStream();
				}
			}
		}
		// Assuming that the client sent 0xFF, we need to process the String.
		// TODO: Need to allow custom commands to be supplied to handle string 
		//		 data.
		streamWrite(socketString);
	}
}

void WebSocket::streamWrite(String socketString) {
	#if DEBUGGING
		Serial.println("*** Socket String Dump. ***");
		Serial.println(socketString.getChars());
		Serial.println("*** End Socket String Dump. ***");
	#endif
	// Writes the recieved string back to the user. Basically an echo.
	socket_client.write((uint8_t)0x00);
	socket_client.write(socketString.getChars());
	socket_client.write((uint8_t)0xFF);
}

void WebSocket::disconnectStream() {
	#if DEBUGGING
		Serial.println("*** TERMINATING SOCKET ***");
	#endif
	socket_reading = false;
	socket_initialized = false;
	socket_client.flush();
	socket_client.stop();
	socket_client = false;
	#if DEBUGGING
		Serial.println("*** SOCKET TERMINATED! ***");
	#endif
}

#endif