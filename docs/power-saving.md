# Stroombesparing

Drie ingrepen, allemaal per vlag aan of uit te zetten.

## 1. Langzamer adverteren (`BLE_POWER_SAVING=1`)

Standaard adverteert MeshCore 30 seconden lang elke 20 ms en daarna voor
onbepaalde tijd elke 152,5 ms. Een companion-knooppunt staat het grootste deel
van zijn leven *niet* verbonden te adverteren, dus dat tweede getal is wat de
cel daadwerkelijk leegtrekt.

Nu: nog steeds 20 ms in de eerste 30 seconden na opstarten en na elke
verbreking — koppelen en herverbinden voelen dus hetzelfde — maar daarna één
advertentie per seconde. Dat is ruwweg een factor 6,5 minder zendtijd in de
toestand waarin het toestel het grootste deel van de tijd verkeert.

Instelbaar met `BLE_ADV_INTERVAL_SLOW` (eenheden van 0,625 ms; 1600 = 1000 ms).

## 2. Rustige verbindingsparameters als er niets gebeurt

Zolang een app verbonden is, wisselen radio en telefoon elke 15–30 ms een
pakketje uit, ook als er niets te zeggen valt. Na `BLE_IDLE_MILLIS` (20 s)
zonder verkeer vraagt de firmware nu een rustiger interval aan (100–200 ms met
slave latency 4, effectief ongeveer één keer per seconde), en schakelt terug
zodra er in één van beide richtingen een frame beweegt.

De prijs is tot ongeveer een seconde extra vertraging op het eerste frame na
een stille periode. Daarna is het weer snel.

Twee dingen zijn bewust ingebouwd:

* er zit een ondergrens (`BLE_CONN_PARAM_MIN_GAP`, 2 s) tussen twee
  aanvragen, want de SoftDevice accepteert geen aaneengesloten
  parameterwijzigingen en een verkeersburst mag de verbinding niet laten
  stuiteren;
* de supervision timeout is ruim gekozen (6 s tegen een vereiste van
  (1+4) × 200 ms × 2 = 2 s), zodat de verbinding niet wegvalt zodra er een
  paar events gemist worden.

## 3. Zuiniger batterijmeting

`getBattMilliVolts()` zette bij *elke* aanroep de sensorrail aan, wachtte 10 ms
blokkerend op de ADC en zette de rail weer uit. Het protocol, de telemetrie en
de UI vroegen alle drie los van elkaar, dus dat gebeurde veel vaker dan de
meting verandert.

Nu: één meting per 8 seconden uit een cache (`BATT_CACHE_MILLIS`), en die ene
meting is de mediaan van vijf ADC-samples (`BATT_SAMPLE_COUNT`). Het
mediaanfilter is er niet voor de stroom maar voor de juistheid: de LR1110 trekt
tijdens een zendburst hard genoeg om een losse sample honderd millivolt of meer
omlaag te slepen, en die uitschieters mogen niet bij de ontlaadcurve of bij de
afsluitdrempel terechtkomen.

## 4. nRF52-startbeveiliging (`NRF52_POWER_MANAGEMENT`)

MeshCore heeft hier een module voor, maar de T1000-E stond in
`docs/nrf52_power_management.md` op "No" — niet geïmplementeerd. Dat is nu wel
zo:

* **Startbeveiliging**: onder `PWRMGT_VOLTAGE_BOOTLOCK` (3400 mV) weigert het
  toestel op te starten en gaat het meteen terug naar SYSTEMOFF. Dat voorkomt
  dat een lege cel zichzelf in een startlus praat en tijdens een flash-schrijf
  in een brownout loopt. Op USB wordt deze controle overgeslagen.
* **Afsluitreden onthouden**: waarom het toestel de vorige keer uitging (leeg,
  op verzoek, startbeveiliging) is terug te lezen.
* **Wakker worden**: de knop werkt altijd, en USB (VBUS) wekt het toestel —
  de lader insteken is dus de herstelweg.

### Wat hier bewust níét aan staat

LPCOMP-wake op spanning staat uit (`PWRMGT_ENABLE_LPCOMP_WAKE 0`). De
batterijdeler van de T1000-E zit achter de geschakelde sensorrail
(`PIN_3V3_EN`), en het is niet vastgesteld dat AIN0 in SYSTEMOFF met die rail
uit nog een bruikbare spanning ziet. De pinnen en de referentie-instelling
staan wel klaar in `variant.h`; zet de vlag pas op 1 als het op hardware is
nagemeten. VBUS-wake heeft er geen last van en werkt gewoon.

## Wat dit *niet* doet

Er is geen duty-cycling van de LoRa-ontvanger. Dat zou het knooppunt pakketten
laten missen en is een heel andere afweging dan hier gemaakt is: alle
bovenstaande ingrepen zijn onzichtbaar voor het mesh-verkeer.

Er zijn ook geen gemeten stroomcijfers bij dit werk. De ingrepen zijn
onderbouwd met wat de radio aantoonbaar minder doet (advertentie-interval,
verbindingsevents, rail-cycli), niet met een meting aan een echt toestel. Wil
je harde cijfers, meet dan de stroom met en zonder `BLE_POWER_SAVING=1`.
