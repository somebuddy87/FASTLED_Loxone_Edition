//////////////////////////////////////////////////////////
//    LOXpixel! SK6812 Loxone Integration V0.1          //
//    Dennis Henning                                    //
//    https://unser-smartes-zuhause.de                  //
//    09/2019                                           //
//////////////////////////////////////////////////////////


// Credits

// Die in diesem Beispiel verwendeten Effekte sind nicht auf meinem Mist gewachsen. Ich habe diese lediglich aus verschiedenen Effekt Bibliotheken zusammengesucht und in dieses Beispiel eingefuegt. Darf gerne erweitert werden!

// Parsen des Loxone Lumitech Strings in Anlehnnung an das PicoC Skript zur HUE Integration in Loxone von Andreas Lackner https://www.loxforum.com/forum/faqs-tutorials-howto-s/7738-phillips-hue-mit-loxone-verwenden

// Farbtemperatur zu RGB Implementierung in Anlehnung an http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/


//RGBW Implementierung aktivieren
#define FASTLED_RGBW

//Verhindert in meinem Fall das Flackern / Aufblitzen der LEDs
#define FASTLED_INTERRUPT_RETRY_COUNT 0

#include <FastLED.h>


#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>


//Grundeinstellungen

//Anzahl LEDS
#define NUM_LEDS 300

//LED Pin
#define LED_PIN 5

//Hostname fuer Arduino OTA
const char* hostname = "Kueche";
//Port fuer Arduino OTA
int otaport = 8266;

//WLAN Zugangsdaten
const char* ssid = "YOUR SSID";
const char* password = "YOUR PASSWORD";

//LOXPixel! Lokaler UDP Port
unsigned int localUdpPort = 8888;

//LOXPixel! IP Einstellungen (feste IP)
IPAddress ip(192, 168, 178, 13);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);

//Start-Farben pro Wand
CRGB bgColor( 0, 0, 0);
CRGB bgColor2( 0, 0, 0);
CRGB bgColor3( 0, 0, 0);
CRGB bgColor4( 0, 0, 0);

//Effekteinstellungen

//Feuer Effekt
#define COOLING  55
#define SPARKING 120


//RIPPLE Effekt
int color;
int center = 0;
int step = -1;
int maxSteps = 16;
float fadeRate = 0.8;
int diff;

uint32_t currentBg = random(256);
uint32_t nextBg = currentBg;

uint8_t gHue = 0;
int effektNR = 0;
int pos1 = 0;
int pos2 = 0;
byte dothue = 0;


// Einstellungen ENDE


WiFiUDP Udp;

char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
char ReplyBuffer[] = "acknowledged";
bool gReverseDirection = false;

CRGBArray<NUM_LEDS> leds;


CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 targetPalette(OceanColors_p);


void setup() {

  WiFi.persistent(false);
  WiFi.disconnect(true);

  //WIFI Einstellungen
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setPort(otaport);
  ArduinoOTA.setHostname(hostname);
  Serial.println(" connected");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  Udp.begin(localUdpPort);

  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS);

  //Starthelligkeit auf 255
  FastLED.setBrightness(255);

}

void effekte()
{

  switch (effektNR) {
    case 0:
      //Farben Setzen
      //Einteilungen pro Wand
      // An dieser Stelle kann die Anzahl der Einteilungen angepasst werden

      //Wand1 LED 0 -> LED 49 , Fadingzeit 5
      fadeTowardColor( leds, 50, bgColor, 5);
      //Wand2 LED 50 -> LED 101 , Fadingzeit 5
      fadeTowardColor( leds + 51, 50, bgColor2, 5);
      //Wand3 LED 102  -> LED 202 , Fadingzeit 5
      fadeTowardColor( leds + 101, 100, bgColor3, 5);
      //Wand4 LED 202 -> LED 300 , Fadingzeit 5
      fadeTowardColor( leds + 201, 99, bgColor4, 5);

      break;
    case 1: //Rainbow
      fill_rainbow( leds, NUM_LEDS, gHue, 7);
      break;
    case 2: //Rainbow with Glitter
      fill_rainbow( leds, NUM_LEDS, gHue, 7);
      if (random8() < 80) {
        leds[ random16(NUM_LEDS) ] += CRGB::White;
      }
      break;
    case 3: //Confetti
      fadeToBlackBy(leds, NUM_LEDS, 10);
      pos1 = random16(NUM_LEDS);
      leds[pos1] += CHSV(gHue + random8(64), 200, 255);
      break;
    case 4: //Sweep
      fadeToBlackBy(leds, NUM_LEDS, 20);
      pos2 = beatsin16(13, 0, NUM_LEDS - 1);
      leds[pos2] += CHSV(gHue, 255, 192);
      break;
    case 5: //Juggle
      fadeToBlackBy(leds, NUM_LEDS, 20);
      dothue = 0;

      for (int i = 0; i < 8; i++) {
        leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
        dothue += 32;
      }
      break;
    case 6: //Fire
      Fire2012();
      break;
    case 7: //Ripple
      ripple();
      break;
    case 8: //Noise
      fillnoise8();
      break;
    default:
      break;

  }

}


CRGB parseLoxone(String loxonestring)
{

  CRGB rgbvalue(0, 0, 0);
  CRGB ctrgbvalue(0, 0, 0);

  loxonestring.remove(0, 1);
  int value = loxonestring.toInt();

  if (value < 200000000) { // RGB


    int loxone_red, loxone_green, loxone_blue;
    loxone_red = 0;
    loxone_green = 0;
    loxone_blue = 0;

    // Hinweis: value ist red + green*1000 + blue*1000000
    loxone_blue = round(value / 1000000);
    loxone_green = round((value - (loxone_blue * 1000000)) / 1000);
    loxone_red = round((value - loxone_blue * 1000000) - (loxone_green * 1000));


    rgbvalue.red = round(loxone_red * 2.55);
    rgbvalue.green = round(loxone_green * 2.55);
    rgbvalue.blue = round(loxone_blue * 2.55);

  }
  else { // LUMITECH

    float bri, ct;
    int briNorm, miredNorm;

    bri = floor((value - 200000000) / 10000); // 0-100
    ct = floor((value - 200000000) - (bri * 10000)); // Wert in Kelvin, von 2700 - 6500

    briNorm = (int) round(bri * 2.55); // 0-255
    miredNorm = (int) round(1000000 / ct); // Wert von 154 - 370

    ctrgbvalue = colorTemperatureToRGB(ct);

    rgbvalue.red = ctrgbvalue.red;
    rgbvalue.green = ctrgbvalue.green;
    rgbvalue.blue = ctrgbvalue.blue;

  }
  return rgbvalue;
}

CRGB colorTemperatureToRGB(int kelvin) {

  CRGB rgbvalue(0, 0, 0);
  float temp = kelvin / 100;

  float red, green, blue;

  if ( temp <= 66 ) {

    red = 255;

    green = temp;
    green = 99.4708025861 * log(green) - 161.1195681661;


    if ( temp <= 19) {

      blue = 0;

    } else {

      blue = temp - 10;
      blue = 138.5177312231 * log(blue) - 305.0447927307;
    }

  } else {
    red = temp - 60;
    red = 329.698727446 * pow(red, -0.1332047592);
    green = temp - 60;
    green = 288.1221695283 * pow(green, -0.0755148492 );
    blue = 255;
  }

  if (red > 255)
  {
    red = 255;
  }

  if (green > 255)
  {
    green = 255;
  }

  if (blue > 255)
  {
    blue = 255;
  }

  rgbvalue.red = round(red);
  rgbvalue.green = round(green);
  rgbvalue.blue = round (blue);

  return rgbvalue;

}

void loop() {

  ArduinoOTA.handle();

  effekte();


  int packetSize = Udp.parsePacket();
  if (packetSize) {


    //UDP Paket lesen
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");
    Serial.println(packetBuffer);

    String hexstring = packetBuffer;

    //Unterscheidung der zu steuernden Wand anhand des vorangestellten Sonderzeichen. Dies ist beliebig anpassbar

    if (hexstring.charAt(0) == '#')
    {
      bgColor = parseLoxone(hexstring);
    }

    if (hexstring.charAt(0) == '!')
    {
      bgColor2 = parseLoxone(hexstring);
    }

    if (hexstring.charAt(0) == '?')
    {
      bgColor3 = parseLoxone(hexstring);
    }

    if (hexstring.charAt(0) == '*')
    {
      bgColor4 = parseLoxone(hexstring);
    }

    for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) packetBuffer[i] = 0; // Buffer leeren

    //Sonderzeichen fuer Effektsteuerung
    if (hexstring.charAt(0) == '@')
    {
      effektNR = hexstring.charAt(1) - '0';

    }


  }

  FastLED.show();

  EVERY_N_MILLISECONDS( 20 ) {
    gHue++;
  }

  EVERY_N_SECONDS(5) {                                                      // Change the target palette to a random one every 5 seconds.
    targetPalette = CRGBPalette16(CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 192, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)));
  }

  EVERY_N_MILLIS(10) {
    nblendPaletteTowardPalette(currentPalette, targetPalette, 48);
  }


}

void nblendU8TowardU8( uint8_t& cur, const uint8_t target, uint8_t amount)
{
  if ( cur == target) return;

  if ( cur < target ) {
    uint8_t delta = target - cur;
    delta = scale8_video( delta, amount);
    cur += delta;
  } else {
    uint8_t delta = cur - target;
    delta = scale8_video( delta, amount);
    cur -= delta;
  }
}


CRGB fadeTowardColor( CRGB& cur, const CRGB& target, uint8_t amount)
{
  nblendU8TowardU8( cur.red,   target.red,   amount);
  nblendU8TowardU8( cur.green, target.green, amount);
  nblendU8TowardU8( cur.blue,  target.blue,  amount);
  return cur;
}

// Fade an entire array of CRGBs toward a given background color by a given amount
// This function modifies the pixel array in place.
void fadeTowardColor( CRGB* L, uint16_t N, const CRGB& bgColor, uint8_t fadeAmount)
{
  for ( uint16_t i = 0; i < N; i++) {
    fadeTowardColor( L[i], bgColor, fadeAmount);
  }
}

void Fire2012()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor( heat[j]);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}


void ripple() {

  if (currentBg == nextBg) {
    nextBg = random(256);
  }
  else if (nextBg > currentBg) {
    currentBg++;
  } else {
    currentBg--;
  }
  for (uint16_t l = 0; l < NUM_LEDS; l++) {
    leds[l] = CHSV(currentBg, 255, 50);         // strip.setPixelColor(l, Wheel(currentBg, 0.1));
  }

  if (step == -1) {
    center = random(NUM_LEDS);
    color = random(256);
    step = 0;
  }

  if (step == 0) {
    leds[center] = CHSV(color, 255, 255);         // strip.setPixelColor(center, Wheel(color, 1));
    step ++;
  }
  else {
    if (step < maxSteps) {
      //Serial.println(pow(fadeRate,step));

      leds[wrap(center + step)] = CHSV(color, 255, pow(fadeRate, step) * 255);     //   strip.setPixelColor(wrap(center + step), Wheel(color, pow(fadeRate, step)));
      leds[wrap(center - step)] = CHSV(color, 255, pow(fadeRate, step) * 255);     //   strip.setPixelColor(wrap(center - step), Wheel(color, pow(fadeRate, step)));
      if (step > 3) {
        leds[wrap(center + step - 3)] = CHSV(color, 255, pow(fadeRate, step - 2) * 255);   //   strip.setPixelColor(wrap(center + step - 3), Wheel(color, pow(fadeRate, step - 2)));
        leds[wrap(center - step + 3)] = CHSV(color, 255, pow(fadeRate, step - 2) * 255);   //   strip.setPixelColor(wrap(center - step + 3), Wheel(color, pow(fadeRate, step - 2)));
      }
      step ++;
    }
    else {
      step = -1;
    }
  }

  LEDS.show();
  //delay(50);
}

int wrap(int step) {
  if (step < 0) return NUM_LEDS + step;
  if (step > NUM_LEDS - 1) return step - NUM_LEDS;
  return step;
}


void one_color_allHSV(int ahue, int abright) {                // SET ALL LEDS TO ONE COLOR (HSV)
  for (int i = 0 ; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(ahue, 255, abright);
  }
}

void fillnoise8() {

#define scale 30                                                          // Don't change this programmatically or everything shakes.

  static uint16_t dist;                                                     // A random number for our noise generator.

  for (int i = 0; i < NUM_LEDS; i++) {                                      // Just ONE loop to fill up the LED array as all of the pixels change.
    uint8_t index = inoise8(i * scale, dist + i * scale);                   // Get a value from the noise function. I'm using both x and y axis.
    leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);    // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }
  dist += beatsin8(10, 1, 4);                                               // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave.
  // In some sketches, I've used millis() instead of an incremented counter. Works a treat.
} // fillnoise8()
