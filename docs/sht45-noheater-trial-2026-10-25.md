# SHT45-Vergleich ohne Heizer

Versuch gestartet am 2026-10-25T01:34:26+02:00 (Europe/Zurich).
Firmware: `3.1.1-esp32s3-noheater`, installiert per OTA auf `192.168.31.211`.

Vor der Umstellung (Firmware 3.1.0):

- SHT45: 22,67 Grad C; SHTC3: 21,80 Grad C; Differenz Inbox minus Ambient: +0,87 Grad C.
- SHT45: 39,91 %rF roh, 40,91 %rF mit bisherigem Heiz-Feuchteoffset.
- Gespeicherter Abstand der SHT45-Messung zum Heizzyklus: 60010 ms.

Fuer den Versuch ist `SHT45_HEATER_ENABLED = false` gesetzt.
Mess- und Uebernahmeintervalle bleiben gleich (SHT45-Uebernahme alle 65 s).
Ohne Heizzyklus wird auch kein heizungsbezogener Feuchteoffset angewendet:
`humidity_offset_rh = 0`, `sht_heater_elapsed_ms = null`.
Es wird kein Temperaturoffset gesetzt. SHTC3 und SCD41 laufen unveraendert weiter.

Der Neustart trennt den Versuch durch eine neue Boot-ID von den bisherigen Daten.
Die Firmwarekennung markiert den Versuch zusaetzlich in der Live-API.
Ein thermisches Ergebnis ist direkt nach der Umstellung noch nicht belastbar;
Restwaerme und veraenderte Umgebungsbedingungen sind beim Verlauf zu beruecksichtigen.

## Ende des Versuchs

Der Benutzer meldete nach Betrieb ohne Heizung eine Temperaturdifferenz von null.
Auf seinen Wunsch wurde der bisherige Heizzyklus mit Firmware
`3.1.2-esp32s3` wieder aktiviert. Dies ist eine Beobachtung am aktuellen Aufbau,
keine absolute Kalibrierung der beiden Sensoren.

Nach Wiederaktivierung: Firmware 3.1.2 und neue Boot-ID 3910737168 bestaetigt.
Vor dem ersten Heizimpuls: SHT45 21,31 Grad C, SHTC3 21,26 Grad C, Differenz +0,05 Grad C.
