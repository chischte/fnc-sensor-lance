# Komponentenbilder im Anschlussplan

Der Anschlussplan `ESP32-C3_SuperMini_Layout.svg` verwendet Bildausschnitte
aus den vorhandenen Projektunterlagen:

- DC/DC-Wandler, ESP32-C3 SuperMini, SCD41 und SHT45:
  `fnc-fogger/docs/fnc-fogger-layout-v4.png` aus dem benachbarten Projekt.
- SHTC3: ursprünglicher Sensorplan `ESP32-S3_CAM_Layout.png` aus der
  Git-Historie dieses Repositorys.

Die Originalbilder sind in der SVG eingebettet; SVG-Viewports begrenzen
die sichtbaren Ausschnitte. Die SVG benötigt daher keine externen Bilddateien.
Das ESP32-Bild ist um 180 Grad gedreht, damit USB-C oben liegt.

Die Pin-Tabelle und der kompakte Sensorbus beschreiben die Anschlüsse dieses
Projekts: SDA GPIO4, SCL GPIO5, Sensorversorgung 3,3 V und gemeinsame Masse.
Die Sensoranschlüsse im Bus sind logisch dargestellt; die tatsächliche
Pinreihenfolge hängt vom jeweiligen Modul ab.
