#include <string>
#include <map>

#define LED_PIN 2
#define TOUCH_PIN 13
#define WPM 20

unsigned long dividertime = 4000 / WPM;
bool buttonDown = false;
unsigned long lastTime = 0;
std::string built = "";

std::map<std::string, char> morses = {
	{ ".-",	  'A' },
	{ "-...",	'B' },
	{ "-.-.",	'C' },
	{ "-..",	'D' },
	{ ".",		'E' },
	{ "..-.",	'F' },
	{ "--.",	'G' },
	{ "....",	'H' },
	{ "..",		'I' },
	{ ".---",	'J' },
	{ "-.-",	'K' },
	{ ".-..",	'L' },
	{ "--",		'M' },
	{ "-.",		'N' },
	{ "---",	'O' },
	{ ".--.",	'P' },
	{ "--.-",	'Q' },
	{ ".-.",	'R' },
	{ "...",	'S' },
	{ "-",		'T' },
	{ "..-",	'U' },
	{ "...-",	'V' },
	{ ".--",	'W' },
	{ "-..-",	'X' },
	{ "-.--",	'Y' },
	{ "--..",	'Z' },
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
	{ "----",	' ' }, // Special character, doesnt exist in International Morse Code repurposed for space (for thinking time :) )
};

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
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
    char c = ' ';

    if (morses.find(built) != morses.end()) {
      c = morses[built];
    }

    if (c != ' ') {
      Serial.print(c);
    }

    built = "";
    lastTime = millis();
  }
}