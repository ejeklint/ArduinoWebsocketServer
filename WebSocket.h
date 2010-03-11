#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#define CRLF "\r\n"

#include <string.h>
#include <stdlib.h>

class WebSocket {
	public:
		// Different kinds of requests.
		enum requestType { INVALID, GET, HEAD, POST, UPGRADE };
		// Constructor for websocket class.
		WebSocket(const char *urlPrefix = "/", int port = 8080);
		// Start the socket listening for connections.
		void begin();
		// Handle connection requests to validate and process/refuse
		// connections.
		void connectionRequest();
		// Loop to read information from the user.
		// void socketStream();
		// Read all input from client sent to server during stream.
		// void read();
	private:
		Server socket_server;
		Client socket_client;
		
		const char *socket_urlPrefix;
		
		bool socket_connected;
		bool socket_reading;
		
		int socket_port;

		// Discovers if the client's header is requesting an upgrade to a
		// websocket connection.
		bool analyzeRequest(int bufferLength);
		// Disconnect user gracefully.
		// void disconnectStream();
		// Send handshake header to the client to establish websocket 
		// connection.
		void sendHandshake();
		// Essentially a panic button to close all sockets currently open.
		// Ideal for use with an actual button or as a safetey measure.
		// void socketReset();
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
	
	// If there is a connected client.
	if(socket_client) {
		// Check and see what kind of request is being sent. If an upgrade
		// field is found in this function, the function sendHanshake(); will
		// be called.
		if(analyzeRequest(bufferLength)) {
			// Websocket listening stuff.
		} else {
			// Might just need to break until out of socket_client loop.
			socket_client.stop();
		}
	}
}

bool WebSocket::analyzeRequest(int bufferLength) {
	// Use TextString ("String") library to do some sort of read() magic here.
	String headerString = String(bufferLength);
	int bite;
	
	while((bite = socket_client.read()) != -1) {
		headerString.append(bite);
	}
	
	if(headerString.contains("Upgrade: WebSocket")) {
		#if DEBUGGING
			Serial.println("Upgrade!");
		#endif
		sendHandshake();
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
}

#endif