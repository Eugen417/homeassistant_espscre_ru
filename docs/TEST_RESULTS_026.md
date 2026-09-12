# Wifi-secrets hergebruiken — 0.2.6

De wizard controleert de ESPHome-map van de add-on. Met geldige wifi_ssid en
wifi_password in secrets.yaml worden wifi-velden verborgen en uitgeschakeld.
Alleen als dat bestand nog niet bestaat vraagt de wizard om wifi voor de eerste
installatie. Een bestaand onvolledig/ongeldig bestand wordt niet overschreven.
De browser ontvangt uitsluitend beschikbaarheid, geen secretwaarden.

API-encryptie- en OTA-sleutels blijven uniek gegenereerd door ESP Screens en staan
in de eigen apparaat-YAML. Updates behouden die YAML. Geen firmwarewijziging.

Controles: 45 Python-tests geslaagd, inclusief hergebruik zonder invoer, ongewijzigd
behoud van bestaande secrets, geen secretwaarden in status/profiel, eerste
installatie en ongeldige YAML. JavaScript-syntax en Git-diff gecontroleerd.
Browserpreview: bij bestaande wifi alleen bevestiging zichtbaar, geen wifi-invoer;
profiel aanmaken beschikbaar. Test gebruikt geen echte wifiwaarden.

Alle acht C++-regressies geslaagd; firmware is niet gewijzigd. De HA-add-on is
met back-up bijgewerkt van 0.2.4 naar 0.2.6. De echte firmware-status-API meldt
wifi state=ready en geen ontbrekende sleutels. Indelingen en instellingen voor/na
exact gelijk. Geen nieuw fysiek apparaat aangemaakt of geflasht tijdens deze test.
