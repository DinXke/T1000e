# T1000-E companion firmware — batterijstatus en stroombesparing

Patches op [MeshCore](https://github.com/ripplebiz/MeshCore) voor de Seeed
Tracker T1000-E, die twee dingen bij elkaar brengen:

* **een batterijmeter die klopt** — een echte LiPo-ontlaadcurve in plaats van
  een rechte lijn van 3,0 naar 4,2V, een lage-batterijwaarschuwing en een nette
  afsluiting vóór de brownout, plus een manier om bestaande companion-apps het
  juiste percentage te laten tonen zonder dat die iets hoeven aan te passen;
* **stroombesparing** — langzamer BLE-adverteren, rustige
  verbindingsparameters als er niets gebeurt, een zuiniger batterijmeting en de
  nRF52-startbeveiliging die op dit board nog niet was aangezet.

Dit repository bevat **geen kopie van MeshCore**. `apply.sh` haalt MeshCore op
bij de bron, op de vastgezette commit in `meshcore.lock` (`d929643`, v1.17.1 —
exact de basis van de officiële `t1000e_companion_radio_ble`-build), en zet
daar de patches op.

## Bouwen

```bash
./build.sh
```

Dat cloont MeshCore, past alles toe en bouwt de env
`t1000e_companion_radio_ble_ps`. Resultaat in `meshcore/out/`:

* `*.uf2` — dubbelklik de resetknop en sleep het bestand op de
  `T1000-E-BOOT`-schijf;
* `*.zip` — nRF DFU-pakket, voor OTA of `nrfutil`.

Je hebt PlatformIO nodig (`pip install platformio`). Als dat lokaal niet lukt,
bouwt de GitHub Actions-workflow in `.github/workflows/build.yml` het en hangt
de artefacten aan de run.

De standaard `t1000e_companion_radio_ble`-env blijft onaangeroerd naast de
nieuwe staan, dus je kunt beide bouwen en vergelijken.

## Testen

```bash
make -C test
```

Draait de ontlaadcurve en de beslislogica van de batterijbewaker op de host —
geen hardware, geen embedded toolchain nodig. Dit is de code die je toestel kan
uitschakelen, dus die is los getrokken en uitgetest in plaats van alleen
beweerd.

## Documentatie

* [`docs/battery.md`](docs/battery.md) — wat er mis was, wat de curve doet,
  hoe de spoofing werkt en **hoe je kalibreert** (lees dit voordat je de
  drempels vertrouwt);
* [`docs/power-saving.md`](docs/power-saving.md) — wat er precies zuiniger is,
  wat het kost, en wat er bewust níét in zit;
* [`NOTICE.md`](NOTICE.md) — herkomst en licenties, inclusief de vraag of er
  iets van EasySkyMesh in zit (nee).

## Wat er verandert, in het kort

| | standaard | hier |
|---|---|---|
| percentage uit spanning | rechte lijn 3,0–4,2V | stuksgewijze LiPo-curve |
| percentage naar de app | app rekent zelf, verkeerd | firmware corrigeert; echte waarde blijft leesbaar |
| batterijpercentage in het protocol | niet aanwezig | byte 11 van `PACKET_BATTERY` |
| lage-batterijwaarschuwing | geen | deuntje + melding onder 3400 mV |
| automatisch uitschakelen | geen | onder 3200 mV, na 3 metingen op rij |
| ADC-kalibratie | niet ondersteund op dit board | `set batt.calibrate <mV>`, opgeslagen |
| BLE-adverteren in rust | elke 152,5 ms | elke 1000 ms |
| BLE-verbinding in rust | 15–30 ms | 100–200 ms, latency 4 |
| batterijmeting | elke aanroep, 1 sample | 1× per 8 s, mediaan van 5 |
| startbeveiliging op lege cel | geen | weigert te starten onder 3400 mV |

## Status

Gebouwd en getest in CI ([run #3](https://github.com/DinXke/T1000e/actions/runs/33270524295)):

* de patch applyt schoon op een verse `d929643`-clone, en levert byte-identiek
  de tree op die is getest;
* de firmware compileert en linkt — RAM 61,6% (145.000 van 235.520 bytes),
  flash 49,3% (349.576 van 708.608 bytes), dus ruim binnen de grenzen;
* `.uf2` (699 KB) en `.zip` (350 KB) hangen als artefact aan de run;
* de hosttests op de ontlaadcurve en de beslislogica van de batterijbewaker
  zijn groen.

**Wat hiermee nog niet is aangetoond**, en waar je zelf naar moet kijken:

* dat het op hardware doet wat het hoort te doen. Het is gecompileerd, niet
  gevlogen. Flash eerst op een toestel dat je kunt missen;
* **de juistheid van de spanningsmeting**. Zie `docs/battery.md` — kalibreer
  met een multimeter voordat je de drempels vertrouwt. Zonder die stap is elk
  percentage hier een aanname;
* de daadwerkelijke stroomwinst in mA. De ingrepen zijn onderbouwd met wat de
  radio aantoonbaar minder doet, niet met een meting;
* of LPCOMP-wake op dit board werkt. Daarom staat die uit; VBUS-wake (lader
  insteken) werkt wel.
