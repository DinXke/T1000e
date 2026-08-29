# Herkomst en licenties

## Waar deze code vandaan komt

Dit repository bevat **patches op MeshCore**, niet een kopie ervan. `apply.sh`
haalt MeshCore zelf op bij de bron, op de vastgezette commit in
`meshcore.lock`.

| Onderdeel | Herkomst | Licentie |
|---|---|---|
| MeshCore (alles wat de patches raken) | [ripplebiz/MeshCore](https://github.com/ripplebiz/MeshCore) @ `d929643` (v1.17.1) | MIT — © 2025 Scott Powell / rippleradios.com |
| `overlay/src/helpers/BatteryCurve.{h,cpp}` | nieuw, geschreven voor dit project | MIT (zie hieronder) |
| De patches in `patches/` | nieuw, geschreven voor dit project | MIT |

MeshCore is MIT-gelicentieerd. Dat staat afgeleide werken uitdrukkelijk toe,
inclusief aanpassen en verspreiden, mits de copyrightvermelding en de
licentietekst meegaan. Die staan in `license.txt` van de MeshCore-checkout die
`apply.sh` maakt, en blijven daar ongewijzigd staan.

## Over EasySkyMesh

De aanleiding voor dit werk was een vergelijking met de
`t1000e_companion_radio_blePowerSaving17.1`-build van
[IoTThinks/EasySkyMesh](https://github.com/IoTThinks/EasySkyMesh).

**Er zit geen EasySkyMesh-code in dit repository.** Wat er met die build is
gedaan, en niet meer dan dat:

* de meegeleverde `.uf2`/`.zip` is uitgepakt en er is `strings` op gedraaid, om
  vast te stellen welke MeshCore-versie eronder zit;
* het resultaat: de enige tekstuele afwijking ten opzichte van de officiële
  `v1.17.1-d929643`-build is de versiebanner `PowerSaving17.1`. Er is geen
  disassembly gedaan en er is niets uit het binair afgeleid;
* van GitHub is alleen het `README.md` en het licentiebestand opgehaald, om de
  licentievraag te kunnen beantwoorden.

Alle wijzigingen hier zijn zelf geschreven, op basis van de MeshCore-broncode.
Waar het idee ("langzamer adverteren, rustigere verbindingsparameters als er
niets gebeurt") overeenkomt met wat EasySkyMesh doet, is dat omdat het de
voor de hand liggende manier is om BLE-verbruik te drukken op deze stack — niet
omdat er iets is overgenomen.

### Wat de licentie van EasySkyMesh zegt

Twee repositories, en het onderscheid is hier belangrijk:

* **[IoTThinks/EasySkyMesh](https://github.com/IoTThinks/EasySkyMesh)** — de
  distributieplek voor de kant-en-klare binaries. Deze heeft **geen
  licentiebestand en geen licentieveld** op GitHub. Zonder licentie geldt het
  standaardregime: alle rechten voorbehouden. Je mag zo'n binary downloaden en
  flashen, maar je mag er niets uit overnemen of hem herdistribueren.
* **[IoTThinks/MeshCore](https://github.com/IoTThinks/MeshCore)** — waar hun
  README naar verwijst voor de broncode. Dit is een fork van MeshCore en draagt
  gewoon `license.txt` met de MIT-licentie van Scott Powell / rippleradios.com.

Praktisch: hun *broncode* is MIT en had dus gebruikt mogen worden met
bronvermelding; hun *binaries* niet. Voor dit repository maakt het niet uit,
want er is uit geen van beide iets overgenomen.

Als je alsnog rechtstreeks iets uit `IoTThinks/MeshCore` wilt overnemen, is dat
toegestaan onder MIT, en hoort daar een vermelding van hun copyright bij in dit
bestand.

## Licentie van dit repository

MIT, zodat het naadloos samengaat met MeshCore en teruggegeven kan worden aan
upstream. Zie `LICENSE`.
