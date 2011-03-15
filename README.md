## Websocket Server for Arduino

This is a simple library that implements a Websocket server running on an Arduino. The Websocket specification is a moving target and this implementation is based on a [draft specification][1] which expired i February 2011. But this is the version that has support by a few browsers and so is what's usable now. The protocol will change slightly as indicated by the [current draft][2] into something that looks nicer to your eye, if you're into staring at Request Headers.

The implementation in this library has restrictions as the Arduino platform resources are quite limited. Most notably, the handshake headers are case sensitive while the specification state they should be case _insensitive_, and, after a header field name and it's trailing colon, there **must** be one and only one space before the header value while the specification only "prefers" one space but allows 0-n.

This will most likely not be a problem as current implementations in Safari, Chrome and Firefox formats the request in that particular way. See below for what it looks like.

_Header example:_

	Upgrade: WebSocket
	Connection: Upgrade
	Host: example.com
	Origin: http://example.com
	Sec-WebSocket-Key1: 4 @1  46546xW%0l 1 5
	Sec-WebSocket-Key2: 12998 5 Y3 1  .P00

	^n:ds[4U

The last line, the somewhat cryptic `^n:ds[4U` is the third key sent by the client, is always 8 bytes long and not terminated by a newline.

_Response example:_

	Upgrade: WebSocket
	Connection: Upgrade
	Sec-WebSocket-Origin: http://example.com
	Sec-WebSocket-Location: ws://example.com/demo
	Sec-WebSocket-Protocol: sample

	8jKS'y:G*Co,Wxa-

The last line is the MD5 Digest, always 16 bytes and not terminated by a newline.

The next version of the specification will get rid of those butt-ugly keys, keeping  everything readable. Or at least not looking broken like the keys above.

Other limitations and assumptions in the current implementation:

* The server requires all received data to be UTF-8 only.
* The server only accepts frames starting with 0x00 and ending with 0xFF. That is, the alternative frame definition in the websocket standard with 0xFF followed by specified length, is not supported. (If I got that part of the spec right.)
* The server assumes all data sent to client is UTF-8 only. No encoding is done by the server.
* The server handles incoming frames of limited lengths (default setting is 256). Remember, an Arduino is not an ocean of free memory cells. Don't even think about trying 2^32 - 1...

### Getting started

The example websockets.html file should be served from any web server you have access to. Remember to change the ws://... URL in it to your Arduino.

Install the library to "libraries" folder in your Arduino sketchbook folder. For example, on a mac that's `~/Documents/Arduino/libraries`.

Try one of the examples to ensure that things work.

Start playing with your own code!

### The Future

As the Websocket specification matures, the implementations in various browsers will follow and this library will need to change accordingly. I will try to keep up, but I'm not monitoring the Websocket World daily so do file an issue for attention. Or bugs! Or corrections on where I've failed to understand the specification! Or any wish!

_Enjoy!_

Oh by the way, quoting myself:

> Don't forget to place a big ***fat*** disclaimer in the README. There is most certainly bugs in the code and I may well have misunderstood some things in the specification which I have only skimmed through and not slept with. So _please_ do not use this code in appliancies where people or pets could get hurt, like space shuttles, dog tread mills and Large Hadron Colliders.


[1]: http://www.whatwg.org/specs/web-socket-protocol/ "Protol version implemented here"
[2]: http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol "Latest specification"
