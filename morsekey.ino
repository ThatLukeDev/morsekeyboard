#include <string>
#include <map>

#include <Arduino.h>
#include "BLEDevice.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"

#define LED_PIN 2
#define TOUCH_PIN 13
#define WPM 20

unsigned long dividertime = 4000 / WPM;
bool buttonDown = false;
bool modifier = false;
uint8_t modifierv = 0b0000;
unsigned long lastTime = 0;
std::string built = "";

std::map<std::string, char> morses = {
	{ ".-",	  'a' },
	{ "-...",	'b' },
	{ "-.-.",	'c' },
	{ "-..",	'd' },
	{ ".",		'e' },
	{ "..-.",	'f' },
	{ "--.",	'g' },
	{ "....",	'h' },
	{ "..",		'i' },
	{ ".---",	'j' },
	{ "-.-",	'k' },
	{ ".-..",	'l' },
	{ "--",		'm' },
	{ "-.",		'n' },
	{ "---",	'o' },
	{ ".--.",	'p' },
	{ "--.-",	'q' },
	{ ".-.",	'r' },
	{ "...",	's' },
	{ "-",		't' },
	{ "..-",	'u' },
	{ "...-",	'v' },
	{ ".--",	'w' },
	{ "-..-",	'x' },
	{ "-.--",	'y' },
	{ "--..",	'z' },
	{ "-----",	'0' },
	{ ".----",	'1' },
	{ "..---",	'2' },
	{ "...--",	'3' },
	{ "....-",	'4' },
	{ ".....",	'5' },
	{ "-....",	'6' },
	{ "--...",	'7' },
	{ "---..",	'8' },
	{ "----.",	'9' },
	{ ".-.-.-",	'.' },
	{ "--..--",	',' },
	{ "..--..",	'?' },
	{ ".-..-.",	'"' },
	{ "-....-",	'-' },
	{ "-...-",	'=' },
	{ "---...",	':' },
	{ "-.-.-.",	';' },
	{ "-.-.--",	'!' },
	{ "-..-.",	'/' },
	{ "-.--.",	'(' },
	{ "-.--.-",	')' },
	{ "----",	16 }, // Modifier
};

void bluetoothKeyTask(void*);

struct InputReport {
  uint8_t modifiers; // 0b1 = ctrl, 0b10 = shift, 0b100 = alt
  uint8_t reserved;
  uint8_t pressedKeys[6];
};

const uint8_t REPORT_DESCRIPTOR_MAP[] = {
  USAGE_PAGE(1),      0x01, // Generic Desktop Controls
  USAGE(1),           0x06, // Keyboard
  COLLECTION(1),      0x01, // Application
  REPORT_ID(1),       0x01, // Report ID (1)
  USAGE_PAGE(1),      0x07, // Keyboard/Keypad
  USAGE_MINIMUM(1),   0xE0, // Keyboard Left Control
  USAGE_MAXIMUM(1),   0xE7, // Keyboard Right Control
  LOGICAL_MINIMUM(1), 0x00, // Each bit is either 0 or 1
  LOGICAL_MAXIMUM(1), 0x01,
  REPORT_COUNT(1),    0x08, // 8 bits for the modifier keys
  REPORT_SIZE(1),     0x01,
  HIDINPUT(1),        0x02, // Data, Var, Abs
  REPORT_COUNT(1),    0x01, // 1 byte (unused)
  REPORT_SIZE(1),     0x08,
  HIDINPUT(1),        0x01, // Const, Array, Abs
  REPORT_COUNT(1),    0x06, // 6 bytes (for up to 6 concurrently pressed keys)
  REPORT_SIZE(1),     0x08,
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x65, // 101 keys
  USAGE_MINIMUM(1),   0x00,
  USAGE_MAXIMUM(1),   0x65,
  HIDINPUT(1),        0x00, // Data, Array, Abs
  REPORT_COUNT(1),    0x05, // 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),     0x01,
  USAGE_PAGE(1),      0x08, // LEDs
  USAGE_MINIMUM(1),   0x01, // Num Lock
  USAGE_MAXIMUM(1),   0x05, // Kana
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,
  HIDOUTPUT(1),       0x02, // Data, Var, Abs
  REPORT_COUNT(1),    0x01, // 3 bits (Padding)
  REPORT_SIZE(1),     0x03,
  HIDOUTPUT(1),       0x01, // Const, Array, Abs
  END_COLLECTION(0)         // End application collection
};

bool connected = false;
BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;

const InputReport NONEKEY = { };

class BleKeyboardCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    ((BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902)))->setNotifications(true);
    connected = true;

    Serial.println("\nConnected\n");
  }

  void onDisconnect(BLEServer* server) {
    ((BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902)))->setNotifications(false);
    connected = false;

    Serial.println("\nDisconnected\n");
  }
};

void bluetoothKeyTask(void*) {
  BLEDevice::init("Morse Keyboard");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new BleKeyboardCallbacks());

  hid = new BLEHIDDevice(server);
  input = hid->inputReport(1);
  output = hid->outputReport(1);

  hid->manufacturer()->setValue("Nil");
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00, 0x02);

  BLESecurity* security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_BOND);

  hid->reportMap((uint8_t*)REPORT_DESCRIPTOR_MAP, sizeof(REPORT_DESCRIPTOR_MAP));
  hid->startServices();

  hid->setBatteryLevel(100);

  BLEAdvertising* advertising = server->getAdvertising();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->addServiceUUID(hid->deviceInfo()->getUUID());
  advertising->addServiceUUID(hid->batteryService()->getUUID());
  advertising->start();

  Serial.println("\nReady\n");

  delay(portMAX_DELAY);
};

void keyPress(uint8_t val) {
  if (!connected)
    return;
  if (val > KEYMAP_SIZE)
    return;

  KEYMAP map = keymap[val];
  
  InputReport report = {
    .modifiers = modifierv,
    .reserved = 0,
    .pressedKeys = {
      map.usage,
      0, 0, 0, 0, 0
    }
  };

  Serial.print((char)val);
  Serial.print(" ");
  Serial.println(val);

  input->setValue((uint8_t*)&report, sizeof(report));
  input->notify();
  delay(100);
  input->setValue((uint8_t*)&NONEKEY, sizeof(NONEKEY));
  input->notify();
}

void setup() {
  Serial.begin(9600);

  xTaskCreate(bluetoothKeyTask, "bluetooth", 20000, NULL, 5, NULL);
}

void loop() {
  bool currentDown = touchRead(TOUCH_PIN) < 50;

  if (currentDown != buttonDown) {
    if (buttonDown) {
      if (millis() - lastTime < dividertime) {
        built += ".";
      }
      else {
        built += "-";
      }
    }

    buttonDown = currentDown;
    lastTime = millis();
  }
  else if (!buttonDown && millis() - lastTime > dividertime) {
    char c = 0;

    if (morses.find(built) != morses.end()) {
      c = morses[built];
    }

    if (c == 16) {
      modifier = true;
    }
    else if (c != 0) {
      if (modifier) {
        switch (c) {
          case 'e':
            keyPress((uint8_t)32); // space
            break;
          case 'i':
            keyPress((uint8_t)8); // backspace
            break;
          case 's':
            modifierv = (uint8_t)0b0001; // control
            keyPress((uint8_t)'['); // escape
            modifierv = (uint8_t)0b0000; // control
            break;
          case 'h':
            keyPress((uint8_t)0); // 
            break;
          case '5':
            keyPress((uint8_t)10); // enter
            break;
          case 't':
            modifierv = (uint8_t)0b0010; // shift
            break;
          case 'm':
            modifierv = (uint8_t)0b0001; // control
            break;
          case 'o':
            modifierv = (uint8_t)0b0100; // alt
            break;
        }
      }
      else {
        keyPress((uint8_t)c);
        modifierv = 0b000;
      }

      modifier = false;
    }

    built = "";
    lastTime = millis();
  }
}