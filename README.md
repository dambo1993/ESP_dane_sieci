# ESP_dane_sieci
Przykład funkcjonalności zmiany ustawień sieci dla projektów opartych na ESP8266 i arduino IDE.

W wielu projektach dane sieci wpisane są w kod programu przez co po zmianie nazwy/hasła sieci, trzeba kod kompilować od nowa. Tak samo gdy komuś podarujemy/sprzedamy nasz układ - musimy skompilować dane jego sieci co jest problematyczne. 
Kod zawiera przykład implementacji możliwości zmiany danych sieci z którą będzie się łączył nasz układ. 
Domyślnie uruchomiony jest Access Point do którego możemy się podłączyć. Po przejściu na adres"192.168.4.1" zobaczymy formularz do wpisania SSID i hasła docelowej sieci. Gdy je prześlemy moduł będzie próbował się z nimi połączyć. Gdy będziemy musieli zmienic ustawienia - przytrzymujemy przycisk RESET (ale nie jest to reset główny ESP) przez 3 sekundy - wtedy układ uruchomi się ponownie w trybie AP bez danych sieci do połaczenia, które możemy ponownie wprowadzić.
