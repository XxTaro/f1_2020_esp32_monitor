#include <AsyncUDP.h>
#include <WiFi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// WiFi network name and password
const char * ssid = "ssid";
const char * pwd = "password";

// IP address to send UDP data to.
// it can be ip address of the server or 
// a network broadcast address
// here is broadcast address
const char * udpAddress = "10.0.0.120";
const int localUdpPort = 20780;
const int PLAYER_INFO_LENGHT = 58;

//create UDP instance
AsyncUDP asyncUdp;

void setup() {
  Serial.begin(115200);
  
  //Connect to the WiFi network
  WiFi.begin(ssid, pwd);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (asyncUdp.listen(localUdpPort))
    asyncUdp.onPacket(
      [](AsyncUDPPacket packet) {   
        //CarTelemetryData   
        if (packet.length() == 1307) {
          int playerIndex = (int)packet.data()[22];
          int playerInfoIndex = PLAYER_INFO_LENGHT * playerIndex;

          int speedPos1 = 24 + playerInfoIndex;
          int speedPos2 = 25 + playerInfoIndex;
          int speed = (int)packet.data()[speedPos1] + ((int)packet.data()[speedPos2] == 1 ? 256 : 0);
          printf("%d", speed);
          printf("\n");
        }
      }
    );
}

void loop() {}