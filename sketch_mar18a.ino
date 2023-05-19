#include <AsyncUDP.h>
#include <WiFi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <LiquidCrystal_I2C.h>

#define lcdColQty 20
#define lcdLineQty 4
#define lcdAddres 0x27

// WiFi network name and password
const char* ssid = "ssid";
const char* pwd = "pwd";
const int localUdpPort = 20780;
const int PLAYER_INFO_LENGHT = 58;
const int PLAYER_INDEX_POS = 22;
const int PACKET_HEADER_LENGHT = 24;
const int GEAR_BYTE_POS_IN_PLAYER_INFO = 15;

bool isFirstExecution = true;

//create UDP instance
AsyncUDP asyncUdp;
LiquidCrystal_I2C lcd(lcdAddres, lcdColQty, lcdLineQty);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pwd);

  initializeLcd();
  lcd.setCursor(0, 0);
  lcd.print("Trying connection in");
  lcd.setCursor(0, 1);
  lcd.print(ssid);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected in:");
  lcd.setCursor(0, 1);
  lcd.printf("%s:%d", WiFi.localIP().toString(), localUdpPort);
  lcd.setCursor(0, 3);
  lcd.print("Waiting data...");
  
  if (asyncUdp.listen(localUdpPort))
    asyncUdp.onPacket(
      [](AsyncUDPPacket packet) {
        if (isFirstExecution) {
          lcd.clear();
          isFirstExecution = false;
        }
        //CarTelemetryData   
        if (packet.length() == 1307) {
          uint8_t* packetData = packet.data();
          int playerIndex = (int)packet.data()[PLAYER_INDEX_POS];
          int playerInfoIndex = PLAYER_INFO_LENGHT * playerIndex;

          lcd.setCursor(0, 0);
          lcd.printf("%d KM/H ", getSpeed(packetData, playerInfoIndex));
          lcd.setCursor(14, 0);
          char* gear = getGear(packetData, playerInfoIndex);
          lcd.printf("%s GEAR", getGear(packetData, playerInfoIndex));
          free(gear);
        }
      }
    );
}

void initializeLcd() {
  lcd.init();
  lcd.clear();
  lcd.backlight();
}

void waitingGameData() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for");
  lcd.setCursor(0, 1);
  lcd.print("game telemetry");
}

char* getNumberSuffix(int value) {
   switch (value) {
    case 1:
      return "st";
    case 2:
      return "nd";
    case 3:
      return "rd";
    default:
      return "th";
  }
}

char* getGear(uint8_t* packetData, int playerInfoIndex) {
  int gearPos = PACKET_HEADER_LENGHT + playerInfoIndex + GEAR_BYTE_POS_IN_PLAYER_INFO;
  int gearValue = (int)packetData[gearPos];
  char* gearChar = (char*)malloc(2 * sizeof(char));
  switch (gearValue) {
    case 0:
      sprintf(gearChar, "%s", "N");
      return gearChar;
    case 255:
      sprintf(gearChar, "%s", "R");
      return gearChar;
    default:
      sprintf(gearChar, "%d", gearValue);
      return gearChar;
  }
}

int getSpeed(uint8_t* packetData, int playerInfoIndex) {
  int speedPos1 = PACKET_HEADER_LENGHT + playerInfoIndex;
  int speedPos2 = PACKET_HEADER_LENGHT + 1 + playerInfoIndex;
  
  return (int)packetData[speedPos1] + ((int)packetData[speedPos2] == 1 ? 256 : 0);      
}

void loop() {}