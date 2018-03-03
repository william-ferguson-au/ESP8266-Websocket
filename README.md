## Websocket Client and Server for ESP-32

This is a simple library that implements a Websocket client and server running on an ESP-32.

### Getting started

The example WebSocketServer.html file should be served from any web server you have access to. Remember to change the  URL in it to your Arduino. The examples are based on using a WiFly wireless card to connect. If you're using ethernet instead you'll need to swap out the client class.

Install the library to "libraries" folder in your Arduino sketchbook folder. For example, on a mac that's `~/Documents/Arduino/libraries`.

Try the examples to ensure that things work.

Start playing with your own code!

### Notes

Because of limitations of the current Arduino platform (Uno at the time of this writing), this library does not support messages larger than 65535 characters. In addition, this library only supports single-frame text frames. It currently does not recognize continuation frames, binary frames, or ping/pong frames.

### Credits
Thank you to those that have tread this path previously
- Morris Singer
- Branden Hall
- ejeklint for the excellent starting point for this library.

