# Batterijstatus op de T1000-E

## Wat er mis was

MeshCore stuurt rauwe millivolts naar de app. Iedereen die daar een percentage
van maakt — het batterij-icoon in de firmware zelf, en de companion-apps —
gebruikt dezelfde rechte lijn:

```
percentage = (mV - 3000) * 100 / (4200 - 3000)
```

Een LiPo ontlaadt niet lineair. Hij zakt snel van 4,20V naar ongeveer 3,85V,
hangt daar lang, en valt daarna in korte tijd door. De rechte lijn perst die
lange hangperiode samen tot ergens rond de helft, en blijft daarna nog "half
vol" melden als de cel al bijna leeg is. Dat is precies het gedrag dat je ziet:
de meter blijft dagenlang rond de 55% staan en het toestel valt daarna zonder
waarschuwing uit.

Daar kwam bij dat de T1000-E **helemaal geen lage-batterijbewaking had**.
`AUTO_SHUTDOWN_MILLIVOLTS` bestaat in MeshCore, maar wordt alleen afgehandeld
in `ui-tiny` en `ui-new`. De T1000-E bouwt `ui-orig`, en daar stond niets. Er
was dus geen waarschuwing en geen nette afsluiting: het toestel liep gewoon
door tot de brownout, met het risico dat een flash-schrijfactie op dat moment
het bestandssysteem meesleurt.

## Wat er nu gebeurt

### 1. Een echte ontlaadcurve

`overlay/src/helpers/BatteryCurve.h` bevat een stuksgewijs lineaire curve, met
dicht op elkaar liggende punten onder 3,9V waar de echte curve steil loopt.

| gemeten | curve | oude rechte lijn |
|--------:|------:|-----------------:|
| 4200 mV | 100 % | 100 % |
| 4000 mV |  77 % |  83 % |
| 3850 mV |  55 % |  70 % |
| 3800 mV |  40 % |  66 % |
| 3750 mV |  25 % |  62 % |
| 3700 mV |  12 % |  58 % |
| 3600 mV |   4 % |  50 % |
| 3400 mV |   2 % |  33 % |

Het nulpunt van de curve volgt automatisch `AUTO_SHUTDOWN_MILLIVOLTS`, zodat
"0%" en "het toestel schakelt zichzelf uit" hetzelfde moment zijn.

### 2. Spoofing, zodat bestaande apps kloppen

De firmware kan het percentage niet zelf doorgeven in het bestaande veld — het
protocol heeft daar alleen millivolts. In plaats van te wachten tot elke app
de curve overneemt, rekent de firmware terug: hij meldt de millivoltwaarde
waarvan de rechte lijn van de app precies het juiste percentage maakt.

```
echte meting 3750 mV  ->  curve zegt 25%  ->  firmware meldt 3300 mV
                                              app rekent (3300-3000)/12 = 25%  ✓
```

Dit gebeurt op twee plekken, allebei richting de gekoppelde app:

* het `PACKET_BATTERY`-frame (0x0C), dat de batterijmeter voedt;
* de *eigen* telemetrie die de app op "My Telemetry" toont.

Die tweede was nodig omdat de app zichzelf anders tegenspreekt: de meter bovenin
zegt 0% terwijl de telemetriepagina 20% toont, omdat die pagina dezelfde rechte
lijn op de rauwe spanning loslost.

**De prijs**: waar de app een *voltage* toont, toont hij nu een gefabriceerd
voltage. Dat is een bewuste ruil, en daarom staat de echte waarde op vier
plekken nog gewoon te lezen:

* het `STATS_TYPE_CORE`-frame (diagnostiek, nooit gespooft);
* telemetrie die je *op afstand* van dit knooppunt opvraagt (nooit gespooft —
  wie dit knooppunt van elders bewaakt, krijgt de waarheid);
* de extra bytes achter het `PACKET_BATTERY`-frame (zie hieronder);
* `batt` op de USB-CLI.

Alles is per vlag uit te zetten: `BATTERY_PERCENT_SPOOF`,
`BATTERY_TELEMETRY_SPOOF`, `BATTERY_USE_CURVE`, `BATTERY_EXT_FRAME`.

### 3. Het percentage ook echt meesturen

Met `BATTERY_EXT_FRAME=1` krijgt het `PACKET_BATTERY`-frame drie bytes extra
achter de gedocumenteerde elf:

```
byte  0     0x0C
bytes 1-2   batterijspanning (gespooft als BATTERY_PERCENT_SPOOF aan staat)
bytes 3-6   gebruikte opslag (KB)
bytes 7-10  totale opslag (KB)
byte  11    echte laadtoestand, 0-100          <- nieuw
bytes 12-13 echte spanning in mV               <- nieuw
```

Bestaande apps snijden de eerste elf bytes af of controleren `len >= 11`, dus
die merken er niets van. Een app die het wél weet, kan byte 11 rechtstreeks
gebruiken en heeft de hele omrekening niet meer nodig.

### 4. Waarschuwing en nette afsluiting

`ui-orig` heeft nu een batterijbewaker:

* onder `LOW_BATT_WARN_MILLIVOLTS` (3400): één waarschuwingsdeuntje en een
  melding. Eén keer per ontlading — na opladen wordt hij opnieuw scherp gezet.
* onder `AUTO_SHUTDOWN_MILLIVOLTS` (3200): het afsluitdeuntje en een nette
  `powerOff()`.

De afsluiting vraagt **drie opeenvolgende** metingen onder de drempel. Eén
meting tijdens een LoRa-zendburst kan flink inzakken op een cel die verder
prima is; zonder die eis zou het toestel zichzelf midden in een gesprek
uitzetten. Op boardniveau zit daar nog een mediaanfilter van vijf ADC-samples
vóór.

## Kalibreren — lees dit voordat je de drempels vertrouwt

De curve is precies zo goed als de spanningsmeting eronder, en die is op deze
board niet vanzelfsprekend juist. Er is een concrete aanwijzing dat er iets
scheef zit: een gezonde LiPo hangt het grootste deel van zijn leven rond
3,80–4,00V, maar dit toestel meldt dagenlang ~3,66V. Dat past bij een deler die
structureel 150–200 mV te laag leest, en het past net zo goed bij een cel die
werkelijk zo laag staat.

**De T1000-E is IP65 en verzegeld.** Je komt niet bij de celaansluitingen, dus
"meet even met een multimeter" is geen bruikbaar advies. Daarom gebruikt deze
firmware een referentie die je toch al in huis hebt: **de lader zelf**.

### De referentie: een lader die klaar is

Een LiPo-lader stopt bij een vaste eindspanning, in de praktijk 4,20V binnen
ongeveer een procent. Op het moment dat het laden klaar is, weet je dus wat de
cel doet — zonder iets open te maken.

De T1000-E heeft daar hardware voor: `EXT_CHRG_DETECT` (P1.3) is LAAG zolang de
lader stroom levert en gaat HOOG zodra hij afkapt. De firmware leest die pin en
weigert te kalibreren op elk ander moment, want kalibreren halverwege het laden
zet je metingen er blijvend naast zonder dat je het merkt.

### Zo doe je het — vanuit de app, geen kabel naar een computer

Dit is de aanbevolen route op een verzegeld toestel.

1. Laad de T1000-E volledig op. Het statuslampje ademt tijdens het laden en
   wordt rustig groen als hij klaar is.
2. **Laat de USB aangesloten.**
3. Open in de MeshCore-app de instellingen van dit knooppunt en zoek de
   custom variables / device settings. Je ziet daar:

   | naam | betekenis |
   |---|---|
   | `batt_mv` | wat de firmware nu meet |
   | `batt_pct` | dat, door de ontlaadcurve |
   | `batt_cal` | de correctiefactor die nu geldt |

4. Zet `batt_cal` op `full`.
5. Lees `batt_mv` opnieuw: die hoort nu rond 4200 te staan.

Klaar. De correctie wordt opgeslagen en overleeft een herstart.

Ziet de app die velden niet, dan kan hetzelfde via USB — zie hieronder.

### Zelfde ding via de USB-CLI

1. Sluit de T1000-E met USB aan en open een seriële terminal op 115200 baud.
   De `Console` op flasher.meshcore.io kan dit ook.
2. Houd de knop lang ingedrukt **binnen 8 seconden na het opstarten**. Je krijgt
   `========= CLI Rescue =========`.
3. `batt` — de regel `charger` vertelt je of het moment goed is.
4. `set batt.calibrate full`

### Andere mogelijkheden

* `set batt.calibrate 3840` of `batt_cal` = `3840` — als je op een of andere
  manier wél een gemeten waarde hebt. Wordt geweigerd zolang de lader stroom
  levert, want dan meet je de lader en niet de cel.
* `set batt.calibrate reset` of `batt_cal` = `reset` — terug naar de nominale
  deler van het board.
* `set adc.multiplier <getal>` — de factor rechtstreeks zetten (standaard 2.0).

### Wanneer een kalibratie geweigerd wordt

De firmware accepteert alleen correcties tot ongeveer een derde omhoog of
omlaag. Zit het verder uit elkaar, dan is het geen scheve deler maar iets
anders — een defecte cel, een verkeerd referentiegetal — en dat hoor je te
zien in plaats van weg te schalen achter geloofwaardig ogende getallen.

**Kalibreer vóórdat je de drempels aanpast.** Als de meting 180 mV te laag is,
schakelt een drempel van 3200 het toestel uit terwijl de cel nog op 3,38V staat.

## Waarom 3200 mV en niet 3400

Andere boards in MeshCore gebruiken 3300 of 3400. Voor deze is dat te hoog
gebleken: dit toestel draaide gewoon door op een gemelde 3250 mV, en elf
minuten later nog steeds op 3110 mV. Een drempel van 3400 had het meteen
uitgezet. 3200 laat het toestel dat laatste stuk uitrijden — dat blijkt maar
een handvol minuten te zijn — en zet het daarna netjes uit, met waarschuwing,
in plaats van het in een brownout te laten lopen.

Als je na kalibratie merkt dat de meting scheef stond, pas de twee drempels dan
mee aan in `variants/t1000-e/platformio.ini`.

## Testen

De curve en de beslislogica van de bewaker draaien op de host, zonder hardware:

```
make -C test
```

Dat controleert onder meer: monotoniteit over het hele bereik, dat de
spoofing-heenweg-en-terugweg exact het curvepercentage oplevert, dat één losse
lage meting níét afsluit, dat een ontbrekende meting (0 mV) nooit als lege
batterij telt, en dat de teller niet omklapt op een cel die lang leeg blijft.
