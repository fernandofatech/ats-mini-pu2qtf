#include "Common.h"
#include "Utils.h"
#include "Menu.h"
#include "Draw.h"
#include "Intro.h"
#include "Remote.h"

// Colors (RGB565): dark "shack" background, amber callsign, phosphor green CW
#define C_BG       0x0862
#define C_AMBER    0xFD20
#define C_GREEN    0x47E8
#define C_DIM      0x4A69
#define C_TEXT     0xDEFB
#define C_WF_HOT   0xFFE0

// Morse for A-Z, 0-9 and a few symbols
static const char *morseFor(char c)
{
  static const char *letters[] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
    "..-", "...-", ".--", "-..-", "-.--", "--.."
  };
  static const char *digits[] = {
    "-----", ".----", "..---", "...--", "....-", ".....",
    "-....", "--...", "---..", "----."
  };
  if(c >= 'A' && c <= 'Z') return letters[c - 'A'];
  if(c >= '0' && c <= '9') return digits[c - '0'];
  switch(c) {
    case '/': return "-..-.";
    case '?': return "..--..";
    case '.': return ".-.-.-";
    case '=': return "-...-";
  }
  return "";
}

//
// Keyer task: times the chime and the CW text while the main core animates
//
static volatile bool keyDown = false;
static volatile bool keyerAbort = false;
static volatile bool keyerDone = false;
static volatile int8_t chimeNote = -1;   // chime note being played, -1 when none
static volatile int16_t curChar = -1;    // index of the character being sent
static volatile int8_t curElement = -1;  // element of curChar being sent

static bool keyerWait(uint32_t ms)
{
  vTaskDelay(pdMS_TO_TICKS(ms));
  return !keyerAbort;
}

static void keyerTask(void *)
{
  const uint32_t dit = 1200 / INTRO_WPM;
  const char *text = INTRO_CW_TEXT;

  for(int8_t i = 0 ; i < 4 && !keyerAbort ; i++)
  {
    chimeNote = i;
    keyerWait(i == 3 ? 260 : 110);
    keyerWait(25);
  }
  chimeNote = -1;
  keyerWait(500);

  for(int16_t i = 0 ; text[i] && !keyerAbort ; i++)
  {
    curChar = i;
    if(text[i] == ' ') { keyerWait(dit * 4); continue; }

    const char *code = morseFor(text[i]);
    for(int8_t e = 0 ; code[e] && !keyerAbort ; e++)
    {
      curElement = e;
      keyDown = true;
      keyerWait(code[e] == '-' ? dit * 3 : dit);
      keyDown = false;
      keyerWait(dit);
    }
    curElement = -1;
    keyerWait(dit * 2);
  }

  keyDown = false;
  curChar = strlen(text);
  keyerDone = true;
  vTaskDelete(NULL);
}

//
// Animation
//
static void drawAntenna(int x, int y, bool on, uint32_t t)
{
  // Mast and base
  spr.drawLine(x, y, x, y + 34, C_TEXT);
  spr.drawLine(x - 8, y + 34, x + 8, y + 34, C_TEXT);
  spr.drawLine(x, y + 8, x - 6, y + 34, C_DIM);
  spr.drawLine(x, y + 8, x + 6, y + 34, C_DIM);
  spr.fillCircle(x, y, 3, on ? C_AMBER : C_DIM);

  // Radiating arcs, expanding while the key is down
  for(int r = 0 ; r < 3 ; r++)
  {
    int radius = 9 + r * 7 + (on ? (t / 40) % 7 : 0);
    uint16_t color = on ? (r == 0 ? C_AMBER : (r == 1 ? 0xC400 : 0x7A00)) : C_DIM;
    spr.drawArc(x, y, radius, radius - 1, -60, 60, color);
    spr.drawArc(x, y, radius, radius - 1, 120, 240, color);
  }
}

static void drawMorse(int x, int y, const char *code, int8_t active)
{
  for(int8_t e = 0 ; code[e] ; e++)
  {
    uint16_t color = e == active ? C_WF_HOT : (e < active || active < 0 ? C_GREEN : C_DIM);
    if(code[e] == '.') { spr.fillCircle(x + 3, y + 3, 3, color); x += 12; }
    else { spr.fillRoundRect(x, y, 20, 7, 3, color); x += 28; }
  }
}

static void drawFrame(LGFX_Sprite &wf, int32_t marqueeX, uint32_t t)
{
  const char *text = INTRO_CW_TEXT;
  int16_t ch = curChar;
  bool on = keyDown || chimeNote >= 0;

  spr.fillSprite(C_BG);

  // Header: antenna + callsign + subtitle
  drawAntenna(24, 10, on, t);
  spr.setTextDatum(TL_DATUM);
  spr.setFont(&lgfx::fonts::FreeSansBold24pt7b);
  spr.setTextColor(C_AMBER);
  spr.drawString(INTRO_CALLSIGN, 62, 4);
  spr.setFont(&lgfx::fonts::Font2);
  spr.setTextColor(C_DIM);
  spr.drawString("SHACK DE ESCUTA  *  GG66rh", 64, 47);

  // On-air lamp
  spr.fillRoundRect(266, 10, 46, 18, 4, on ? 0xF800 : 0x3000);
  spr.setFont(FONT_DEFAULT);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(on ? C_TEXT : 0x7800);
  spr.drawString("ON AIR", 289, 19);

  // CW "decoder": the text typed out as it is sent, with a blinking cursor
  spr.drawRoundRect(4, 66, 312, 40, 6, C_DIM);
  spr.setFont(&lgfx::fonts::FreeMonoBold12pt7b);
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(C_GREEN);
  char shown[48] = "";
  if(ch >= 0) { strncpy(shown, text, min<int>(ch + 1, sizeof(shown) - 1)); shown[min<int>(ch + 1, sizeof(shown) - 1)] = '\0'; }
  spr.drawString(shown, 12, 70);
  if((t / 300) & 1) spr.fillRect(12 + spr.textWidth(shown) + 2, 72, 10, 16, C_GREEN);

  // Morse pattern of the current character, or the chime "notes"
  if(chimeNote >= 0)
    for(int8_t i = 0 ; i <= chimeNote ; i++) spr.fillCircle(20 + i * 18, 99 - i * 2, 3 + i, C_AMBER);
  else if(ch >= 0 && text[ch] && text[ch] != ' ')
    drawMorse(12, 94, morseFor(text[ch]), curElement);

  // Waterfall: noise floor plus a bright trace at the tone while keyed
  wf.scroll(0, 1);
  for(int x = 0 ; x < wf.width() ; x++)
  {
    uint8_t n = esp_random() & 0x1F;
    uint16_t color = n > 27 ? 0x2A7F : (n > 22 ? 0x10AF : 0x0045);
    if(on && abs(x - 160 - (chimeNote >= 0 ? (chimeNote - 2) * 30 : 0)) < 4) color = C_WF_HOT;
    else if(on && abs(x - 160 - (chimeNote >= 0 ? (chimeNote - 2) * 30 : 0)) < 8) color = 0xFC00;
    wf.drawPixel(x, 0, color);
  }
  wf.pushSprite(&spr, 0, 112);
  spr.setFont(FONT_DEFAULT);
  spr.setTextColor(C_DIM, 0x0045);
  spr.drawString("CW 700 Hz", 4, 116);

  // Marquee
  spr.fillRect(0, 146, DISPLAY_WIDTH, 24, 0x0020);
  spr.setFont(&lgfx::fonts::FreeSans9pt7b);
  spr.setTextColor(C_TEXT);
  int32_t w = spr.textWidth(INTRO_MARQUEE);
  spr.drawString(INTRO_MARQUEE, marqueeX % w, 150);
  spr.drawString(INTRO_MARQUEE, marqueeX % w + w, 150);

  spr.pushSprite(0, 0);
}

static void drawFinale(uint32_t t)
{
  spr.fillSprite(C_BG);
  for(int i = 0 ; i < 40 ; i++)
    spr.drawPixel(esp_random() % DISPLAY_WIDTH, esp_random() % DISPLAY_HEIGHT, (esp_random() & 1) ? C_AMBER : C_GREEN);
  spr.setTextDatum(MC_DATUM);
  spr.setFont(&lgfx::fonts::FreeSansBold24pt7b);
  spr.setTextColor(C_AMBER);
  spr.drawString("73!", DISPLAY_WIDTH / 2, 62);
  spr.setFont(&lgfx::fonts::FreeSans12pt7b);
  spr.setTextColor(C_TEXT);
  spr.drawString("de " INTRO_CALLSIGN, DISPLAY_WIDTH / 2, 108);
  spr.setFont(FONT_DEFAULT);
  spr.setTextColor(C_DIM);
  spr.drawString(getVersion(true), DISPLAY_WIDTH / 2, 150);
  spr.pushSprite(0, 0);
}

bool introPlay()
{
  keyDown = false;
  keyerAbort = false;
  keyerDone = false;
  chimeNote = -1;
  curChar = -1;
  curElement = -1;
  xTaskCreatePinnedToCore(keyerTask, "keyer", 4096, NULL, 5, NULL, 0);

  LGFX_Sprite wf(&spr);
  wf.setPsram(true);
  wf.setColorDepth(16);
  wf.createSprite(DISPLAY_WIDTH, 32);
  wf.fillSprite(0x0045);

  ledcWrite(PIN_LCD_BL, currentBrt);

  uint32_t start = millis();
  int32_t marqueeX = DISPLAY_WIDTH;
  bool skipped = false;
  while(!keyerDone)
  {
    if(digitalRead(ENCODER_PUSH_BUTTON) == LOW) { keyerAbort = true; skipped = true; }
    drawFrame(wf, marqueeX, millis() - start);
    // Remote screenshots keep working during the intro (used for the docs)
    if(Serial.available() > 0 && Serial.peek() == 'c') { Serial.read(); remoteCaptureToSerial(); }
    marqueeX -= 2;
    delay(10);
  }
  wf.deleteSprite();

  if(!skipped)
  {
    drawFinale(millis());
    delay(1200);
  }

  // Wait for the button to be released so the click does not reach the menu
  while(digitalRead(ENCODER_PUSH_BUTTON) == LOW) delay(10);
  return !skipped;
}
