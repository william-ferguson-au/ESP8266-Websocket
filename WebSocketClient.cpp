//#define DEBUGGING

//MS:

#include <esp_log.h>
#include "global.h"
#include "WebSocketClient.h"

#include "sha1.h"
#include "Base64.h"

static const char* TAG = "WebSocketClient";

bool WebSocketClient::handshake(Client &client) {

    socket_client = &client;

    ESP_LOGI(TAG, "Start WebSocket handshake");

    // If there is a connected client->
    if (socket_client->connected()) {
        // Check request and look for websocket handshake
      ESP_LOGI(TAG, "Client connected");

      if (analyzeRequest()) {
        ESP_LOGI(TAG, "Websocket established");
        return true;
      } else {
        // Might just need to break until out of socket_client loop.
        ESP_LOGI(TAG, "Invalid handshake");
        disconnectStream();
        return false;
      }
    } else {
        ESP_LOGI(TAG, "Client not connected");
        return false;
    }
}

bool WebSocketClient::analyzeRequest() {
    String temp;

    int bite;
    bool foundupgrade = false;
    unsigned long intkey[2];
    String serverKey;
    char keyStart[17];
    char b64Key[25];
    String key = "------------------------";

    randomSeed(analogRead(0));

    ESP_LOGI(TAG, "analyzeRequest");

    for (int i=0; i<16; ++i) {
        keyStart[i] = (char)random(1, 256);
    }

    base64_encode(b64Key, keyStart, 16);

    for (int i=0; i<24; ++i) {
        key[i] = b64Key[i];
    }

    ESP_LOGI(TAG, "Sending websocket upgrade headers");
    ESP_LOGI(TAG, "socket_client#connected=%d", socket_client->connected());

    ESP_LOGI(TAG, "Sending websocket upgrade headers #2");
    if (socket_client->connected())
    {
      ESP_LOGI(TAG, "socket_client#connected=true");
    }
    else
    {
      ESP_LOGI(TAG, "socket_client#connected=false");
    }
    ESP_LOGI(TAG, "socket_client#connected=%u", socket_client->connected());

    ESP_LOGI(TAG, "connecting to host=%s", host);
    ESP_LOGI(TAG, "connecting to path=%s", host);
    ESP_LOGI(TAG, "connecting to protocol=%s", protocol);

    socket_client->print(F("GET "));
    ESP_LOGI(TAG, "After first print");


    socket_client->print(path);
    socket_client->print(F(" HTTP/1.1\r\n"));
    socket_client->print(F("Upgrade: websocket\r\n"));
    socket_client->print(F("Connection: Upgrade\r\n"));
    socket_client->print(F("Host: "));
    socket_client->print(host);
    socket_client->print(CRLF); 
    ESP_LOGI(TAG, "print #2");

    socket_client->print(F("Sec-WebSocket-Key: "));
    socket_client->print(key);
    socket_client->print(CRLF);
    socket_client->print(F("Sec-WebSocket-Protocol: "));
    socket_client->print(protocol);
    socket_client->print(CRLF);
    ESP_LOGI(TAG, "print #3");
    socket_client->print(F("Sec-WebSocket-Version: 13\r\n"));
    socket_client->print(CRLF);

    ESP_LOGI(TAG, "Analyzing response headers");

    while (socket_client->connected() && !socket_client->available()) {
        delay(100);
        Serial.println("Waiting...");
    }

    // TODO: More robust string extraction
    while ((bite = socket_client->read()) != -1) {

        temp += (char)bite;

        if ((char)bite == '\n') {
            ESP_LOGI(TAG, "Got Header: %s", temp.c_str());

            if (!foundupgrade && temp.startsWith("Upgrade: websocket")) {
                foundupgrade = true;
            } else if (temp.startsWith("Sec-WebSocket-Accept: ")) {
                serverKey = temp.substring(22,temp.length() - 2); // Don't save last CR+LF
            }
            temp = "";		
        }

        if (!socket_client->available()) {
          delay(20);
        }
    }

    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    uint8_t *hash;
    char result[21];
    char b64Result[30];

    SHA1Context sha;
    int err;
    uint8_t Message_Digest[20];
    
    err = SHA1Reset(&sha);
    err = SHA1Input(&sha, reinterpret_cast<const uint8_t *>(key.c_str()), key.length());
    err = SHA1Result(&sha, Message_Digest);
    hash = Message_Digest;

    for (int i=0; i<20; ++i) {
        result[i] = (char)hash[i];
    }
    result[20] = '\0';

    base64_encode(b64Result, result, 20);

    // if the keys match, good to go
    return serverKey.equals(String(b64Result));
}


bool WebSocketClient::handleStream(String& data, uint8_t *opcode) {
    uint8_t msgtype;
    uint8_t bite;
    unsigned int length;
    uint8_t mask[4];
    uint8_t index;
    unsigned int i;
    bool hasMask = false;

    ESP_LOGI(TAG, "Start handleStream");

    if (!socket_client->connected() || !socket_client->available())
    {
        return false;
    }      

    msgtype = timedRead();
    if (!socket_client->connected()) {
        return false;
    }

    length = timedRead();

    if (length & WS_MASK) {
        hasMask = true;
        length = length & ~WS_MASK;
    }


    if (!socket_client->connected()) {
        return false;
    }

    index = 6;

    if (length == WS_SIZE16) {
        length = timedRead() << 8;
        if (!socket_client->connected()) {
            return false;
        }
            
        length |= timedRead();
        if (!socket_client->connected()) {
            return false;
        }   

    } else if (length == WS_SIZE64) {
        ESP_LOGW(TAG, "No support for over 16 bit sized messages");
        return false;
    }

    if (hasMask) {
        // get the mask
        mask[0] = timedRead();
        if (!socket_client->connected()) {
            return false;
        }

        mask[1] = timedRead();
        if (!socket_client->connected()) {

            return false;
        }

        mask[2] = timedRead();
        if (!socket_client->connected()) {
            return false;
        }

        mask[3] = timedRead();
        if (!socket_client->connected()) {
            return false;
        }
    }
        
    data = "";
        
    if (opcode != NULL)
    {
      *opcode = msgtype & ~WS_FIN;
    }
                
    if (hasMask) {
        for (i=0; i<length; ++i) {
            data += (char) (timedRead() ^ mask[i % 4]);
            if (!socket_client->connected()) {
                return false;
            }
        }
    } else {
        for (i=0; i<length; ++i) {
            data += (char) timedRead();
            if (!socket_client->connected()) {
                return false;
            }
        }            
    }
    
    return true;
}

void WebSocketClient::disconnectStream() {
    ESP_LOGI(TAG, "Terminating socket");

    // Should send 0x8700 to server to tell it I'm quitting here.
    socket_client->write((uint8_t) 0x87);
    socket_client->write((uint8_t) 0x00);
    
    socket_client->flush();
    delay(10);
    socket_client->stop();
}

bool WebSocketClient::getData(String& data, uint8_t *opcode) {
    ESP_LOGI(TAG, "#getData");
    return handleStream(data, opcode);
}    

void WebSocketClient::sendData(const char *str, uint8_t opcode) {
  ESP_LOGI(TAG, "Sending data - 1");
    ESP_LOGI(TAG, "Sending data: %s", str);
    if (socket_client->connected()) {
        sendEncodedData(str, opcode);       
    }
}

void WebSocketClient::sendData(String str, uint8_t opcode) {
    ESP_LOGI(TAG, "Sending data - 2");
    ESP_LOGI(TAG, "Sending data: %s", str.c_str());
    ESP_LOGI(TAG, "Sending data - 3");
    ESP_LOGI(TAG, "Sending data client#connected=%d", socket_client->connected());
    if (socket_client->connected()) {
        sendEncodedData(str, opcode);
    }
}

int WebSocketClient::timedRead() {
  ESP_LOGI(TAG, "Starting timedRead");
  while (!socket_client->available()) {
    delay(20);  
  }

  return socket_client->read();
}

void WebSocketClient::sendEncodedData(char *str, uint8_t opcode) {
    uint8_t mask[4];
    int size = strlen(str);

    ESP_LOGI(TAG, "sendEncodedData -1");
    ESP_LOGI(TAG, "sendEncodedData '%s'", str);

    // Opcode; final fragment
    socket_client->write(opcode | WS_FIN);

    // NOTE: no support for > 16-bit sized messages
    if (size > 125) {
        socket_client->write(WS_SIZE16 | WS_MASK);
        socket_client->write((uint8_t) (size >> 8));
        socket_client->write((uint8_t) (size & 0xFF));
    } else {
        socket_client->write((uint8_t) size | WS_MASK);
    }

    mask[0] = random(0, 256);
    mask[1] = random(0, 256);
    mask[2] = random(0, 256);
    mask[3] = random(0, 256);
    
    socket_client->write(mask[0]);
    socket_client->write(mask[1]);
    socket_client->write(mask[2]);
    socket_client->write(mask[3]);
     
    for (int i=0; i<size; ++i) {
        socket_client->write(str[i] ^ mask[i % 4]);
    }
}

void WebSocketClient::sendEncodedData(String str, uint8_t opcode) {
    ESP_LOGI(TAG, "sendEncodedData - 2");
    int size = str.length() + 1;
    char cstr[size];

    ESP_LOGI(TAG, "sendEncodedData - 3");
    str.toCharArray(cstr, size);
    ESP_LOGI(TAG, "sendEncodedData - 4");

    sendEncodedData(cstr, opcode);
}
