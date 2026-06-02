# 🧾 Bill of Materials (BOM)
# 📌 Overzicht

Deze BOM bevat alle mechanische, elektrische en besturingscomponenten
voor de automatisatie van de productielijn (transport → mengen → robotarm → controle).

---

# 🏗️ 1. Mechanische Componenten

| ID | Component | Specificatie | Aantal | Leverancier | Link | Artikelnummer | Prijs (€) | Hebben | Opmerking |
|----|------------|--------------|--------|--------------|------|---------------|-----------|--------|------------|
| M-01 | Weegschaal | Industriële platform weegschaal | 1 | 	TBD | TBD | TBD | TBD | Ja | Gewichtmeting |
| M-02 | DC-Motor Transportband | RS PRO Geared DC Motor, 24 V DC, 20 Ncm, 75 rpm, 5 mm shaft | 1 | RS PRO | TBD | 258-8316 | TBD | Ja | Conveyor aandrijving |
| M-03 | DC-Motor Mixer | 24V DC | 1 | TBD | TBD | TBD | TBD | Ja | Basis mengmotor |
| M-05 | Servo-Motor Robotarm | High torque servo | 1 | TBD | TBD | TBD | TBD | Ja | Hoofdaandrijving |
| M-06 | Steppermotor Robotarm | drylin E stappenmotor, NEMA 17, gevlochten draad | 3 | igus | TBD | MOT-AN-S-060-005-042-L-A-AAAA | TBD | Ja | Robotarm assen |
| M-07 | Motor Drivers | drylin D7 stepper motor driver (NEMA11/17) | 3 | igus | TBD | D7_control | TBD | Ja | Driver voor steppermotoren |
| M-08 | Kabelrups | e-chain E08, 20 mm breed | 1 | Conrad | TBD | E08-20-038-0 | 41,99 | Nee | Kabelmanagement voor connectors |
| M-09 | Schroeven set | M3, 35 mm | 1 | TBD | TBD | TBD | TBD | Nee | Assortiment doosje aanbevolen |

---

# 🔍 2. Sensoren & Detectie

| ID | Component | Type | Aantal | Leverancier | Link | Artikelnummer | Prijs (€) | Hebben | Functie |
|----|------------|------|--------|--------------|------|---------------|-----------|--------|----------|
| S-01 | Inductieve Sensor | M12 / PNP / 24V | 1 | TBD | TBD | TBD | TBD | Ja | Positiedetectie |
| S-02 | Lichtsluis | Foto-elektrisch | 1 | TBD | TBD | TBD | TBD | nee | Objectdetectie |
| S-03 | Batch-reader | GigE Vision | 1 | Beckhoff | TBD | TBD | TBD | Ja | Barcode / batchcontrole |
| S-04 | Sensor Kabel | M12 connector, 4-pin, 2 m | 1 | Mouser | TBD | PXPPNP12FBF04ACL050PVC | 40,64 | Nee | Verbinding inductieve sensor |
| S-05 | Camera (GigE) | Mako G-040C PoE | 1 | Allied Vision | TBD | G-040C | TBD | Ja | Vision camera voor TC Vision |

---

# 🧠 3. Besturing & Elektronica

| ID | Component | Specificatie | Aantal | Leverancier | Link | Artikelnummer | Prijs (€) | Hebben | Functie |
|----|------------|--------------|--------|--------------|------|---------------|-----------|--------|----------|
| C-01 | PLC | Industriële controller | 1 | TBD | TBD | TBD | TBD | Nee | Hoofdbesturing |
| C-02 | PLC Klemmen | Terminal blocks | TBD | TBD | TBD | TBD | TBD | Nee | Bekabeling |
| C-03 | ESP | ESP32 Dev Board | 1 | TBD | TBD | TBD | TBD | Nee | Communicatie |
| C-04 | PSoC | Programmable SoC (robotarm) | 1 | TBD | TBD | TBD | TBD | Ja | Embedded controle |
| C-05 | EL7332 | EtherCAT Terminal, 2-channel motion interface, DC motor, 24 V DC, 1 A | 1 | Beckhoff | TBD | EL7332 | TBD | Ja | DC motor aansturing |
| C-06 | EL1008 | EtherCAT Terminal, 8-channel digital input, 24 V DC, 3 ms | 1 | Beckhoff | TBD | EL1008 | TBD | Ja | Digitale ingangen |
| C-07 | EL6001 | EtherCAT Terminal, 1-channel communication interface, serial, RS232 | 1 | Beckhoff | TBD | EL6001 | TBD | Ja | RS232 communicatie |
| C-08 | Header Connectors | 4-pin, 2.5 mm pitch | 3 | Mouser | TBD | 61900411121 | 1,15 | Nee | Voor componentverbindingen |
| C-09 | Voedingsbron | Mean Well HEP-240-48A, 48 V DC, 5 A, 240 W | 1 | Conrad | TBD | HEP-240-48A | 129,99 | Ja | Opmerking: 48 V max, 10 A gewenst |
| C-10 | Industrie PC | Beckhoff C6105 | 1 | Beckhoff | TBD | C6105 | TBD | Ja | Runtime target voor TC Vision |
| C-11 | PoE Injector | 30W PoE injector (INJ POE-30W) | 1 | TBD | TBD | INJ POE-30W | TBD | Ja | Voeding voor PoE camera |

---

# 🔄 4. Subsystemen

## 🚚 Transport
- DC-Motor Transportband  
- Lichtsluis  
- Inductieve Sensor  
- Weegschaal  

## 🥣 Mengstation
- DC-Motor Mixer  
- Servo-Motor Mixer  

## 🤖 Robotarm
- Servo-Motor Robotarm  
- Steppermotoren  
- Motor Drivers  

## 📷 Controle
- Batch-reader (GigE)  

## 🧠 Besturing
- PLC  
- PLC Klemmen  
- ESP  
- PSoC  

---

# 💰 Totale Kost (optioneel)

| Categorie | Subtotaal (€) |
|------------|----------------|
| Mechanisch | TBD |
| Sensoren | TBD |
| Besturing | TBD |
| **TOTAAL** | TBD |

---

# 📝 Notities

- Controleer spanningscompatibiliteit (24V systeem).
- Drivers moeten overeenkomen met motor type.
- PLC I/O aantal nog te bepalen.
