#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include "pgmspace.h"

#define BUTTON_RESET_PIN 0

ESP8266WebServer server(80);

// glowna funkcja serwera
void handleRoot()
{
	server.send(200, F("text/html"), F("<h1>You are connected with ESP</h1>") );
}

// reset ESPka
void reset_module()
{
	Serial.println(F("Reset!"));
	delay(100);
	ESP.reset();
	delay(100);
	wdt_reset();
	while(1)
	{
		wdt_reset();
	}
}

// Przesyla strone z formatka do wpisania danych sieci
void dane_sieci_strona()
{
  server.send(200, "text/html",
  F("<h1>Wprowadz ustawienia sieci WiFi</h1>\
  <form action=\"/danesieci\">\
  SSID:<br>\
  <input type=\"text\" name=\"ssid\"><br>\
  Haslo:<br>\
  <input type=\"text\" name=\"pass\"><br><br>\
  <input type=\"submit\" value=\"Submit\">\
  </form>"));
}

// Odbiera SSID i haslo do nowej sieci
void odbior_danych_sieci()
{
  String message = "Otrzymalem:\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  // debug na konsoli
  for( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  Serial.println ( message );

  // rozkodowanie komendy
  if( server.argName(0) == "ssid" && server.argName(1) == "pass")
  {
    server.send(200, "text/html", F("<h1>Odebralem dane sieci. Laczenie...</h1>"));
    delay(100);

    EEPROM.begin(512); //Max bytes of eeprom to use
    yield();
    Serial.println();
    Serial.print(F("Writing to EEPROM..."));

    // zapis flagi poprawnosci
    EEPROM.write(0, '0');
    // zapis SSID
    int i = 1;
    for (i = 1; i < 51 || i < server.arg( 0 ).length(); i++)
    {
      EEPROM.write(i, server.arg( 0 )[i-1]);
    }
    EEPROM.write(i, 0);

    // zapis hasla:
    for (i = 51; i < 51 || i < (server.arg( 1 ).length() + 51); i++)
    {
      EEPROM.write(i, server.arg( 1 )[i-51]);
    }
    EEPROM.write(i, 0);

    EEPROM.commit();
    EEPROM.end();

		reset_module();
  }
  else
  {
      server.send(200, "text/html", F("<h1>Podano niepoprawne dane!</h1>"));
  }
}

// funkcja tworzy Acces Point i uruchamia serwer do zmiany danych sieci
void utworz_AP()
{
  bool result = WiFi.softAP("ESP_server", "dziendoberek",10);
  if(result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", dane_sieci_strona);
  server.on("/danesieci", odbior_danych_sieci);
  server.begin();
  Serial.println("HTTP server started");
}

// Funkcja uruchamiajaca proces zmianyt parametrw sieci
// Zapisuje odpowiednia flage do EEPROMU oraz
void zresetuj_ustawienia_sieci()
{
  EEPROM.begin(512); //Max bytes of eeprom to use
  yield();

  // zapis flagi poprawnosci
  EEPROM.write(0, 'k');

  EEPROM.commit();
  EEPROM.end();

	reset_module();
}

// funkcja inicjalizacyjna
// sprawdza zawartosc EEPROMU i w zaleznosci od niej ustawia AP do pobrania nowych danych
// lub uruchamia domyslna aplikacje
void sprawdz_dane_z_pamieci_i_polacz()
{
  Serial.println("Sprawdzanie zawartosci EEPROMU...");

  char dest[100];
  char new_data = '0';

  EEPROM.begin(512);
  delay(10);

  // jesli w EEPROMIE w pierwszej komorce jest wartosc '0' - wtedy oznacza to, ze mamy skonfigurowana siec
  // pobieramy wiec dane, jesli nie - uruchamiamy AP z serwerem do podania strony
  if( char(EEPROM.read(0)) != '0' )
  {
    EEPROM.end();
    delay(500);
    // ustawienie AP.
    utworz_AP();
  }
  else
  {
    // pobieramy z EEPROMU dane sieci i laczymy sie z nia:
    char ssid[50];
    char pass[49];

    // pobranie ssid
    for (int i = 1; i < 51; i++)
    {
      ssid[i-1] = char(EEPROM.read(i));
    }

    // pobranie hasla
    for (int i = 51; i < 100; i++)
    {
      pass[i-51] = char(EEPROM.read(i));
    }

    EEPROM.end();
    Serial.println("Dane z EEPROMU:");
    Serial.println(ssid);
    Serial.println(pass);
    Serial.println("Nawiazuje polaczenie:");

		// https://stackoverflow.com/questions/39688410/how-to-switch-to-normal-wifi-mode-to-access-point-mode-esp8266
		WiFi.softAPdisconnect();
		WiFi.disconnect();
		WiFi.mode(WIFI_STA);
		delay(100);

    WiFi.begin ( ssid, pass );

    // oczekiwanie na polaczenie + testowanie przycisku RESETU
		// w przypadku, gdyby ktos podal bledna siec

		int reset_counter = 0;
    while ( WiFi.status() != WL_CONNECTED )
    {
      delay ( 200 );
      Serial.print ( "." );
			if( digitalRead(BUTTON_RESET_PIN) == LOW )
			{
				reset_counter++;
				if(reset_counter == 15)
				{
					zresetuj_ustawienia_sieci();
				}
			}
			else
			{
				reset_counter = 0;
			}
    }

    Serial.println ( "" );
    Serial.print ( "Connected to " );
    Serial.println ( ssid );
    Serial.print ( "IP address: " );
    Serial.println ( WiFi.localIP() );

    server.on("/", zresetuj_ustawienia_sieci);
    server.on("/reset", zresetuj_ustawienia_sieci);
    server.begin();
  }
}



void setup()
{
	delay(1000);
  //zresetuj_ustawienia_sieci();
	Serial.begin(115200);
	// ustawienia pinu resetu
	pinMode(BUTTON_RESET_PIN, INPUT_PULLUP);
  sprawdz_dane_z_pamieci_i_polacz();
}

unsigned long t1_previous = 0;
unsigned long t1_current = 0;

unsigned int button_reset_counter = 0;

const unsigned long t1_interval = 50;
const unsigned long t2_interval = 10000;

void loop()
{
	// obsluga klientow serwera
	server.handleClient();

	// obsluga przycisku resetu ustawien sieci
	t1_current = millis();
	if(t1_current - t1_previous >= t1_interval)
	{
		t1_previous = t1_current;

		if( digitalRead(BUTTON_RESET_PIN) == LOW )
		{
			button_reset_counter++;
			if(button_reset_counter == 15)
			{
				zresetuj_ustawienia_sieci();
			}
		}
		else
		{
			button_reset_counter = 0;
		}
	}

}
