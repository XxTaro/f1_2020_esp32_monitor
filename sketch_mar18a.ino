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
const int ENGINE_RPM_BYTE_POS_IN_PLAYER_INFO = 16;

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
        uint8_t* packetData = packet.data();
        int playerIndex = (int)packet.data()[PLAYER_INDEX_POS];
        int playerInfoIndex = PLAYER_INFO_LENGHT * playerIndex;
        //CarTelemetryData   
        if (packet.length() == 1307) {
          lcd.setCursor(0, 0);
          lcd.printf("%d KM/H ", getSpeed(packetData, playerInfoIndex));

          lcd.setCursor(0, 1);
          lcd.printf("%d RPM ", getEngineRpm(packetData, playerInfoIndex));

          lcd.setCursor(0, 2);
          char* gear = getGear(packetData, playerInfoIndex);
          lcd.printf("%s GEAR", getGear(packetData, playerInfoIndex));
          free(gear);

          lcd.setCursor(0, 3);
          lcd.printf("DRS %s ", getDrs(packetData, playerInfoIndex));
          
        }

        if (packet.length() == 251) {
          int totalLaps = getTotalLaps(packetData);
          lcd.setCursor(17, 0);
          lcd.printf("/%02d", totalLaps);
        }

        if (packet.length() == 1190) {
          int currentLap = getCurrentLap(packetData, playerInfoIndex);
          int carPosition = getCarPosition(packetData, playerInfoIndex);
          lcd.setCursor(11, 0);
          lcd.printf("LAP %02d", currentLap);
          lcd.setCursor(11, 1);
          lcd.printf("POS %02d", carPosition);
        }

        if (packet.length() == 1213) {
          int totalCars = getTotalActiveCars(packetData);
          lcd.setCursor(17, 1);
          lcd.printf("/%02d", totalCars);
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

int getEngineRpm(uint8_t* packetData, int playerInfoIndex) {
  int rpmPos1 = PACKET_HEADER_LENGHT + playerInfoIndex + 16;
  int rpmPos2 = PACKET_HEADER_LENGHT + playerInfoIndex + 16 + 1;
  char* rpmInHexChar = (char*)malloc(4*sizeof(char));
  sprintf(rpmInHexChar, "%X%X\0", packetData[rpmPos2], packetData[rpmPos1]); //LITTLE ENDIAN
  return hexToDecimal(rpmInHexChar);
}

int getTotalLaps(uint8_t* packetData) {
  int totalLapsPos = PACKET_HEADER_LENGHT + 3;
  return (int)packetData[totalLapsPos];
}

int getCurrentLap(uint8_t* packetData, int playerInfoIndex) {
  int currentLapPos = PACKET_HEADER_LENGHT + playerInfoIndex + 45;
  return (int)packetData[currentLapPos];
}

int getCarPosition(uint8_t* packetData, int playerInfoIndex) {
  int carPositionPos = PACKET_HEADER_LENGHT + playerInfoIndex + 44;
  return (int)packetData[carPositionPos];
}

int getTotalActiveCars(uint8_t* packetData) {
  int activeCarsPos = PACKET_HEADER_LENGHT;
  return (int)packetData[activeCarsPos];
}

const char* getDrs(uint8_t* packetData, int playerInfoIndex) {
  int drsPos = PACKET_HEADER_LENGHT + playerInfoIndex + 18;
  return (int)packetData[drsPos] == 0 ? "OFF" : "ON";
}

unsigned int hexToDecimal(const char* hex) {
    unsigned int decimal = 0;

    for (int i = 0; hex[i] != '\0'; i++) {
        char currentChar = hex[i];

        if (currentChar >= '0' && currentChar <= '9') {
            decimal = decimal * 16 + (currentChar - '0');
        } else if (currentChar >= 'A' && currentChar <= 'F') {
            decimal = decimal * 16 + (currentChar - 'A' + 10);
        } else if (currentChar >= 'a' && currentChar <= 'f') {
            decimal = decimal * 16 + (currentChar - 'a' + 10);
        }
    }

    return decimal;
}

void loop() {}