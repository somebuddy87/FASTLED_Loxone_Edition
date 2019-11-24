//////////////////////////////////////////////////////////
//    LOXpixel! SK6812 Loxone Integration V0.3          //
//    Dennis Henning                                    //
//    https://unser-smartes-zuhause.de                  //
//    11/2019                                           //
//////////////////////////////////////////////////////////

// Credits

// Die in diesem Beispiel verwendeten Effekte sind nicht auf meinem Mist gewachsen. Ich habe diese lediglich aus verschiedenen Effekt Bibliotheken zusammengesucht und in dieses Beispiel eingefuegt. Darf gerne erweitert werden!

// Parsen des Loxone Lumitech Strings in Anlehnnung an das PicoC Skript zur HUE Integration in Loxone von Andreas Lackner https://www.loxforum.com/forum/faqs-tutorials-howto-s/7738-phillips-hue-mit-loxone-verwenden

// Farbtemperatur zu RGB Implementierung in Anlehnung an http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/

// Das Webinterface basiert auf dem IOTWebConf Projekt : https://github.com/prampec/IotWebConf

#include <Arduino.h>
#include <IotWebConf.h>

//RGBW Implementierung aktivieren
#define FASTLED_RGBW
#define FASTLED_ESP8266_DMA

//Verhindert in meinem Fall das Flackern / Aufblitzen der LEDs
#define FASTLED_INTERRUPT_RETRY_COUNT 0

#include <FastLED.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//Grundeinstellungen

//Anzahl LEDS
#define NUM_LEDS 300

//LED Pin
//Solange FASTLED_ESP8266_DMA aktiviert ist, wird als LED PIN immer der RX Pin des Node MCU verwendet
#define LED_PIN 3

//Port fuer Arduino OTA
int otaport = 8266;
bool gradient_mode = 0;
bool needreset = false;

//LOXPixel! Lokaler UDP Port
unsigned int localUdpPort = 8888;

//Start-Farben pro Wand
CRGB bgColor(255, 0, 0);
CRGB bgColor2(0, 255, 0);
CRGB bgColor3(0, 0, 255);
CRGB bgColor4(255, 0, 0);
CRGB bgColor5(0, 255, 0);
CRGB bgColor6(0, 0, 255);
CRGB bgColor7(255, 0, 0);
CRGB bgColor8(0, 255, 0);

CRGB gradient1(0, 0, 0);
CRGB gradient2(0, 0, 0);
CRGB gradient3(0, 0, 0);
CRGB gradient4(0, 0, 0);

//Effekteinstellungen

//Feuer Effekt
#define COOLING 55
#define SPARKING 120

//Lightning Effekt

#define FPS 50
#define FLASHES 8
#define FREQUENCY 50
unsigned int dimmer = 1;

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

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "LOXpixel!";

//Hostname fuer Arduino OTA
const char* hostname = thingName;

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "loxpixel";

#define STRING_LEN 128
#define NUMBER_LEN 32

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "Lox0815"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN 2

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN

// -- Callback method declarations.
void configSaved();
boolean formValidator();
boolean connectAp(const char* apName, const char* password);
void connectWifi(const char* ssid, const char* password);

DNSServer dnsServer;
WebServer server(80);

char ipAddressValue[STRING_LEN];
char gatewayValue[STRING_LEN];
char netmaskValue[STRING_LEN];

char stringParamValue[STRING_LEN];

// For first Start!
char intParamValue[NUMBER_LEN] = "300" ;
char stripeTypeParamValue[NUMBER_LEN];
char datenpinParamValue[NUMBER_LEN];

char brightnessParamValue[NUMBER_LEN];

char floatParamValue[NUMBER_LEN];

char start1Value[NUMBER_LEN];
char length1Value[NUMBER_LEN];

char start2Value[NUMBER_LEN];
char length2Value[NUMBER_LEN];

char start3Value[NUMBER_LEN];
char length3Value[NUMBER_LEN];

char start4Value[NUMBER_LEN];
char length4Value[NUMBER_LEN];

char start5Value[NUMBER_LEN];
char length5Value[NUMBER_LEN];

char start6Value[NUMBER_LEN];
char length6Value[NUMBER_LEN];

char start7Value[NUMBER_LEN];
char length7Value[NUMBER_LEN];

char start8Value[NUMBER_LEN];
char length8Value[NUMBER_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
IotWebConfSeparator separator0 = IotWebConfSeparator("Basic Settings");
IotWebConfParameter ipAddressParam = IotWebConfParameter("IP address", "ipAddress", ipAddressValue, STRING_LEN, "text", "192.168.3.222", "192.168.3.222");
IotWebConfParameter gatewayParam = IotWebConfParameter("Gateway", "gateway", gatewayValue, STRING_LEN, "text", "192.168.3.1", "192.168.3.0");
IotWebConfParameter netmaskParam = IotWebConfParameter("Subnet mask", "netmask", netmaskValue, STRING_LEN, "text", "255.255.255.0", "255.255.255.0");

IotWebConfParameter stripeTypeParam = IotWebConfParameter("Stripe type (0=SK6812 / 1=WS2812 / 2=WS2801) (Disabled! Stripe Type has to be set in code actually)", "stripeTypeParam", stripeTypeParamValue, NUMBER_LEN, "number", "0", NULL, "min='0' max='5' step='1'");

IotWebConfSeparator separator1 = IotWebConfSeparator("Network Settings");
IotWebConfParameter intParam = IotWebConfParameter("Number of LEDs", "intParam", intParamValue, NUMBER_LEN, "number", "300", NULL, "min='1' max='2000' step='1'");
IotWebConfParameter datenpinParam = IotWebConfParameter("Data PIN (Disabled! Data pin has to be set in code actually)", "datenpinParam", datenpinParamValue, NUMBER_LEN, "number", "3", NULL, "min='1' max='50' step='1'");
IotWebConfParameter brightnessParam = IotWebConfParameter("Brightness (1-255)", "brightnessParam", brightnessParamValue, NUMBER_LEN, "number", "255", NULL, "min='1' max='255' step='1'");

// -- We can add a legend to the separator
IotWebConfSeparator separator2 = IotWebConfSeparator("LED Part 1");

IotWebConfParameter start1 = IotWebConfParameter("Start Part 1", "start1", start1Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");
IotWebConfParameter length1 = IotWebConfParameter("Length Part 1", "length1", length1Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");

IotWebConfSeparator separator3 = IotWebConfSeparator("LED Part 2");

IotWebConfParameter start2 = IotWebConfParameter("Start Part 2", "start2", start2Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");
IotWebConfParameter length2 = IotWebConfParameter("Length Part 2", "length2", length2Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");

IotWebConfSeparator separator4 = IotWebConfSeparator("LED Part 3");

IotWebConfParameter start3 = IotWebConfParameter("Start Part 3", "start3", start3Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");
IotWebConfParameter length3 = IotWebConfParameter("Length Part 3", "length3", length3Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");

IotWebConfSeparator separator5 = IotWebConfSeparator("LED Part 4");

IotWebConfParameter start4 = IotWebConfParameter("Start Part 4", "start4", start4Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");
IotWebConfParameter length4 = IotWebConfParameter("Length Part 4", "length4", length4Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");

IotWebConfSeparator separator6 = IotWebConfSeparator("LED Part 5");

IotWebConfParameter start5 = IotWebConfParameter("Start Part 5", "start5", start5Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");
IotWebConfParameter length5 = IotWebConfParameter("Length Part 5", "length5", length5Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");

IotWebConfSeparator separator7 = IotWebConfSeparator("LED Part 6");

IotWebConfParameter start6 = IotWebConfParameter("Start Part 6", "start6", start6Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");
IotWebConfParameter length6 = IotWebConfParameter("Length Part 6", "length6", length6Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");

IotWebConfSeparator separator8 = IotWebConfSeparator("LED Part 7");

IotWebConfParameter start7 = IotWebConfParameter("Start Part 7", "start7", start7Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");
IotWebConfParameter length7 = IotWebConfParameter("Length Part 7", "length7", length7Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");

IotWebConfSeparator separator9 = IotWebConfSeparator("LED Part 8");

IotWebConfParameter start8 = IotWebConfParameter("Start Part 8", "start8", start8Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");
IotWebConfParameter length8 = IotWebConfParameter("Length Part 8", "length8", length8Value, NUMBER_LEN, "number", "0", NULL, "min='0' max='2000' step='1'");

//IotWebConfParameter floatParam = IotWebConfParameter("Float param", "floatParam", floatParamValue, NUMBER_LEN, "number", "e.g. 23.4", NULL, "step='0.1'");

IPAddress ipAddress;
IPAddress gateway;
IPAddress netmask;

// -- Javascript block will be added to the header.
const char CUSTOMHTML_SCRIPT_INNER[] PROGMEM = "\n\
document.addEventListener('DOMContentLoaded', function(event) {\n\
  let elements = document.querySelectorAll('input[type=\"password\"]');\n\
  for (let p of elements) {\n\
    let btn = document.createElement('INPUT'); btn.type = 'button'; btn.value = '🔓'; btn.style.width = 'auto'; p.style.width = '83%'; p.parentNode.insertBefore(btn,p.nextSibling);\n\
    btn.onclick = function() { if (p.type === 'password') { p.type = 'text'; btn.value = '🔒'; } else { p.type = 'password'; btn.value = '🔓'; } }\n\
  };\n\
});\n";
// -- HTML element will be added inside the body element.
//const char CUSTOMHTML_BODY_INNER[] PROGMEM = "<div><img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAASwAAAB0CAMAAAAICf9pAAABS2lUWHRYTUw6Y29tLmFkb2JlLnhtcAAAAAAAPD94cGFja2V0IGJlZ2luPSLvu78iIGlkPSJXNU0wTXBDZWhpSHpyZVN6TlRjemtjOWQiPz4KPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyIgeDp4bXB0az0iQWRvYmUgWE1QIENvcmUgNS42LWMxMzggNzkuMTU5ODI0LCAyMDE2LzA5LzE0LTAxOjA5OjAxICAgICAgICAiPgogPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4KICA8cmRmOkRlc2NyaXB0aW9uIHJkZjphYm91dD0iIi8+CiA8L3JkZjpSREY+CjwveDp4bXBtZXRhPgo8P3hwYWNrZXQgZW5kPSJyIj8+IEmuOgAAAARnQU1BAACxjwv8YQUAAAABc1JHQgCuzhzpAAADAFBMVEX////9/f3h4eHx8fH19fX+/v4AAAD8/Pz7+/v5+fkXPBr39/fk5eTv7+/09PTo6Onz8/Pr6+stdTMjXShuRwru7u5ZWVnV1dXe3t4LMmTY2Njj4+MnZy1Li1HFxcWAAAD6AADq6urg4OApai/n5+fd3d1QU+Xb29vs7OxPjlWVu5jaAABWVlbBAAC1vbUqcDHf39+qq6rIycj/BQXQxRw3fj5BgdHLybO6sBi0qhcueTXLzMykmxWNtJCflxTMwhrZ2dkeHh7T09NFh0tTj1mIjIiBg4EhVyYlYithlmbl5eYfUSQbRh+PkY8vz0I+gkVGhNJrn3CWl5a4uM9bYFzjAABGScgdTCHbjxZJS9A+QLEjnTHSsbB2pXtkZGT/GBj/eHibnJtAQrc0NDQyezm8vb3BwcFZlF/mrVOys7MosjgYQRyfoJ86Ojp6fHqBrIUpbTArcswlpzQhISHtx4q4t7felB/Ozs7b49tydnK2ybgRSpJQUFAqSCy0dRHIAAApKnX/KysOPnvF08bs2sHOhhPtxIPR0tE6fND/Pz8sxj7/ZWVcj9TipUUYayH/UFC0AADpt2tvb2+IsIzDfxIWVqfhnzUqvDulvaeWqJilaw+hpKU5VDvtAABhqmhKSkqqAACGjZ7gmSqHHR1hcmNIXkrSAAByn9oUW7WEreCWAACQoZK3qqeet6CGiPCjpfP1AAAoQ2xpYw3wzMykcnKbi3zqv3rS4NSuxrHG0eOYYxB1UyW9zb975IfY3eoeasmNXA2UdGFGRkZL2lvcxKjt5eClutp8UAsVX70YZsj0np6BZkbr0appaWmwv7FiZezu4s7mlZUWY8SSRUX0sbGxmprIvray5beWsNiom43lxZuQiBLf11w2OJp6l3yZ5qFQYoLvubnAlGCP55ld2mu9xOl1fpdgboZGWXnn4YLTrYSxiYn7jY05O6JZWoh7isDce3vo442nLy9JfU3dmDNufbHTRUXYV1fnRUU5OXvf8OG4v83iwFY7TFU4h0Bje91IlfJiAAAfU0lEQVR42u18eXgTV5ZvapVKpVJJsmXZkhcky1EZTLzFnhn7+b2yvKEyjrHbBLoT23i84QA2S2y32fIMIYIGj9lCszYOS0gTSOgHBAKkG+jseyZbJ5Okk07S09vrnvWt/7xzq0qbtRja5vtm5un8Ubaq7j331q/OOfecc0/VXXclKEEJSlCCEpSgBCUoQQlKUIISlKAEJShBCUpQgv6/J0rDkkCsFktgMSVpyM9++4//+NvP3mWoO/tMGAY9FZZlGA317xUs1nTvZ7/97Wf3PkrermhhWkUmp7x3ClpaLm2+fPns7Nn7L6/d/H0yAi+MUXhFAxJwVi/e3gwpYMrO7JMh9X9172ef3fuXf0Fg4RhMNTWKudRaMXt2RROpmWrO3ObLs0OpYu33SS0VxuuydwBdmE1qIxhoSTg/e/ZA7+bbEn6KIb599tlvf8rMoH0h8f/8V//l3r/8T3/BafwYvPD1L3/59QsfTzE1jLxW1VpR0dRoYuM2YyzXKoD2tDZVVzU2ZnibBvbAz/2XQiWSYh3eriZo1LQ2QsAxcm3THrhi/sLEUreDleXZZ5599tlnfjqD9mUyWBi5+ecvfP31Cz//JRFfYjDinuxGb29Xup6Mu3xcqtizZ6CpKn2RTqH587q8rQN79uwXgs8cwOpeVlfibert1U+6N4rR98L5svoNmabbuW2MfPadZ4DeeWaK+5gWWMS1h+775puf/3C2RRt/ZeDuSco2m9OTcDIOVobLrQOt3rJ6XSgtyq9uHRgY+D4bQIvhxFrdouzG3Iz9xvB70xBnM3Ib0+franI47W3cl5b703fvfuedd+6+38HcSbAeeOi++374wGyBmS5YlNa4v7W1qbEOADrc31zY09NT2Nx/GH7VlXlbW1uvBcwdxfJFG3R15oyMKneYslGsmJGbkV+vO+lx3I4W3sU4/vXz791///c+/1xi75zNInIUsC4L05UshFVTk9cMCniyoWjY5i5wFrhtw5kNANf8lblNTU3XAhZKQxQ0g4aCaFWEqY2Wa83ILUnS6fpww21ZasZ06L/K9Os7CZaxQAFrLaeZJlga49mmpt78+TpdbZHVyVsIo8FIWHintagW0Mqu8nqbLvmlhWJM1hZdfX5GbtXmEAkC6w5nsufrakXu9rRJw4l/RFj9bzB1dwwsOCGDtXraYGHkZa+3F+Rqw3IfLhhYDUaBH6FhjYLT17FBt2hlVW9vq8tvtDHS3qdbllSSm9EaAgsjeDNyzfW6w0U8e3uLGka6PE/88Y9PFOF30MDPGFgU6/b29nbV6TYU2lzGEG8HY4wu6yGdrt6c0ZsbtOda2cavrMrNeDugnJhxP1j3ect0y523fctaoyT6fDY7dwddhxkDC6xNbjW60QabY5LfqiF5awOIUVdubtCeqza+LDfD63cfAO9qsO6LdCdswm3fMqUlOYfDYpjJMO5OgUWRm6uUG/XwbKSjyXtOgBg15lbtCYgM2Pga3fx0OLffiKlnKjKqS0A2MyXyz/DDMS3DaLEZDXdigDVdA6+1eKsySpKWbcjUR7lRjNR3b1iWVFZV1WgLsfGek0g5M6rtsjhQ7Gaw7rKLFUsJg3HjTMaAWCyu0cBa/YP7HgKwsGmARZHXSqqqwD+KcaNazl2jq1/ZWFVyIHAdM+CFumVB94GxNMnWvcXjCOqSErrKkStEneT5D17/w5IlS17/4DzJhAtRWO4pAKqWihrAhySogCvx7ptvPgz05qPvkmGhZRSwcmWwNhPTAUvDDZRUlUCME2MZQxZKt2heV1VJY9DDZiziCCgniJMPOmHk21W5jcjFsgdlk2LOjwM42z9gMLipr5aE0FdE2I1pSBQaqrEhRuzY8WOgU+GRIsZ+K58tfT4AIoT9jz4WpIcfJUN6RIKl/5EMVg5BTQMshm8saTQnzR/JiYE5RuSMzE8yN5aYN5NUAEEXsvFdyH3QQlDozchYCUroDvElMPKr9vEl20dfJ0Cqlmzfvn18tL29fXQc/tu+5HyocWRN7wBaz7zzLUJaQ5Q/vuoUwHIq1F2j2J/uOPXjUwte/ZN//aA05KPPPffcrqVDew9u3LJvF/z/8EvBZxABFispYDkN0wCLIt82l5Tk180/5IrhHwEwh+bXrSwpMR8IwqkhUpGNr8qo2kyC+w9Gr26Si4URD84p3jZa/J6J+GB8+3jxziyZdh5BeI1/FbLskvr774Y4+rvfQew1RvyJtHWrduxY8HJIIMBwOxbsWPDUpt/4LApYEHN8umvXrmNXkhV6ZctS+Plu4BlEA+shBFac+HhqsDTc0+aSruz6+TZLrGVCY7HNr0/vKjF7g0EoxThUG98kGPTVGVXpy3SFeGhojREf7XxyVnHxe/gH4+Ptp7MCNHFudHx8/AMyRAq+d//dd3/3r7+Dbgy03vNl2qZXty4ofT4APWZ4eWvpqnVpaX3qEAirpbvGNgJMb1y8ePEN+Hvl2NKlS1/ya2IkWK4ZAIsxec0lZen1J5zGWPJJGZ0n6tPLSswlrmDwFrDxvdf4y9UZ5ggXC0LXnU8Wt4++99X28SNZWccvnIHo/MMLx7OyTi9s37Zt2/kQKQgBCxjbu3+Stu5npaU7/IoISrigdOvjaWlPiKpgaQyfLl26D8TqYneRz+cr6r54Nfn60NjYLkJ7J8FipWxziTm9viFOFMtKDfXzzF3mdB8ZREOx8fm53grrgLcqKcLFgtB17pzi9m1LlowvBKiKfGKBM9Xmy7ygoDU6HiIFoWCh5feJtLTHVy0o9SuiljtVWvq3m9K+9Kh6jpGPji3ddzD56hmfW8+beL3bd+Zq8sFjY/teVOdwR8CiSF96fpd5Xv2BOMkkxlGIwMpP7zaG5kyRjU9q9LZWtKIYvCZ1kudB4gis0W2jc7KOn/E5HRxE5xxf4PvweNbEkfbR0ddVh3YyWBTrsP592qantpZu/amMDUa+XLoAlPAn3XYFPYp5aWxsbG9y8hmbiyAZhiEJl+1M8it79+0bUy1FNLBWTxss4p70/DIAqy9OnkdjuQfAKstPfzvUo1NsfHaVt6kXrHuYixUAa1Zxe/GTgJWNNyK3EZwowmX9MCvrkeLR9lG/aE0CC/nBmYoinkJZxKASutWkImb8dN++xdeTL6LwjFI8NYf1RvLBoWNDqmhFBeuHP1g9LbAw7qwMVl1RHM8WI3x1CKx5B0IXAcXG15nBXc1eFOZihYI1a87prA9tpoCBQtHmhayJheBGvGfEooJ1l5ZIPYQUsXTry8BUQ5zaipTw761qah8E61+O7dv4ylWfKxjIk1JP8vUtx4Z+p6SNI8Hi9zwAYPXapwGWxnJgngxWPGeNMqaqYIWJH7Lx4FOUlYHJq4nMYslgzToyN+v9sJgTbqvo+Gkw/cWjAZWZBBbFCDZFERdILEY+L6+EPwnYRMz4IojQleRPnCGrr4Zz37i+d2hoC85GBYtxVCDJ+pGe/fPB0gpPz1sJd1sX11kzOJPSzWUr5z0dnsCGe+qH82WgoPdIEV6aAtbCiawPw4VOQ+RcOD33XHvxufPKehEBlgyorIhbf8yR/I6tSAkPBWwiI4wNHdty/epwaLKQIvWfXN84tHjLi7JdjQTLNBts1uofSdMAixGeTkJgJcXVZRIHsMwrkyaBhRHWkrKusrKukq4veOauaGCdmzNx3BbeCzypntM7FxYXH3mPwKKDhfIahbIiLnje8fICpIS/DjomrHPL4qG9V24UGLEwH+ijKxsXL97yO5lLJFgCAuuhH7mmC5bZnD0VWPOyUaNJYGm4A41VXfn55saMqmtkVMk69+TEhclCCxrzTxNzAKx/UExgFLCQIv4aFLF0x4+tp3YgJSwK2CfK+CICa+OZcGHWcJ6Ne0GyfiXL2x0DKxtpmDPenqJhWNHVwjCbhRIz1VXZSUnpXbm5XgsTA6wPJ8+PIvELMlj/U3FXooAFgbNLVkQ5IgQl7AsmYDXc7wCsxYtf5JHXECSLOLQYgcWzdwosrVBYl468gm5jHAPv9y/uCQuJGEtTbm7+IrQb21idu98YbTWcNWfngxE7dSz/+52wUB75Hy42FligiE5ZESEkBCUMuO6yvv0KwDo29uKbj4bTi2P7hhbv3SjFAWv1tMCCa/XzwCvNfjtOngf8C5C+/KT6MP8CM+zPqG6s0504gRz56gw3S0Uz8HMfjMhNMsKDCKyFT0qxwaJAUiCifnXVqp89vuk3npAQneURWPuW7nrs4TB6bNdSBNZB2TmIChZaDacDFkZ46pPyu8ryD1hip1u1woH8srLsukWh/gXF2KtzM1D8XAiOfEl1buukfWgVrEciwdJYHpy7EF1SnJ6oYIEi8r4v17362mt/+/i6bjxEbFlp497FQ0PH9o0tDaOxY1s2Hrxy5ToeCyzwsx6YFlgQwR2uy4Yg+QtTvHBHCbZP4iFWXE67d9XrRoZ9siNfnTGpVCQOWNyDjywEczYXjwMWDOD+V8AK0Hrq/4bGYqQdwFq898orydEpOlha4TICq4JnphFIk/YTEMo0dpW44gXS+V0l5nmLakKcFMW6z1um68bxosNoryfXK4UFPHHBevLIrCOP7IwLFka8DDr46quvrVq1wxGWBD24cfGWgzGgSr4R3WZpuLUPrF79QIVjOmCxrob5SfklJWVvG2KnaNT8YGiwLVt38yJUBcI5l6NkTfWkUpG4YM05Atbsn+KBRTEQEq56at26p1aVlv4ihDOJX9m4BcAa7MmMQuWiwMQA62+AKkzTAYsRuufXZ5c0lgxwsZN/AyWNXen1y4aDTRTrnrRsQxHPgk/UAjY+ozrDF2rj49ms9xaCfZ/7vj62gUd5mQULXt2UhsIelAgMyRVeObh3y8YrwyIehSSBjOqUYsQ9CKwDwnTAwgj3YZRhb6xyxkwrF5Q0oiz9iaDLTLHu3mo57V5AaNBumS7SxiuB9JEno6yGpn84Ulw8Z+eFOKuhkpeB8Pk3aZteK12wgw8oIqm/cWXv4i0bP3IYyEhSN8UiwTL6nv7ii6en2jacKgevb1iGdroaz8bcsDjbKG//NAez9BpiIBdtqh72oLo1ebdMtvFvh9h4BaxzCyPBYvn/Nat41iMTv+djOqUoL7MAQsKfDPalpa1btaD0FwGnlJUuynHNg/F2ACPAolihwGbN4afaA55id8fUo5ufVIY2TGNshbmrM6rMdcsO2wJFcxSJrLucdkfpOLRbptj46pByQAWs4nPvTc6UUeR5OH9k52nVXY0GlpbbUVoKSviEz/fEJEVk+E+uHwRf/XfxNCoSLA1LWASO1FLTAQsjUkdQKV919Z4Ym6x7qqtRQV8zHngqjMWbizZVT9iUIEdjBBsv+/Fngzz8yb9tk1drjHgP5QQnjru5WLEhZlCSo7/x2O3lEPa8VloaUESt8NErV/YODf2LPU65TgRYqCxIy2imrBGYYkea5Xt0qJSvt3dtFBnVGNb29qKCvsPWwJNUrDtyGySS8se9cs1WdUawZksFq739q0kxNuPY1t5+bu7pC3isFA3FPl+KlDCtECcIXAl7AooIVvaNVzYO7Tv2YpygIxKsW6Qpah00RE4NKpTp9XovRaClIa95vb1ldTpdYSArhay7XElS6w6YT2TjlyEb3xQIqFWwRke3h8fYmPH10dH2OaezzvAxk39ICVFeBkJCfyIwoIgoc5V8fcu+fWMvxa67iQIWqj6ZutBCAatsZXqMKhoWpYfBUfI2tW6OKDna3NTkRcWP/WLglhXrrrgNVLAiQq7ZAmfLgIXZLHlrInSZY89vGx09N5H1vqjeSARYGuMvlB2KIlhSAonAgCIywvDV5INDY2PPcTEtUKTN0pKXPv744//D3pKB7yorcxo1kwjDKEreqdsAaFV5W1uvTSpmW9uqYHXSFwAGQ9Y9Y+Ui2W0IrYjYoNh4e2AfAvlZs4pHt237wBB4ooDV+LZtxXOzss6oOhy5YeFXwkPyABoi9YkwRcSMzk+SX9m4b+nSTy1hRRMUUCywtMavX/gavTTA3kphSElV77VL348gkwYt/Tl9CK3GptaB2ZuNrFYuk9Syxs2zB1q9EP/pTmbajQEdkby51V3gNpSHBiH+mq3qXH9drpqDn7Owfdv46yalcoZiyPPbx7e1P5mVdUPktNG3whh+h5wc/dIqJ0dREWtgjxpTKp6sb6CNr6W7Hn43UJIjT/hdBosOFkZ+/M0LQN9844pfM6eCldE0MHsyVbRehvHBPrsLN6Ay7t7WgYqzay/pjQaj/tLasxUDrb3geQJWTsIv8Rrj/txqVIvV7AzNYAVqtgI2PpBWnpg1vn3JVy+RLEsaz/8BYfVIVtb7RS4y+iarhlCVsFutF0MGMUwRQRUGryK0du167tNHOWAMrA0vvfvmw7veZamoYDGWb+77OaL7Po7vaalqWAVIoPdLQmhPa5W81mOsIPYd1oGJbmwaqKjwI1kxoLxEMJLpDNR7gnXPqM411+lOWsMLItX9fLDxatJUzZSevng865Ft25cs+cPrqEBr+/ZtxRNZWccHg6nPSdv35LeghE9tCm4TKnvUm0IUkQFVuJqcfGVo6a7nHnvs4TfflKu0HnvsubFPA2IdDhbLr37oB/ehOsnZ8ddHlOCrr6urS0rPXpkfRivTkw7Iniag5e45gd6pqDPnKu/s7BloQoZJp9vQ4cODtbFarqnLPG9R6OoY3M8HDvXp5q6zso33g/Xhh8ezshYiuFC5lmyust4/kxNkSeKf/3egz5XCEEba+rNX16WBi+UIFrE6PF+CI//Uz1Y9TyqKKLgRWskHF48huBA9t2vp2OKNvwpsr/2dTAGwpKqyBx764UMPlJ21TAWWLiapexAYa3H6DrUo7+zUpa/Mz55Xvwz92FDbY5OIwI1h5D118vkW2+SkO0VK3Ur/wyK6TTWtPHGm/Mz7gM9pVFJzZK5cTnMhM0dgQ1x95aUBGSwN8SdAKg0VzARLjkDv+uSTmx5X0kDo4WZelFMyEPocO3Zs8d6D19Gvqw4/WP9Npo/8YLme/huFfPElCwLu2GA1qzEMxRAucXj5yIawqydrezxOgQx6JxrXSeXKoBSxnaPhwGGTqR89/0BaOUcc/v3xYMlR1o1BD86FLEqkXgGrm0Dm8zsyKkgJQx4GIxeLIPq1asdYDvcN3rganst642KRX7L0Kliq4wruxj/LWB2Y4v0P8FQ8RZlRqcgXqNSH4ImTRF/R8toTLWC+NrS01Db3lVudjlBn4i4tgfuKlH7aSIdNcA8jplbJEAoWL+ht5WcuvI8Ae//G73t8Obwx1NyxJhti6UE7zhRrEofltFTYa3jodLnCWlV+ijHyOb6iTy6+8YYM0xs3Ln7SXW6TDJoInuoiUfTFP+dXHxCnKD0HoeH1eFSy8yGV+qgencfdnuFyNE7RsMeWKgnG8GpZUFeX3I+IMiY8bpfKFAvNZ7GkoAe2iGu5z+bkifB3ajWkCaanl1kCBg40V5cl3B+KHBdFxg5clNkixsMesUAS/MX0oTxVDPS24XKf2zFV2kF95ZWMmv7BwuuvSU7gJb0dt0u8iTOykwqL5RbRionDS4rlXiHJP4whCQE9MLvksBgnv34cVq2MxXhzOIx1+GztwFfv4oXQ6Ua8fQ8eHjR1cOzMvfKivk+u1GTfQox+V9xt7JBMqfwatJKbm8mXA1AhuILKlK+/Ky9ia/+NviIfO62coARYCbASYCXASoCVACtBUcCamNi5c2IiAdYtgaXEzQmwbgUsvQLWR0Tig19T0uTAP0Hxco6TAv8ExQ1yE5/dS1CCEpSgBP3bXaX+fX2A9M+Z74zdI8a20f7KIrnqJFh7EjkoptVilHKcaQwo6vbne+s+Ht3GzoTLwnAttCCXCVNai02LaZjymK4QxtasZzHlONPiUn6LDlhwvrcRPQh0CzcT33lj+BTaRfrxJxmyjTZoY050TQehVY4zChYWb9RY872NuNRFp/AzARbrSqFxDn2fl+TpFJfgaqElgtGiHTBURB767V6K7KNFgoGjTWDhErIEGqUl6KXaUN6Ogp8a9NKfRtlcl3mhskTlL4s29JSTGCa3M/BoVCTWkwalsMCXnRXeDCkp8/WPqFXmgckHpR8Wss0lN+KccGfsDHyFGIHV3UKvGDT20EAr0IFONdINHSvoNh/LsMtX0Os98ly1XMtbkhH04K2cFrAbsiSSbW0966G32tDKMKlt8k+S7uhY040CQQ26sqKQUFoWcrVr1hSi5vBjudquTx7VyTpq19BtqQyD/q63oq8ZMLXypRVcmzoip1fnyzJCLYyYiVi0AT9oIPdf72HJ5WvQBVRmrnU0rKCPHqJTJAPZIDOnpgVWHl05UltJd4vNdGfDoY5Ounm5g6crOxsa1qwZJgrp/uW7bcTIChPDWulBE0ta6T4xBewGsgQOU0tlZT/07uHkhqLBteJoRwo9TAh0Z+XuHpgwxsKVjv4+vqWSRi1bjtZ20j2kcUUNtCvkTNDurb7laFTBsn4NDLqC43bTDct3uxn0eanyQx3NbXSts0UdkcfV+fYYfSuaYbZoqBaOBFPmsMj9bFwtXdtxlB5mqbs0xjb6rQ5onae31CIBWDEtA4LA6vBY4YZSPfR6n823nvbZTThN99hsg/RuewvtESXBtIYWSaK5084xcHSLyG4gS2CH57zcinorDTmug860Wit383aa7isQSEbDcuiK3YWjlp5CurIcDv0WgsdFT+VRF4zU5071wKi4o5nus9kK6UN2+qbPLXGsVsOQgl7srrxpC4yIO/3zFSxSAUyxxqEHi0SAKcP1qJ+eL6IbbLZyejehQYaj1ua2ddB5BZnobB9dOJ0UEQLL43Ra6ZQcG50n2t15tGgCJb9pteO2zkqxgc7r4zmTb1AwCitqeJKFI56TQutJEiZZ4PT3VhvyRzs9Vk9eJZ4DHCTLYGFhYSZcKZQcIBGeggIPnWdzW9HdWSRf4U26wI3a8WhUF+or2nz0W2Ie3Qa8fNB5kDM5b9KDbmigjljgHxFmZc8shAdWABaJA2uS40yBfpJUAw9a9NysNDEakNEeJ4/DqGItXWSzeei3ppOpRTYrRxBSASw3ElZ9Ho0bDTg8Co4DJGy2Gppu0XOCQBDddLnAknCU8CBYau88UUQNnfZKWrF6wKzAQqB/m8WaSnqFB5hByxw6z+mCwSRuUDaPbhHaEUYQUFxwqX1TxPI2xKsD/eAszXRNAR8cscA/X7sARghaF6SqYBXgPtTP2qawoXmWMaXQboG1wKge9WyKwEwPLLvBYAewctRBYVZ2wA3m1tmZg6eWg7JzBpK07E7REwwHR5Pe3wdNXf1PjxrWuMHWNTc3F4JkpUhGQkp1O/V2d3kNfRTERWkpCXBw9tH9g+XraVGEEyyahF6QUN+O5uY+Oy72jNA1uNOdKpmK6BS3g5MiRszL6ac7esrhLxoKNcAdLtSvNo+W2RxysCz/Fm0lMCM8ex86C9RnmSZYyjODQW/aLWhQArjT5RzXQ4/gDt7to1McOUVEKl3oIFkcjpxrhB4UBFABZ4G/N26SG1rzKns8NrfdhXSDZDkHb7IILjdM1eNXJJdFD4KYQlutw50yWC4ZLNwBQj3oEUW3JAiSCLzsPM873J2VPTChyBHzrKDA1kwAK7WzEndYK2ncgurv6PW1dIfPZnPjgi9T6qCbQbIKAawadFZ0u4yamQArteAmXXNIX0u/1WBx0nTnoebKykxppLajje7A19Ce5kqcY4zNa3COFProypr+ysq84NSdu+WG4iCIR0d/uYAjEOTsqHGkdnktuCFhYFlr6ZHmPFoFi+Fh1Br9YGVlbUdzP86t70C8eCNpFPrptublhYNS5Ig+mG7zTRgZb6BTajoraacg92vo6aRHOpb3F8GqNCzepPsLwQzkWcvR2eZ+/XSCD8UjVla27k56tx5uag1fQK+HmaVk4jy4KZ0Nbnz3Cs/RfpeB4eBIMgTe3EnfLMxL0dv9vQtqUMMcu3MwpXLN0R5O8jvNGhKxGCm3+VvyHBzE4RFApp/2pKJ2WgvI3RqpoKetkl7TYtfvXkPfbMDBFGs51f64UiNGtPbk0Z3NeUclPhV41dTQuBP1q7HaysE5gTnwu1eIuG+kkk6Bfk7nMDrb4pqOo8Vwdhx5MLjdBKu0W3JIbrcdgTVsE3MkgeMLRNHJ85LT059pYRgJHTFWcNpsoijiJofa26nXo4YmwqLPEXP0HAEMORUs3imKqXo9ro7DGQUcl/Spomiz5UgSaoeRvBvUxiTlQEuXxSKlivATZIDhUn1KgSYfMaIdd4vojEA4UoGVmMoLqJ+Td0gwlRy9ySHpHYILWLpRP4urADEnphP2YCzBId+YI0iWEEycAR1AifJsMJKRZQkT7+AMcJbnBVLDGAX01jGGjBFvEgiSVHsDoYbgSUNLB0eywJBVgx10xmI0cuo44HJznNFocfAOk4UgUDtKSwomC8kaYRQTR5LAyySXMGpIwSVJLgc6N3lEghAcDocAY5KIlwCzlPuRrAH95uQRlf9RPxa1Mk2vMFLJ9fjjKwie0IGAxQYn/GEWOonqUFkGfbuWlT+2qkWnGa0m0FsJ/DAlNlRjtWB0JvMNGUceRuagZdSPtCmdWOCvVa5hwT0hEvzTiBG1WoWFP85kNFp1DpjMBkWjMku5n//szOfuooTpFIbJLzxhWGLfb3ICyG9xEnQLqUW/xUnQf7jEfIISlKD/QPT/ADikJbq/EB6rAAAAAElFTkSuQmCC'/></div>\n";
//const char CUSTOMHTML_BODY_INNER[] PROGMEM = "<div><img src='data:image/png;base64, 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'/></div>\n";
const char CUSTOMHTML_BODY_INNER[] PROGMEM = "<div><img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAASwAAABFCAMAAADHLythAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAyZpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuNi1jMTM4IDc5LjE1OTgyNCwgMjAxNi8wOS8xNC0wMTowOTowMSAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvIiB4bWxuczp4bXBNTT0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wL21tLyIgeG1sbnM6c3RSZWY9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9zVHlwZS9SZXNvdXJjZVJlZiMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIENDIDIwMTcgKFdpbmRvd3MpIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOkU4QTJFMEY4RjhFNDExRTlCM0QzODcxMDgwMkNFQjM2IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOkU4QTJFMEY5RjhFNDExRTlCM0QzODcxMDgwMkNFQjM2Ij4gPHhtcE1NOkRlcml2ZWRGcm9tIHN0UmVmOmluc3RhbmNlSUQ9InhtcC5paWQ6RThBMkUwRjZGOEU0MTFFOUIzRDM4NzEwODAyQ0VCMzYiIHN0UmVmOmRvY3VtZW50SUQ9InhtcC5kaWQ6RThBMkUwRjdGOEU0MTFFOUIzRDM4NzEwODAyQ0VCMzYiLz4gPC9yZGY6RGVzY3JpcHRpb24+IDwvcmRmOlJERj4gPC94OnhtcG1ldGE+IDw/eHBhY2tldCBlbmQ9InIiPz47PiWoAAAAwFBMVEWgtpUXZqeKpSv6+/mtR438zRjpGyo6ZSFye1YhYnGepnwkldeO1fbO2skuWRH2hEpFcjQiq1ayxKmz00RnilWKS2vErRcnZht8mSZoo5bZ49ZLaxYlf5YuWyZzlWOkdjVwaiRUUjNDUxjn7OTD0b26LyTh592kxz9Xf0QaTw4lhjfDsJ+DpHvt8epjfRbx9PC6x8jdfkJPh2aZ4v/p28xtQBcbXQh0srGMNBu+fzvz6eGIzedqT0siYkJ9wM/////CrG7yAAAAQHRSTlP///////////////////////////////////////////////////////////////////////////////////8AwnuxRAAAFLRJREFUeNrsXIFbm8jTpkHdhoZ1S4ikUDABJJBCOLWinon5//+rb2Z3gYUQo73rd/d7nhuitUsk4c07787OzKrFdtCauX/LdEt9LpptKaejvD1tG7octdTnx/t/nfVuytbaU5ZtB/KwgwgGtM/+blwfVHvzun8usl3XfFs5/SNctsZKOWrTdpAG/z6wSuaslcNJ21M5uWuMmhys3ZfGdm+DVS7Gn7q264BFzxqbkUiOBst2NPxXgrX+qlrzKQNYd/e1TZhVg/UdbQz/2vpb100Wu0+fxo3hfz7vdYVZs7PZFm12dkZrB7WXcnB7dra1/4VgpQDWWpgDP9FYBev17u4bPF7umSHB+k43drqh37+MaXH8qnGQ0t2nHUltmz9SOv40JoHtqmCFDE45IYCVt8zaVjaMVtuzbar/K5m1DuF+Pqc2A7QUKQKwvlE4w+jrPUtrsBhi4X8H1NzjV83pbjdGLtUWArVAtqwOWOja1/7Z2TJvmbXF6+8BrLO3rv+PMcv5upbykANYnEItWOgLJnu9vyMNWOCnfwBYX/w3piuNokqNG08qQ65gvtYBCy6gfwaZ2gYKs/D6RQVOemIG+afccG23YK1bKUKwkFCm83r/yA7A2hnHJljTDSgKlt/MFqXvjwGuHTPN8hCsGYsTeE7spn4NFjAr/CBYrhHk0oI8/n1uqDBrLaem0k0CCVZMXievxEr0d4L1g2S+/2lM06B5hm4I2aK+FPMWLNCsGaXwWRg0pNszHkcU8EPrnO80I/QWC/FY0PT3xFkAFlOZRSPpSJQqYL0wyor3guWjz42zpDNqSU/sgXXNo4UlgPW8hIlRMKtE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'/></div>\n";

const char CUSTOMHTML_HEAD[] PROGMEM = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>LOXpixel! Config</title>";

const char CUSTOMHTML_STYLE_INNER[] PROGMEM = "button[type=submit] {border: 0;border-radius: 0.3rem;background-color: #54920a;color: #fff;line-height: 2.4rem;font-size: 1.2rem;width: 100%; margin-top: 4px;} fieldset{border-radius: 0.3rem;margin: 4px;background-color: #effbe6;}";

const char CUSTOMHTML_FORM_END[] PROGMEM = "</fieldset><button type='submit'>Save</button></form>";

// -- This is an OOP technique to override behaviour of the existing
// IotWebConfHtmlFormatProvider. Here two method are overriden from
// the original class. See IotWebConf.h for all potentially overridable
// methods of IotWebConfHtmlFormatProvider .
class CustomHtmlFormatProvider : public IotWebConfHtmlFormatProvider
{
  protected:
    String getStyleInner() override
    {
      return
        String(FPSTR(CUSTOMHTML_STYLE_INNER)) +
        IotWebConfHtmlFormatProvider::getStyleInner();
    }
    String getScriptInner() override
    {
      return
        IotWebConfHtmlFormatProvider::getScriptInner() +
        String(FPSTR(CUSTOMHTML_SCRIPT_INNER));
    }
    String getBodyInner() override
    {
      return
        String(FPSTR(CUSTOMHTML_BODY_INNER)) +
        IotWebConfHtmlFormatProvider::getBodyInner();
    }
    String getHead() override
    {
      return
        String(FPSTR(CUSTOMHTML_HEAD)) +
        IotWebConfHtmlFormatProvider::getHead();
    }

    String getFormEnd() override
    {
      return
        String(FPSTR(CUSTOMHTML_FORM_END));
    }


};
// -- An instance must be created from the class defined above.
CustomHtmlFormatProvider customHtmlFormatProvider;

void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";

  s += FPSTR(CUSTOMHTML_BODY_INNER);
  s += "<title>LOXpixel! Status</title> <style>";
  s += FPSTR(CUSTOMHTML_STYLE_INNER);
  s += "</style></head><body>";
  s += "<ul>";
  s += "<li>IP address: ";
  s += ipAddressValue;
  s += "<li>Subnet mask: ";
  s += netmaskValue;
  s += "<li>Gateway: ";
  s += gatewayValue;
  s += "</ul>";

  s += "<ul>";
  s += "<li>Stripe type (0=SK6812 / 1=WS2812 / 2=WS2801): ";
  s += atoi(stripeTypeParamValue);
  s += "<li>Number of LEDs: ";
  s += atoi(intParamValue);
  s += "<li>Brightness: ";
  s += atoi(brightnessParamValue);
  s += "<li>Data PIN: ";
  s += atoi(datenpinParamValue);
  s += "</ul>";

  s += "<table>";
  s += "<tr>";
  s += "<th>LED Part 1</th>";
  s += "<th>Value</th>";

  s += " </tr>";
  s += "<tr>";
  s += "<td>Start</td>";
  s += "<td>";
  s += atoi(start1Value);
  s += "</td>";

  s += "</tr>";
  s += "<tr>";
  s += "<td>Length</td>";
  s += "<td>";
  s += atoi(length1Value);
  s += "</td>";

  s += "</tr>";

  s += "</table>";

  s += "<table>";
  s += "<tr>";
  s += "<th>LED Part 2</th>";
  s += "<th>Value</th>";

  s += " </tr>";
  s += "<tr>";
  s += "<td>Start</td>";
  s += "<td>";
  s += atoi(start2Value);
  s += "</td>";

  s += "</tr>";
  s += "<tr>";
  s += "<td>Length</td>";
  s += "<td>";
  s += atoi(length2Value);
  s += "</td>";

  s += "</tr>";

  s += "</table>";

  s += "<table>";
  s += "<tr>";
  s += "<th>LED Part 3</th>";
  s += "<th>Value</th>";

  s += " </tr>";
  s += "<tr>";
  s += "<td>Start</td>";
  s += "<td>";
  s += atoi(start3Value);
  s += "</td>";

  s += "</tr>";
  s += "<tr>";
  s += "<td>Length</td>";
  s += "<td>";
  s += atoi(length3Value);
  s += "</td>";

  s += "</tr>";

  s += "</table>";

  s += "<table>";
  s += "<tr>";
  s += "<th>LED Part 4</th>";
  s += "<th>Value</th>";

  s += " </tr>";
  s += "<tr>";
  s += "<td>Start</td>";
  s += "<td>";
  s += atoi(start4Value);
  s += "</td>";

  s += "</tr>";
  s += "<tr>";
  s += "<td>Length</td>";
  s += "<td>";
  s += atoi(length4Value);
  s += "</td>";

  s += "</tr>";

  s += "</table>";

  s += "<table>";
  s += "<tr>";
  s += "<th>LED Part 5</th>";
  s += "<th>Value</th>";

  s += " </tr>";
  s += "<tr>";
  s += "<td>Start</td>";
  s += "<td>";
  s += atoi(start5Value);
  s += "</td>";

  s += "</tr>";
  s += "<tr>";
  s += "<td>Length</td>";
  s += "<td>";
  s += atoi(length5Value);
  s += "</td>";

  s += "</tr>";

  s += "</table>";

  s += "<table>";
  s += "<tr>";
  s += "<th>LED Part 6</th>";
  s += "<th>Value</th>";

  s += " </tr>";
  s += "<tr>";
  s += "<td>Start</td>";
  s += "<td>";
  s += atoi(start6Value);
  s += "</td>";

  s += "</tr>";
  s += "<tr>";
  s += "<td>Length</td>";
  s += "<td>";
  s += atoi(length6Value);
  s += "</td>";

  s += "</tr>";

  s += "</table>";

  s += "<table>";
  s += "<tr>";
  s += "<th>LED Part 7</th>";
  s += "<th>Value</th>";

  s += " </tr>";
  s += "<tr>";
  s += "<td>Start</td>";
  s += "<td>";
  s += atoi(start7Value);
  s += "</td>";

  s += "</tr>";
  s += "<tr>";
  s += "<td>Length</td>";
  s += "<td>";
  s += atoi(length7Value);
  s += "</td>";

  s += "</tr>";

  s += "</table>";

  s += "<table>";
  s += "<tr>";
  s += "<th>LED Part 8</th>";
  s += "<th>Value</th>";

  s += " </tr>";
  s += "<tr>";
  s += "<td>Start</td>";
  s += "<td>";
  s += atoi(start8Value);
  s += "</td>";

  s += "</tr>";
  s += "<tr>";
  s += "<td>Length</td>";
  s += "<td>";
  s += atoi(length8Value);
  s += "</td>";
  s += "</tr>";

  s += "</table>";

  s += "<form action='config'>";
  s += "  <input type='submit' value='go to configuration page' />";
  s += "</form>";

  s += "</body></html>\n";

  server.send(200, "text/html; charset=UTF-8", s);
}

void setup()
{
 
  ArduinoOTA.setPort(otaport);
  ArduinoOTA.setHostname(hostname);


  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      Serial.println("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      Serial.println("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      Serial.println("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      Serial.println("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  Udp.begin(localUdpPort);

  //Serial.begin(115200);
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY, 1);
  Serial.println();
  Serial.println("Starting up...");

  // -- Applying the new HTML format to IotWebConf.
  iotWebConf.setHtmlFormatProvider(&customHtmlFormatProvider);
  //iotWebConf.configSave();
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", [] { iotWebConf.handleConfig(); });
  server.onNotFound([]() {
    iotWebConf.handleNotFound();
  });

  IotWebConfParameter* thingNameParam = iotWebConf.getThingNameParameter();

  thingNameParam->label = "LOXpixel! name";

  IotWebConfParameter* apPasswordParam = iotWebConf.getApPasswordParameter();

  apPasswordParam->label = "LOXpixel! password";

  IotWebConfParameter* wifiPasswordParam = iotWebConf.getWifiPasswordParameter();

  wifiPasswordParam->label = "WiFi password";

  iotWebConf.addParameter(&separator1);

  iotWebConf.addParameter(&ipAddressParam);
  iotWebConf.addParameter(&gatewayParam);
  iotWebConf.addParameter(&netmaskParam);

  iotWebConf.addParameter(&separator0);
  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);

  iotWebConf.addParameter(&stripeTypeParam);
  iotWebConf.addParameter(&intParam);
  iotWebConf.addParameter(&datenpinParam);
  iotWebConf.addParameter(&brightnessParam);

  iotWebConf.addParameter(&separator2);
  iotWebConf.addParameter(&start1);
  iotWebConf.addParameter(&length1);

  iotWebConf.addParameter(&separator3);
  iotWebConf.addParameter(&start2);
  iotWebConf.addParameter(&length2);

  iotWebConf.addParameter(&separator4);
  iotWebConf.addParameter(&start3);
  iotWebConf.addParameter(&length3);

  iotWebConf.addParameter(&separator5);
  iotWebConf.addParameter(&start4);
  iotWebConf.addParameter(&length4);

  iotWebConf.addParameter(&separator6);
  iotWebConf.addParameter(&start5);
  iotWebConf.addParameter(&length5);

  iotWebConf.addParameter(&separator7);
  iotWebConf.addParameter(&start6);
  iotWebConf.addParameter(&length6);

  iotWebConf.addParameter(&separator8);
  iotWebConf.addParameter(&start7);
  iotWebConf.addParameter(&length7);

  iotWebConf.addParameter(&separator9);
  iotWebConf.addParameter(&start8);
  iotWebConf.addParameter(&length8);

  //iotWebConf.addParameter(&floatParam);


  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.setApConnectionHandler(&connectAp);
  iotWebConf.setWifiConnectionHandler(&connectWifi);



  // -- Initializing the configuration.

  boolean validConfig = iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", [] { iotWebConf.handleConfig(); });

  server.onNotFound([]() {
    iotWebConf.handleNotFound();
  });

  Serial.println("Ready.");

  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, atoi(intParamValue));
  //FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, 3);

  //Starthelligkeit auf 255
  // FastLED.setBrightness(255);
  FastLED.setBrightness(atoi(brightnessParamValue));
}

void effekte()
{

  switch (effektNR)
  {
  case 0:
    //Farben Setzen
    //Einteilungen pro Wand
    // An dieser Stelle kann die Anzahl der Einteilungen angepasst werden

    if (gradient_mode == 1)
    {
      // Maximal 4 Farben
      fill_gradient_RGB(leds, NUM_LEDS, gradient1, gradient2, gradient3, gradient4);
    }

    else
    {

      if (atoi(start1Value) >= 0)
      {
        if (atoi(start1Value) == 1 || atoi(start1Value) == 0)
        {
          fadeTowardColor(leds, atoi(length1Value), bgColor, 5);
        }
        else
        {
          fadeTowardColor(leds + atoi(start1Value) - 1, atoi(length1Value), bgColor, 5);
        }
      }

      if (atoi(start2Value) > 0)
      {
        fadeTowardColor(leds + atoi(start2Value) - 1, atoi(length2Value), bgColor2, 5);
      }

      if (atoi(start3Value) > 0)
      {
        fadeTowardColor(leds + atoi(start3Value) - 1, atoi(length3Value), bgColor3, 5);
      }

      if (atoi(start4Value) > 0)
      {
        fadeTowardColor(leds + atoi(start4Value) - 1, atoi(length4Value), bgColor4, 5);
      }

      if (atoi(start5Value) > 0)
      {
        fadeTowardColor(leds + atoi(start5Value) - 1, atoi(length5Value), bgColor5, 5);
      }
      if (atoi(start6Value) > 0)
      {
        fadeTowardColor(leds + atoi(start6Value) - 1, atoi(length6Value), bgColor6, 5);
      }

      if (atoi(start7Value) > 0)
      {
        fadeTowardColor(leds + atoi(start7Value) - 1, atoi(length7Value), bgColor7, 5);
      }

      if (atoi(start8Value) > 0)
      {
        fadeTowardColor(leds + atoi(start8Value) - 1, atoi(length8Value), bgColor8, 5);
      }

      // //Wand2 LED 50 -> LED 101 , Fadingzeit 5
      // fadeTowardColor( leds + 51, 50, bgColor2, 5);
      // //Wand3 LED 102  -> LED 202 , Fadingzeit 5
      // fadeTowardColor( leds + 101, 100, bgColor3, 5);
      // //Wand4 LED 202 -> LED 300 , Fadingzeit 5
      // fadeTowardColor( leds + 201, 99, bgColor4, 5);
    }

    break;
  case 1: //Rainbow
          //Serial.println("Rainbow!");
    rainbowCycle(20);
    break;
  case 2: //Rainbow with Glitter
    fill_rainbow(leds, NUM_LEDS, gHue, 7);
    if (random8() < 80)
    {
      leds[random16(NUM_LEDS)] += CRGB::White;
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

    for (int i = 0; i < 8; i++)
    {
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
  case 9: //Lightning
    lightning();
    break;
  case 10: //Meteor
    // meteorRain - Color (red, green, blue), meteor size, trail decay, random trail decay (true/false)
    meteorRain(0xff, 0xff, 0xff, 10, 64, true);
    break;
  case 11:
    // theatherChase - Color (red, green, blue), speed delay
    theaterChase(0xff, 0, 0, 50);
    break;

  case 12:
  {
    // theaterChaseRainbow - Speed delay
    theaterChaseRainbow(50);
    break;
  }

  case 13:
  {
    CylonBounce(0xff, 0, 0, 4, 10, 50);
    break;
  }
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

  if (value < 200000000)
  { // RGB

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
  else
  { // LUMITECH

    float bri, ct;
    int briNorm, miredNorm;

    bri = floor((value - 200000000) / 10000);        // 0-100
    ct = floor((value - 200000000) - (bri * 10000)); // Wert in Kelvin, von 2700 - 6500

    briNorm = (int)round(bri * 2.55);     // 0-255
    miredNorm = (int)round(1000000 / ct); // Wert von 154 - 370

    ctrgbvalue = colorTemperatureToRGB(ct);

    rgbvalue.red = ctrgbvalue.red;
    rgbvalue.green = ctrgbvalue.green;
    rgbvalue.blue = ctrgbvalue.blue;
  }
  return rgbvalue;
}

CRGB colorTemperatureToRGB(int kelvin)
{

  CRGB rgbvalue(0, 0, 0);
  float temp = kelvin / 100;

  float red, green, blue;

  if (temp <= 66)
  {

    red = 255;

    green = temp;
    green = 99.4708025861 * log(green) - 161.1195681661;

    if (temp <= 19)
    {

      blue = 0;
    }
    else
    {

      blue = temp - 10;
      blue = 138.5177312231 * log(blue) - 305.0447927307;
    }
  }
  else
  {
    red = temp - 60;
    red = 329.698727446 * pow(red, -0.1332047592);
    green = temp - 60;
    green = 288.1221695283 * pow(green, -0.0755148492);
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
  rgbvalue.blue = round(blue);

  return rgbvalue;
}

void loop()
{

  // -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();

  ArduinoOTA.handle();

  effekte();

  //Serial.println("Loop..");
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {

    //UDP Paket lesen
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");
    Serial.println(packetBuffer);

    String hexstring = packetBuffer;

    //Unterscheidung der zu steuernden Wand anhand des vorangestellten Sonderzeichen. Dies ist beliebig anpassbar

    if (hexstring.charAt(0) == '#')
    {
      bgColor = parseLoxone(hexstring);
      gradient_mode = 0;
    }

    if (hexstring.charAt(0) == '!')
    {
      bgColor2 = parseLoxone(hexstring);
      gradient_mode = 0;
    }

    if (hexstring.charAt(0) == '?')
    {
      bgColor3 = parseLoxone(hexstring);
      gradient_mode = 0;
    }

    if (hexstring.charAt(0) == '*')
    {
      bgColor4 = parseLoxone(hexstring);
      gradient_mode = 0;
    }

    if (hexstring.charAt(0) == '=')
    {
      bgColor5 = parseLoxone(hexstring);
      gradient_mode = 0;
    }

    if (hexstring.charAt(0) == '&')
    {
      bgColor6 = parseLoxone(hexstring);
      gradient_mode = 0;
    }

    if (hexstring.charAt(0) == '%')
    {
      bgColor7 = parseLoxone(hexstring);
      gradient_mode = 0;
    }

    if (hexstring.charAt(0) == '$')
    {
      bgColor8 = parseLoxone(hexstring);
      gradient_mode = 0;
    }

    if (hexstring.charAt(0) == 'a')
    {
      gradient1 = parseLoxone(hexstring);
      gradient_mode = 1;
    }
    if (hexstring.charAt(0) == 'b')
    {
      gradient2 = parseLoxone(hexstring);
      gradient_mode = 1;
    }

    if (hexstring.charAt(0) == 'c')
    {
      gradient3 = parseLoxone(hexstring);
      gradient_mode = 1;
    }

    if (hexstring.charAt(0) == 'd')
    {
      gradient4 = parseLoxone(hexstring);
      gradient_mode = 1;
    }

    for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++)
      packetBuffer[i] = 0; // Buffer leeren

    //Sonderzeichen fuer Effektsteuerung
    if (hexstring.charAt(0) == '@')
    {
      //effektNR = hexstring.charAt(1) - '0';
      hexstring.remove(0, 1);
      effektNR = hexstring.toInt();
      Serial.println(" Effekt@!");
      Serial.println(effektNR);
    }
  }

  FastLED.show();

  EVERY_N_MILLISECONDS(20)
  {
    gHue++;
  }

  EVERY_N_SECONDS(5)
  { // Change the target palette to a random one every 5 seconds.
    targetPalette = CRGBPalette16(CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 192, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)));
  }

  EVERY_N_MILLIS(10)
  {
    nblendPaletteTowardPalette(currentPalette, targetPalette, 48);
  }

  

  if (needreset)
  {

    iotWebConf.delay(1000);
    ESP.restart();
  }
}

void nblendU8TowardU8(uint8_t &cur, const uint8_t target, uint8_t amount)
{
  if (cur == target)
    return;

  if (cur < target)
  {
    uint8_t delta = target - cur;
    delta = scale8_video(delta, amount);
    cur += delta;
  }
  else
  {
    uint8_t delta = cur - target;
    delta = scale8_video(delta, amount);
    cur -= delta;
  }
}

CRGB fadeTowardColor(CRGB &cur, const CRGB &target, uint8_t amount)
{
  nblendU8TowardU8(cur.red, target.red, amount);
  nblendU8TowardU8(cur.green, target.green, amount);
  nblendU8TowardU8(cur.blue, target.blue, amount);
  return cur;
}

// Fade an entire array of CRGBs toward a given background color by a given amount
// This function modifies the pixel array in place.
void fadeTowardColor(CRGB *L, uint16_t N, const CRGB &bgColor, uint8_t fadeAmount)
{
  for (uint16_t i = 0; i < N; i++)
  {
    fadeTowardColor(L[i], bgColor, fadeAmount);
  }
}

void lightning()
{
  for (int flashCounter = 0; flashCounter < random8(3, FLASHES); flashCounter++)
  {
    if (flashCounter == 0)
      dimmer = 5; // the brightness of the leader is scaled down by a factor of 5
    else
      dimmer = random8(1, 3); // return strokes are brighter than the leader

    FastLED.showColor(CHSV(255, 0, 255 / dimmer));
    delay(random8(4, 10)); // each flash only lasts 4-10 milliseconds
    FastLED.showColor(CHSV(255, 0, 0));

    if (flashCounter == 0)
      delay(150);             // longer delay until next flash after the leader
    delay(50 + random8(100)); // shorter delay between strokes
  }
  delay(random8(FREQUENCY) * 100); // delay between strikes
}

void Fire2012()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < NUM_LEDS; i++)
  {
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = NUM_LEDS - 1; k >= 2; k--)
  {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < SPARKING)
  {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (int j = 0; j < NUM_LEDS; j++)
  {
    CRGB color = HeatColor(heat[j]);
    int pixelnumber;
    if (gReverseDirection)
    {
      pixelnumber = (NUM_LEDS - 1) - j;
    }
    else
    {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}

// Set all LEDs to a given color and apply it (visible)
void setAll(byte red, byte green, byte blue)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    setPixel(i, red, green, blue);
  }
  showStrip();
}

// Set a LED color (not yet visible)
void setPixel(int Pixel, byte red, byte green, byte blue)
{
#ifdef ADAFRUIT_NEOPIXEL_H
  // NeoPixel
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));
#endif
#ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
#endif
}

// Apply LED color changes
void showStrip()
{
  FastLED.show();
}

int meteor_i = 0;

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay)
{

  // for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {

  // fade brightness all LEDs one step
  for (int j = 0; j < NUM_LEDS; j++)
  {
    if ((!meteorRandomDecay) || (random(10) > 5))
    {
      fadeToBlack(j, meteorTrailDecay);
    }
  }

  // draw meteor
  for (int j = 0; j < meteorSize; j++)
  {
    if ((-j < NUM_LEDS) && (meteor_i - j >= 0))
    {
      setPixel(meteor_i - j, red, green, blue);
    }
  }

  showStrip();

  meteor_i++;
  if (meteor_i >= NUM_LEDS + NUM_LEDS)
  {
    meteor_i = 0;
  }
  // }
}

// used by meteorrain
void fadeToBlack(int ledNo, byte fadeValue)
{
#ifdef ADAFRUIT_NEOPIXEL_H
  // NeoPixel
  uint32_t oldColor;
  uint8_t r, g, b;
  int value;

  oldColor = strip.getPixelColor(ledNo);
  r = (oldColor & 0x00ff0000UL) >> 16;
  g = (oldColor & 0x0000ff00UL) >> 8;
  b = (oldColor & 0x000000ffUL);

  r = (r <= 10) ? 0 : (int)r - (r * fadeValue / 256);
  g = (g <= 10) ? 0 : (int)g - (g * fadeValue / 256);
  b = (b <= 10) ? 0 : (int)b - (b * fadeValue / 256);

  strip.setPixelColor(ledNo, r, g, b);
#endif
#ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  leds[ledNo].fadeToBlackBy(fadeValue);
#endif
}

void ripple()
{

  if (currentBg == nextBg)
  {
    nextBg = random(256);
  }
  else if (nextBg > currentBg)
  {
    currentBg++;
  }
  else
  {
    currentBg--;
  }
  for (uint16_t l = 0; l < NUM_LEDS; l++)
  {
    leds[l] = CHSV(currentBg, 255, 50); // strip.setPixelColor(l, Wheel(currentBg, 0.1));
  }

  if (step == -1)
  {
    center = random(NUM_LEDS);
    color = random(256);
    step = 0;
  }

  if (step == 0)
  {
    leds[center] = CHSV(color, 255, 255); // strip.setPixelColor(center, Wheel(color, 1));
    step++;
  }
  else
  {
    if (step < maxSteps)
    {
      //Serial.println(pow(fadeRate,step));

      leds[wrap(center + step)] = CHSV(color, 255, pow(fadeRate, step) * 255); //   strip.setPixelColor(wrap(center + step), Wheel(color, pow(fadeRate, step)));
      leds[wrap(center - step)] = CHSV(color, 255, pow(fadeRate, step) * 255); //   strip.setPixelColor(wrap(center - step), Wheel(color, pow(fadeRate, step)));
      if (step > 3)
      {
        leds[wrap(center + step - 3)] = CHSV(color, 255, pow(fadeRate, step - 2) * 255); //   strip.setPixelColor(wrap(center + step - 3), Wheel(color, pow(fadeRate, step - 2)));
        leds[wrap(center - step + 3)] = CHSV(color, 255, pow(fadeRate, step - 2) * 255); //   strip.setPixelColor(wrap(center - step + 3), Wheel(color, pow(fadeRate, step - 2)));
      }
      step++;
    }
    else
    {
      step = -1;
    }
  }

  LEDS.show();
  //delay(50);
}

int wrap(int step)
{
  if (step < 0)
    return NUM_LEDS + step;
  if (step > NUM_LEDS - 1)
    return step - NUM_LEDS;
  return step;
}

void one_color_allHSV(int ahue, int abright)
{ // SET ALL LEDS TO ONE COLOR (HSV)
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CHSV(ahue, 255, abright);
  }
}

// used by rainbowCycle and theaterChaseRainbow
byte *Wheel(byte WheelPos)
{
  static byte c[3];

  if (WheelPos < 85)
  {
    c[0] = WheelPos * 3;
    c[1] = 255 - WheelPos * 3;
    c[2] = 0;
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    c[0] = 255 - WheelPos * 3;
    c[1] = 0;
    c[2] = WheelPos * 3;
  }
  else
  {
    WheelPos -= 170;
    c[0] = 0;
    c[1] = WheelPos * 3;
    c[2] = 255 - WheelPos * 3;
  }

  return c;
}

int rainbow_j = 0;
void rainbowCycle(int SpeedDelay)
{
  byte *c;
  uint16_t i, j;

  // for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
  for (i = 0; i < NUM_LEDS; i++)
  {
    c = Wheel(((i * 256 / NUM_LEDS) + rainbow_j) & 255);
    setPixel(i, *c, *(c + 1), *(c + 2));
  }
  showStrip();
  delay(SpeedDelay);
  rainbow_j++;
  if (rainbow_j >= 256 * 5)
  {
    rainbow_j = 0;
  }
  //  }
}

void theaterChase(byte red, byte green, byte blue, int SpeedDelay)
{
  // for (int j=0; j<10; j++) {  //do 10 cycles of chasing
  for (int q = 0; q < 3; q++)
  {
    for (int i = 0; i < NUM_LEDS; i = i + 3)
    {
      setPixel(i + q, red, green, blue); //turn every third pixel on
    }
    showStrip();

    delay(SpeedDelay);

    for (int i = 0; i < NUM_LEDS; i = i + 3)
    {
      setPixel(i + q, 0, 0, 0); //turn every third pixel off
    }
  }
  // }
}

int chaserainbow_j = 0;

void theaterChaseRainbow(int SpeedDelay)
{
  byte *c;

  //for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
  for (int q = 0; q < 3; q++)
  {
    for (int i = 0; i < NUM_LEDS; i = i + 3)
    {
      c = Wheel((i + chaserainbow_j) % 255);
      setPixel(i + q, *c, *(c + 1), *(c + 2)); //turn every third pixel on
    }
    showStrip();

    delay(SpeedDelay);

    for (int i = 0; i < NUM_LEDS; i = i + 3)
    {
      setPixel(i + q, 0, 0, 0); //turn every third pixel off
    }
  }
  chaserainbow_j++;
  if (chaserainbow_j >= 256)
  {
    chaserainbow_j = 0;
  }

  //  }
}

void fillnoise8()
{

#define scale 30 // Don't change this programmatically or everything shakes.

  static uint16_t dist; // A random number for our noise generator.

  for (int i = 0; i < NUM_LEDS; i++)
  {                                                                      // Just ONE loop to fill up the LED array as all of the pixels change.
    uint8_t index = inoise8(i * scale, dist + i * scale);                // Get a value from the noise function. I'm using both x and y axis.
    leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND); // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }
  dist += beatsin8(10, 1, 4); // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave.
  // In some sketches, I've used millis() instead of an incremented counter. Works a treat.
} // fillnoise8()

int cylon_i = 0;
int cylon_dir = 0;

void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay)
{

  if (cylon_dir == 0 && (cylon_i < NUM_LEDS - EyeSize - 2))
  {
    setAll(0, 0, 0);
    setPixel(cylon_i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++)
    {
      setPixel(cylon_i + j, red, green, blue);
    }
    setPixel(cylon_i + EyeSize + 1, red / 10, green / 10, blue / 10);
    showStrip();
    delay(SpeedDelay);
    cylon_i++;
  }

  if (cylon_dir == 0 && (cylon_i >= (NUM_LEDS - EyeSize - 2)))
  {
    cylon_dir = 1;

    delay(ReturnDelay);
  }

  if (cylon_dir == 1 && cylon_i > 0)
  {
    setAll(0, 0, 0);
    setPixel(cylon_i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++)
    {
      setPixel(cylon_i + j, red, green, blue);
    }
    setPixel(cylon_i + EyeSize + 1, red / 10, green / 10, blue / 10);
    showStrip();
    delay(SpeedDelay);
    cylon_i--;
  }

  if (cylon_dir == 1 && cylon_i == 0)
  {
    cylon_dir = 0;
    cylon_i = 0;
    delay(ReturnDelay);
  }
}

void configSaved()
{
  Serial.println("Configuration was updated.");
  needreset = true;
}

boolean formValidator()
{

  boolean valid = true;

  if (!ipAddress.fromString(server.arg(ipAddressParam.getId())))
  {
    ipAddressParam.errorMessage = "Please provide a correct ip address!";
    valid = false;
  }
  if (!netmask.fromString(server.arg(netmaskParam.getId())))
  {
    netmaskParam.errorMessage = "Please provide a correct subnet mask!";
    valid = false;
  }
  if (!gateway.fromString(server.arg(gatewayParam.getId())))
  {
    gatewayParam.errorMessage = "Please provide a correct gateway ip address!";
    valid = false;
  }
  return valid;
}

boolean connectAp(const char* apName, const char* password)
{
  // -- Custom AP settings
  return WiFi.softAP(apName, password, 4);
}

void connectWifi(const char* ssid, const char* password)
{
  ipAddress.fromString(String(ipAddressValue));
  netmask.fromString(String(netmaskValue));
  gateway.fromString(String(gatewayValue));

  if (!WiFi.config(ipAddress, gateway, netmask)) {
    Serial.println("STA Failed to configure");
  }
  Serial.print("ip: ");
  Serial.println(ipAddress);
  Serial.print("gw: ");
  Serial.println(gateway);
  Serial.print("net: ");
  Serial.println(netmask);
  WiFi.begin(ssid, password);
}
