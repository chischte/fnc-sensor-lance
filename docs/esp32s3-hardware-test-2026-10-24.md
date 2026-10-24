# ESP32-S3 Hardwaretest vom 24.10.2026

**Endergebnis: ESP32-S3 laeuft erfolgreich.** Beide Sensoren liefern gueltige
Werte, WLAN und Flash-Puffer funktionieren. OTA einschliesslich Neustart
sowie Betrieb ohne USB mit externer 5-V-Versorgung sind erfolgreich geprueft.
Die unten protokollierten Sensorfehler wurden nach Korrektur der vertauschten
Leitungen behoben; sie beschreiben die vorausgehende Fehlersuche.

Geraet: ESP32-S3 Revision v0.2, MAC `14:c1:9f:5c:c8:b4`.
COM4 ist USB Serial/JTAG, COM5 der CH343-Anschluss desselben Boards.
Firmware: `3.0.0-esp32s3`. WLAN-Adresse beim Test: `192.168.31.211`.

## Erfolgreich geprueft

- USB-Upload mit esptool einschliesslich Flash-Hash-Pruefung.
- Start und PSRAM-Erkennung (8386295 nutzbare Bytes, 8-MB-Baustein).
- WLAN-Verbindung, Startseite und Update-Seite (HTTP 200).
- `/api/current`, `/api/measurement` einschliesslich JSON-Verlauf.
- LittleFS-Messpuffer, NDJSON-Auslieferung und Erhalt ueber Neustarts.
- Echtes OTA des App-BIN, HTTP 200, automatischer Software-Neustart,
  neue Boot-ID und erfolgreiche Wiederverbindung.
- OTA ueber `upload_firmware.ps1 -DeviceUrl http://192.168.31.211`.
- Ungueltiges OTA-Image wird mit HTTP 422 abgelehnt; kein Neustart,
  Flash-Puffer bleibt verfuegbar.
- 38 automatisierte Tests erfolgreich.

Beim ersten OTA-Test wurde eine Neustart-Race-Condition gefunden und behoben:
Schliesst der Client nach Empfang der Antwort sofort, wird ein bereits
verifiziertes Update jetzt trotzdem aktiviert. Der anschliessende Test
bestaetigt Software-Neustart und neue Boot-ID.

## Sensorpruefung noch nicht erfolgreich

Die Sensoren sind laut Benutzer angeschlossen. Startdiagnose bei 100 kHz:

```text
I2C SDA=8 SCL=9
I2C idle SDA=1 SCL=1
SHT45 I2C address 0x44 status=5
SCD41 I2C address 0x62 status=5
```

Status 5 ist laut lokalem ESP32-Wire-Treiber `ESP_ERR_TIMEOUT`.
Beide Sensoren liefern keine gueltigen Messwerte. Die Firmware sucht weiter;
die API liefert NULL mit Validitaetsflag false. Deshalb sind reale CO2-,
Temperatur-, Feuchte- und SHT45-Heizzykluspruefungen noch offen.
Die Leitungen sind bei der Startpruefung HIGH; ein dauerhaft auf LOW
festgehaltener Bus wurde dabei nicht beobachtet. Versorgung, Verdrahtung und
Pull-ups muessen am Aufbau geprueft werden; eine konkrete Ursache ist noch
nicht nachgewiesen.

## Wiederholung: nur SHT45 mit kurzem Kabel

Der Benutzer hat den SCD41 getrennt und den SHT45 direkt mit kuerzerem Kabel
angeschlossen. Erneut ueber API und serielle Startdiagnose getestet:
`SHT45 I2C address 0x44 status=5`. Nach einem vollstaendigen Messintervall
(`uptime_ms=77195`, `sht_read_uptime_ms=72587`, Sequenz 16) bleiben
`boxtemp` und `humidity` NULL, beide Validitaetsflags false.
Der ESP32 blieb waehrend der abschliessenden Beobachtung unter derselben
Boot-ID erreichbar. Kein SHT45-Heizzyklus nachweisbar, da kein gueltiger
Ausgangsmesswert vorhanden ist. Das fehlende SCD41-Signal ist bei diesem
Aufbau erwartet. Kabelverkuerzung und Abtrennen des zweiten Sensors haben
den SHT45-Kommunikationsfehler noch nicht behoben.

## Wiederholung nach verbesserter Stromversorgung

Nach der vom Benutzer verbesserten Stromversorgung erneut per HTTP-API
beobachtet. Waehrend der Abfrage wechselte die Boot-ID von `2388127005`
auf `2211524122`; ein Neustart wurde beobachtet, ohne dass der Test einen
Reset ausgeloest hat. Die Ursache ist nicht bestimmt.
Nach dem naechsten SHT45-Uebernahmezeitpunkt (`uptime_ms=74577`,
`sht_read_uptime_ms=74577`, Sequenz 15) bleiben Temperatur und Feuchte NULL,
mit Validitaetsflags false. LittleFS ist weiter bereit, die HTTP-API
antwortet. Eine erfolgreiche Sensorerfassung ist weiterhin nicht belegt.

## Wiederholung nach Korrektur der vertauschten Leitungen

Der I2C-Bus funktioniert nach dem Umstecken: SCD41 mit Seriennummer
`0A8815073B72` liefert gueltige CO2- und Feuchtewerte, beispielsweise
533 ppm und 44.35 % rF bei Sequenz 68. Die ausgelesene SCD41-Temperatur
betrug 29.47 Grad C, der konfigurierte Offset 2.2 Grad C.
SCD41-Messungen laufen ueber mehrere aufeinanderfolgende 5-s-Intervalle
mit unveraenderter Boot-ID `1364144317`.
SHT45 bleibt auch bei der naechsten regulaeren Uebernahme
(`sht_read_uptime_ms=339192`, Sequenz 68) ungueltig: `boxtemp` und
`humidity` sind NULL. Der Anschlusszustand beider Module muss nochmals
geklaert werden; der jetzt antwortende Sensor ist eindeutig ein SCD41.
Die NDJSON-Auslieferung aus dem aktuellen Flash-Puffer wurde erneut abgefragt.

## Erfolgreiche SHT45-Erfassung nach weiterer Anschlusskorrektur

Bei der erneuten Pruefung liefern erstmals beide Sensoren gleichzeitig
als gueltig markierte Werte. Sequenz 159, Boot-ID `1364144317`,
`sht_read_uptime_ms=794192`: SHT45 23.52 Grad C und 50.91 % rF;
SCD41 288 ppm CO2, 23.30 Grad C und 43.85 % rF.
Die SHT45-Werte stammen von einer neuen regulaeren Uebernahme, nicht von
einem wiederholten alten Wert. Die gueltigen SHT45-Daten wurden auch im
aktuellen LittleFS-NDJSON-Puffer nachgewiesen. Der Heiz-/Abkuehlzyklus
ist damit noch nicht vollstaendig separat geprueft.

## Wiederholungsmessung mit Heiz-/Abkuehlzeit

Beide Sensoren liefern bei Sequenz 185 weiterhin gueltige Werte:
SHT45 24.63 Grad C, 46.04 % rF roh bzw. 47.04 % rF mit +1 Prozentpunkt
Offset; SCD41 493 ppm, 26.62 Grad C und 45.15 % rF.
Der SHT45-Zeitstempel steigt von 859192 auf 924192 ms (65 s).
Die Firmware meldet `sht_heater_elapsed_ms=60010`, entsprechend einer
Messung nach mindestens 60 s Abkuehlzeit. Dies ist ein Firmware-Zeitnachweis,
keine separate elektrische Messung der Heizleistung. Boot-ID unveraendert,
Flash-Puffer bereit, SCD-Fehlerzaehler 0.

## Erfolgreicher Betrieb mit externer Versorgung ohne USB

Versorgung laut Benutzer: 5 V am 5-V-Pin, USB getrennt. Das Geraet ist unter
`192.168.31.211` erreichbar; die ARP-Tabelle bestaetigt die bekannte MAC
`14:c1:9f:5c:c8:b4`. Bei der abschliessenden Pruefung vor dem Commit:
Boot-ID `2924729219`, Sequenz 33, Uptime 162187 ms, SCD-Fehlerzaehler 0,
LittleFS bereit. SHT45: 24.40 Grad C und 41.71 % rF roh bzw. 42.71 % rF
korrigiert; SCD41: 337 ppm, 24.51 Grad C und 46.63 % rF.
Alle vorhandenen Sensorwerte sind als gueltig markiert. Die SHT45-Messung
weist 60010 ms Abkuehlzeit auf. Der ESP32-S3 laeuft damit auch ohne USB
an der externen Versorgung erfolgreich.
