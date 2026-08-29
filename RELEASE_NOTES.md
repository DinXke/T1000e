Companion-firmware (BLE) voor de **Seeed Tracker T1000-E**, gebouwd op
MeshCore v1.17.1 (`d929643` — dezelfde basis als de officiële build).

## Wat zit erin

**Batterijstatus die klopt**
- Echte LiPo-ontlaadcurve in plaats van een rechte lijn van 3,0 naar 4,2V. Waar
  de oude formule op 3,7V nog "58%" zei, zegt deze 12% — dat is het verschil
  tussen een meter die dagenlang op de helft blijft hangen en er dan plots
  mee ophoudt, en een meter die meebeweegt.
- Je bestaande app hoeft niets te doen: de firmware rekent terug, zodat de
  lineaire formule van de app op het juiste percentage uitkomt. Ook op de
  telemetriepagina, zodat de app zichzelf niet tegenspreekt.
- Het echte percentage én de echte spanning gaan als extra bytes mee in het
  `PACKET_BATTERY`-frame, voor apps die het willen gebruiken.
- Telemetrie die een *ander* knooppunt bij jou opvraagt blijft altijd de echte
  spanning geven.

**Waarschuwing en nette afsluiting**
- Waarschuwingsdeuntje onder 3400 mV, uitschakelen onder 3200 mV — na drie
  metingen op rij, zodat een zendburst je toestel niet middenin een gesprek
  uitzet. De standaardfirmware had hier niets: die liep gewoon door tot de
  brownout.
- Startbeveiliging: op een te lege cel start het toestel niet op, in plaats van
  in een startlus te belanden tijdens een flash-schrijf.

**Zuiniger**
- BLE adverteert in rust één keer per seconde in plaats van elke 152,5 ms. De
  eerste 30 seconden na opstarten en na elke verbreking blijft snel, dus
  koppelen en herverbinden voelen hetzelfde.
- Rustige verbindingsparameters als er 20 seconden niets gebeurt, en meteen
  terug naar snel zodra er verkeer is.
- De batterijmeting cyclet de sensorrail nog één keer per 8 seconden in plaats
  van bij elke opvraag.

---

## Flashen — je instellingen blijven staan

Contacten, kanalen, node-naam, BLE-pin en je identiteit (sleutelpaar) blijven
behouden. Dat is geen belofte maar een eigenschap van de indeling: de
applicatie zit door de linker vastgezet tussen `0x27000` en `0xD4000`, en de
bestandssystemen liggen daarboven (`0xD4000` en `0xED000`). Zowel de `.uf2` als
de `.zip` schrijven alleen in het applicatiegebied.

De nieuwe instelling voor batterijkalibratie is toegevoegd op een manier die
oude configbestanden niet breekt: het voorkeurenformaat is sleutelgestuurd, dus
een ontbrekende sleutel houdt gewoon zijn standaardwaarde.

Maak toch even een back-up in de app voordat je flasht. Dat kost een minuut.

### Optie 1 — `.uf2` via USB (eenvoudigst, aanbevolen)

1. Sluit de T1000-E met USB aan.
2. Druk **twee keer snel** op de resetknop. Er verschijnt een schijf met de
   naam `T1000-E-BOOT`.
3. Sleep het `.uf2`-bestand erop.
4. Het toestel herstart zelf.

### Optie 2 — `.zip` via OTA (draadloos, met de nRF DFU-app)

1. Installeer **nRF Device Firmware Update** (App Store / Play Store).
2. Zet het toestel in OTA-modus: in de MeshCore-app naar de commandoregel, typ
   `start ota`. Je hoort `OK` terug te krijgen.
3. Zet in de DFU-app onder `Settings` de optie **Packet receipt notifications**
   aan en zet **Number of Packets** op 8.
4. Kies het `.zip`-bestand, kies het toestel (`T1000E_OTA`), en start de upload.

Lukt OTA niet meteen: bluetooth op je telefoon uit en aan, of het toestel
vergeten in je bluetooth-instellingen en opnieuw koppelen.

### Over flasher.meshcore.io

Die website serveert zijn eigen firmwarelijst; er is geen gedocumenteerde
manier om er een eigen bestand doorheen te sturen. Gebruik voor deze build dus
optie 1 of 2 hierboven. De `Console`-functie van die site werkt wél gewoon met
dit toestel, mocht je de kalibratie liever via USB doen.

---

## Doe dit één keer na het flashen: kalibreren

Dit is de belangrijkste stap. De spanningsmeting van de T1000-E is niet
vanzelfsprekend juist, en de hele curve steunt erop.

De T1000-E is IP65 en verzegeld, dus je komt niet bij de cel met een
multimeter. Daarom gebruikt deze firmware een referentie die je toch al hebt:
**de lader**. Een LiPo-lader kapt af op 4,20V, en het toestel kan aan de
`EXT_CHRG_DETECT`-pin zien wanneer dat gebeurd is. Kalibreren op elk ander
moment wordt geweigerd, want halverwege het laden meet je de lader.

### Vanuit de app — geen kabel naar een computer nodig

1. Laad volledig op. Het lampje ademt tijdens het laden en wordt rustig groen
   als hij klaar is.
2. **Laat de USB aangesloten.**
3. Zoek in de app bij dit knooppunt de custom variables / device settings. Je
   ziet `batt_mv` (huidige meting), `batt_pct` (via de curve) en `batt_cal`.
4. Zet `batt_cal` op `full`.
5. Lees `batt_mv` opnieuw — die hoort nu rond 4200 te staan.

Opgeslagen, en het overleeft een herstart.

### Of via USB, als je app die velden niet toont

Seriële terminal op 115200 baud (de `Console` op flasher.meshcore.io kan dit),
knop lang ingedrukt binnen 8 seconden na opstarten voor `CLI Rescue`, dan:

```
batt                        toont de meting en of het laadmoment goed is
set batt.calibrate full     kalibreert tegen de afgekapte lader
set batt.calibrate reset    terug naar de standaardinstelling
```

De volledige uitleg staat in [`docs/battery.md`](docs/battery.md).

## Wat nog niet is aangetoond

Wees hier eerlijk over voordat je dit op een toestel zet dat je nodig hebt:

- Deze firmware is **gebouwd en getest in CI, maar niet op hardware gedraaid**.
  Flash eerst op een toestel dat je kunt missen.
- De stroomwinst is **niet in mA gemeten**. De ingrepen zijn onderbouwd met wat
  de radio aantoonbaar minder doet, niet met een meting aan een echt toestel.
- LPCOMP-wake op spanning staat **uit**, omdat niet is vastgesteld dat het op
  dit board werkt. Wakker worden via de knop en via USB werkt gewoon.

Werkt er iets niet: terug naar de officiële build kan altijd met dezelfde
`.uf2`-methode, en ook die laat je instellingen staan.
