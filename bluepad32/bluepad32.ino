#include <Bluepad32.h>

struct __attribute__((packed)) JoyPacket {
  uint8_t header; // 0xAA
  uint8_t length; // number of payload bytes
  uint16_t buttons;
  uint8_t misc;
  uint8_t dpad;
  int16_t lx;
  int16_t ly;
  int16_t rx;
  int16_t ry;
  uint8_t crc;
};

ControllerPtr myControllers[BP32_MAX_GAMEPADS];
unsigned long lastDataTime[BP32_MAX_GAMEPADS] = {0};
const unsigned long DATA_TIMEOUT_MS = 500;

uint8_t computeCRC(uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (int i = 0; i < len; i++) {
    crc ^= data[i];
  }
  return crc;
}

void sendZeroPacket() {
  JoyPacket pkt;
  pkt.header = 0xAA;
  pkt.length = 12;
  pkt.buttons = 0;
  pkt.misc = 0;
  pkt.dpad = 0;
  pkt.lx = 0;
  pkt.ly = 0;
  pkt.rx = 0;
  pkt.ry = 0;

  pkt.crc = computeCRC((uint8_t *)&pkt, sizeof(pkt) - 1);

  Serial.write((uint8_t *)&pkt, sizeof(pkt));
}

void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      myControllers[i] = ctl;
      lastDataTime[i] = millis();
      return;
    }
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      myControllers[i] = nullptr;
      lastDataTime[i] = 0;
      sendZeroPacket();
      return;
    }
  }
}

void processGamepad(ControllerPtr ctl, int index) {
  JoyPacket pkt;
  pkt.header = 0xAA;
  pkt.length = 12;

  pkt.buttons = ctl->buttons();
  pkt.misc = ctl->miscButtons();
  pkt.dpad = ctl->dpad();
  pkt.lx = ctl->axisX();
  pkt.ly = ctl->axisY();
  pkt.rx = ctl->axisRX();
  pkt.ry = ctl->axisRY();

  pkt.crc = computeCRC((uint8_t *)&pkt, sizeof(pkt) - 1);

  Serial.write((uint8_t *)&pkt, sizeof(pkt));
  lastDataTime[index] = millis();
}

void processControllers() {
  bool anyControllerActive = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    auto ctl = myControllers[i];
    if (ctl && ctl->isConnected() && ctl->hasData()) {
      if (ctl->isGamepad()) {
        processGamepad(ctl, i);
        anyControllerActive = true;
      }
    }
  }

  if (!anyControllerActive) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
      if (myControllers[i] != nullptr && lastDataTime[i] > 0) {
        unsigned long timeSinceLastData = millis() - lastDataTime[i];
        if (timeSinceLastData > DATA_TIMEOUT_MS) {
          sendZeroPacket();
          lastDataTime[i] = 0;
          break;
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  delay(200);

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);
}

void loop() {

  bool dataUpdated = BP32.update();

  if (dataUpdated) {
    processControllers();
  } else {
    unsigned long now = millis();
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
      if (myControllers[i] != nullptr && lastDataTime[i] > 0) {
        unsigned long timeSinceLastData = now - lastDataTime[i];
        if (timeSinceLastData > DATA_TIMEOUT_MS) {
          sendZeroPacket();
          lastDataTime[i] = 0;
          break;
        }
      }
    }
  }

  delay(5);
}
