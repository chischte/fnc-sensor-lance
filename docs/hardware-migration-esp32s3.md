# Hardwarewechsel am 24.10.2026

Bis einschliesslich Commit `e215ccc` wurde mit Arduino Portenta Machine Control
gearbeitet. Der lokale Git-Tag `portenta-machine-control-final` markiert diesen
vollstaendigen Ausgangsstand mit PT100 und Portenta-OTA.

Ab Firmware 3.0.0-esp32s3 ist ESP32-S3-DEV-KIT-N8R8 das Standardziel:
8 MiB Flash, 8 MiB Octal-PSRAM. SHT45 und SCD41 teilen einen I2C-Bus.

| Leitung | Anschluss |
| --- | --- |
| Blau / SDA | GPIO 8 |
| Gelb / SCL | GPIO 9 |
| Rot / VCC | 3V3 |
| Schwarz / GND | GND |

Die Tabelle beschreibt das am Aufbau verwendete Grove-Kabel in der Reihenfolge
Schwarz, Rot, Blau, Gelb. Blau ersetzt hier die ueblicherweise weisse
SDA-Leitung. Die Farbzuordnung des urspruenglichen Bildes traf auf dieses
Kabel nicht zu und wurde beim Hardwaretest korrigiert.

I2C laeuft mit 100 kHz. PT100, RTD-Treiber und RTD-Diagnosen sind entfernt.
Das API-Feld `outertemp` bleibt fuer bestehende Logger als `null` erhalten;
`valid.outertemp` ist false. Historische Daten im Python-Logger bleiben lesbar.

Der Messpuffer liegt auf LittleFS statt Portenta-QSPI. Zwei App-Partitionen
von je 2 MiB erlauben OTA; 4032 KiB bleiben fuer LittleFS. Zwei rotierende
Messdateien sind auf je etwa 1.5 MiB begrenzt. Ein fehlgeschlagenes Mounten
fuehrt zur Formatierung der Datenpartition; SQLite bleibt Langzeitspeicher.

Erstinstallation ueber USB: `pio run -e esp32s3 -t upload --upload-port COMx`.
WLAN-Zugangsdaten wie bisher in `../wifi-credentials.h` hinterlegen.
Danach `http://<geraete-ip>/update` oeffnen und ausschliesslich
`.pio/build/esp32s3/firmware.bin` hochladen (kein merged/factory BIN,
keine Portenta-Datei). Alternativ:
`./upload_firmware.ps1 -DeviceUrl http://<geraete-ip>`.
Der Upload prueft das Image mit der ESP32-Update-Bibliothek und aktiviert erst
bei erfolgreichem Abschluss die andere App-Partition. Abbruch belaesst die
laufende Firmware. Die Update-Seite hat wie bisher keine Anmeldung und ist
fuer ein vertrauenswuerdiges lokales WLAN vorgesehen.

Der Portenta-Build ist weiterhin explizit mit `-e portenta_h7_m7` verfuegbar,
jetzt ebenfalls ohne PT100. Die vollstaendige alte Firmware steht im Git-Tag.
