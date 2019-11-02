
![](https://unser-smartes-zuhause.de/wp-content/uploads/2019/11/logo.png)
# LOXpixel!  FASTLED Implementierung für Loxone

eine FASTLED Implementierung in Verbindung mit einem ESP8266 Board (NodeMCU)

Einige Zusatzinformationen können in unserem Bau und Smart-Home Blog nachgelesen werden:

https://unser-smartes-zuhause.de/2019/10/20/loxpixel-rgbw-neopixel-integration-in-loxone/

Ich habe mit der Entwicklung im Rahmen meiner Smart-Home Planung für unseren Neubau begonnen. Die Idee war Neopixel ähnliche LED Streifen mit meinem Loxone Miniserver zu verbinden. Und das alles möglichst einfach bedienbar.
## Erster Start
LOXpixel! erstellt bei dem ersten Start einen WLAN Accesspoint mit folgenden Daten:

**SSID: loxpixel!
Passwort: loxpixel**

Nach der Verbindung mit dem Accesspoint, wird man auf die Statusseite verwiesen. Dort müssen zunächst die Grundeinstellungen angepasst werden.

 - Namen (Hostnamen) für die LOXpixel! Platine
 - Feste IP 
 - Subnetzmaske
 - Standardgateway
 - Anzahl der LEDS
 ~~- Datenpin~~
 ~~- LED Controller Typ~~

Der LED Stripe kann in bis zu 8 Abschnitte aufgeteilt werden. Hierbei muss jeweils die erste LED sowie die Länge des Abschnitts angegeben werden.
z.Bsp.:
**Part 1 Anfang :  LED 1
Part 1 Länge:  10
Part 2 Anfang: LED 11**
usw.
## Features
 - Einen neopixel ähnlichen LED Stripe mit bis zu 1200pixel in bis zu 8 Abschnitte aufteilen und einzeln ansteuern (Die Funktion mit mehreren Pixeln und Abschnitten wurde bisher nicht geprüft). 
 
![Beispielbild](https://i2.wp.com/unser-smartes-zuhause.de/wp-content/uploads/2019/10/2019-10-19-13_19_33-Window.png?w=827&ssl=1)
 
 - Die Abschnitte des LED Streifen sind aus der Loxone App als einzelne Leuchtmittel ansteuerbar.
 
![Ansteuerung der Abschnitte aus der Loxone App](https://unser-smartes-zuhause.de/wp-content/uploads/2019/11/Screenshot_2019-11-02-22-30-11-027_com.loxone.kerberos_Easy-Resize.com_.jpg)

![Beispiel der Abschnitte im Test Setup](https://unser-smartes-zuhause.de/wp-content/uploads/2019/11/IMG_20191102_223022_1_Easy-Resize.com_.jpg)

- In der Loxone App kann ein Farbverlauf unter Verwendung des kompletten LED Streifen erstellt werden

![Loxone App Steuerung Farbverlauf](https://unser-smartes-zuhause.de/wp-content/uploads/2019/11/Screenshot_2019-11-02-22-31-48-058_com.loxone.kerberos_Easy-Resize.com_.jpg)
![Beispiel Farbverlauf im Testsetup](https://unser-smartes-zuhause.de/wp-content/uploads/2019/11/IMG_20191102_223200_Easy-Resize.com_.jpg)

- Effekte unter Verwendung des kompletten LED Streifen aus der Loxone App starten

Beispielvideo (Testsetup) :
[![Beispielvideo](http://img.youtube.com/vi/hsXI4zp1-I4/0.jpg)](http://www.youtube.com/watch?v=hsXI4zp1-I4)

- Einfaches Webinterface zur Einstellung der wichtigsten Parameter
![Statusseite](https://unser-smartes-zuhause.de/wp-content/uploads/2019/11/Screenshot_2019-11-02-23-11-58-708_com.android.chrome_Easy-Resize.com_.jpg)
![Grundeinstellungen](https://unser-smartes-zuhause.de/wp-content/uploads/2019/11/Screenshot_2019-11-02-23-12-25-713_com.android.chrome_Easy-Resize.com_.jpg)
# Software
## Vorraussetzungen
TBD
## VSCode / PlatformIO
TBD
# Hardware
## Verwendete Komponenten
LED Streifen:  [BTF-LIGHTING RGBW RGBNW Natürliches Weiß SK6812 (ähnlich WS2812B) 5m 60leds/pixels/m](https://www.amazon.de/BTF-LIGHTING-Nat%C3%BCrliches-Individuell-adressierbar-nicht-wasserdicht/dp/B01N2PC9KK/ref=as_li_ss_tl?th=1&linkCode=ll1&tag=unsersmarte01-21&linkId=7286a35d1bcb23a684513b37aa95e789&language=de_DE)  *

Netzteil:  [Mean Well LPV-100-5 LED-Trafo Konstantspannung 60W 0-12A 5 V/DC](https://www.amazon.de/gp/product/B00MWQF6M2/ref=as_li_ss_tl?ie=UTF8&psc=1&linkCode=ll1&tag=unsersmarte01-21&linkId=ce2ce103875b0810fd30c44153c2f10d&language=de_DE)  *

## LOXpixel! Prototype Board
TBD
## LOXpixel! V2 Board (Ausblick)
TBD

# Integration in Loxone
TBD

![LOXPixel! Funktionen](https://unser-smartes-zuhause.de/wp-content/uploads/2019/11/Screenshot_2019-11-02-22-30-55-465_com.loxone.kerberos_Easy-Resize.com_.jpg)
## Vorlage importieren
TBD

Die mit Sternchen (*) gekennzeichneten Links sind sogenannte Affiliate-Links. Wenn du auf so einen Affiliate-Link klickst und über diesen Link einkaufst, bekomme ich von dem betreffenden Online-Shop oder Anbieter eine Provision. Für dich verändert sich der Preis nicht.
