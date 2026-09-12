# Tegels instellen voor jouw Home Assistant

Bewerk `device.yaml`, binnen de bestaande `substitutions:`. De basiscode hoef
je voor de onderstaande voorbeelden niet aan te passen. Elke wijziging vraagt
opnieuw bouwen en flashen. Doorloop eerst de [installatie](../README.md).

## Entity-ID's verzamelen

Open in HA **Ontwikkelaarstools → Statussen**. Zoek de gewenste entiteit en
kopieer de volledige ID, bijvoorbeeld `light.keuken`. Controleer status en
attributen. De schermtitel is vrij te kiezen; de entity-ID moet exact kloppen.
Test onder **Ontwikkelaarstools → Acties** de gewenste actie met die entiteit.
Een actie kan het echte apparaat bedienen: kies een geschikte test.
Zie [HA-acties](https://www.home-assistant.io/docs/scripts/perform-actions/).

Maak eerst een lijst met positie, titel, entity-ID, tikactie en lange-drukactie.
Gebruik bestaande HA-scripts/scènes voor meerdere apparaten of complexe logica.

## Posities en grenzen

| Posities | Bedoeld gebruik in deze firmware |
|---|---|
| 1–6 | Hoofdtegels; light, switch, scene, script, fan, cover en climate |
| 6 | Ook de vacuumkaart; de kaart is specifiek met tegel 6 verbonden |
| 7 en 9 | Extra lichttegels met helderheid/kleurbediening |
| 8 en 10 | Extra status-/actietegels; geen volledige numerieke/climatekaart |

`TILE_COUNT: "1"` tot `"10"` bepaalt hoeveel **opeenvolgende** posities zichtbaar
zijn. Tot en met zes zijn Vorige/Volgende verborgen; vanaf zeven staat 1–6 op
pagina één en de rest op pagina twee. Vrij scrollen is vervangen door vaste
pagina's. Laat ongebruikte posities op de neutrale voorbeeldentiteiten staan.
Verander geen rastermaten om tegels toe te voegen.

## Belangrijkste instellingen

| Instelling (`N` = tegelnummer) | Betekenis |
|---|---|
| `TILEN_ENTITY` | Entiteit waarop de actie wordt uitgevoerd |
| `TILEN_STATE_ENTITY` | Entiteit voor status/attributen; meestal dezelfde |
| `TILEN_TITLE` | Korte titel, liefst maximaal circa 12 tekens |
| `TILEN_TYPE` | Domein, bijvoorbeeld `light`, `scene`, `climate` |
| `TILEN_TAP_ACTION` | `auto`, `toggle` of `custom` voor onderstaande voorbeelden |
| `TILEN_TAP_SERVICE` | Expliciete HA-actie bij custom, bijvoorbeeld `script.turn_on` |
| `TILEN_LONGPRESS` | `none`, `slider` of (alleen tegel 6) `vacuum` |
| `TILEN_LONGPRESS_SLIDER` | `auto`; kiest passende regeling op basis van type |
| `TILEN_LABEL_ON/OFF` | Tekst voor eenvoudige aan/uitstatus |

De voorbeeldconfig vult ook de overige velden in. Laat die staan tenzij je
ze bewust gebruikt. `STATE_ENTITY` kan bij een scène/script een echte lamp of
schakelaar volgen; een scène heeft zelf geen betrouwbare aan/uitstatus.

## Voorbeelden

Wijzig deze sleutels in de bestaande map; plak niet nogmaals `substitutions:`.

Dimbare lamp op positie 1 (tik schakelt, lang indrukken opent de regeling):

```yaml
  TILE1_ENTITY: "light.keuken"
  TILE1_STATE_ENTITY: "light.keuken"
  TILE1_TITLE: "Keuken"
  TILE1_TYPE: "light"
  TILE1_TAP_ACTION: "auto"
  TILE1_LONGPRESS: "slider"
```

Een niet-dimbare lamp/schakelaar krijgt `LONGPRESS: "none"`. Kleurbediening
vereist dat de lamp de betreffende kleurmodus ondersteunt.

Scène op positie 3:

```yaml
  TILE3_ENTITY: "scene.avond"
  TILE3_STATE_ENTITY: "light.keuken"
  TILE3_TITLE: "Avond"
  TILE3_TYPE: "scene"
  TILE3_TAP_ACTION: "auto"
  TILE3_LONGPRESS: "none"
```

Script op positie 4:

```yaml
  TILE4_ENTITY: "script.alles_uit"
  TILE4_STATE_ENTITY: "script.alles_uit"
  TILE4_TITLE: "Alles uit"
  TILE4_TYPE: "script"
  TILE4_TAP_ACTION: "auto"
  TILE4_LONGPRESS: "none"
```

Airco op positie 5 (lang indrukken opent climate):

```yaml
  TILE5_ENTITY: "climate.woonkamer"
  TILE5_STATE_ENTITY: "climate.woonkamer"
  TILE5_TITLE: "Airco"
  TILE5_TYPE: "climate"
  TILE5_TAP_ACTION: "toggle"
  TILE5_TAP_SERVICE: "climate.toggle"
  TILE5_LONGPRESS: "slider"
```

Controleer `min_temp`, `max_temp`, `target_temp_step`, `hvac_modes` en de
ondersteunde fan-/swingmodes in HA. Deze kaart is primair voor een **enkel
setpoint**, niet een thermostaat die uitsluitend een temperatuurinterval
gebruikt. Test de modi van jouw integratie afzonderlijk; een andere airco
kan andere mogelijkheden hebben.

Robotstofzuiger op positie 6 (tik start; lang indrukken opent de kaart):

```yaml
  TILE6_ENTITY: "vacuum.robot"
  TILE6_STATE_ENTITY: "vacuum.robot"
  TILE6_TITLE: "Stofzuiger"
  TILE6_TYPE: "vacuum"
  TILE6_TAP_ACTION: "custom"
  TILE6_TAP_SERVICE: "vacuum.start"
  TILE6_LONGPRESS: "vacuum"
```

De vacuumkaart gebruikt start/pauze/stop/terugkeren/lokaliseren en de
snelheidsnamen `quiet`, `balanced`, `turbo`, `max`. Vergelijk deze met de
ondersteunde acties en `fan_speed_list` in HA. De snelheidsknoppen moeten in
de basiscode aangepast worden als jouw integratie andere namen verwacht.
Dit is geen universele kaart voor ieder robotmodel.

## Acties inschakelen en controleren

Zet na kalibratie en HA-koppeling `DIRECT_ACTIONS: "true"`. Geef in de
ESPHome-integratie toestemming voor Home Assistant-acties. Gebruik geen
HA-automation die dezelfde schermactie daarnaast nogmaals uitvoert; dat kan
een dubbele toggle veroorzaken. Bij `DIRECT_ACTIONS: "false"` kan de
actie-sensor nog gebeurtenissen publiceren voor eigen automations.

Flash `device.yaml` en test één tegel tegelijk: juiste status, juiste fysieke
actie, terugmelding vanuit HA en lange druk. Controleer ook dat externe
wijzigingen in HA op het scherm verschijnen. Voer daarna de
[acceptatietest](ACCEPTATIE.md) uit.

Extra iconen vereisen een glyph in de `materialdesign_icons`-fontlijst. De
bundel bevat de fonts lokaal. De oudere uitgebreide
[TILE_CONFIGURATION.md](../TILE_CONFIGURATION.md) beschrijft ook het oude
scrollprofiel; gebruik die niet als hardware-/layoutinstructie voor dit bord.
