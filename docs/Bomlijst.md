# 🧾 Bill of Materials (BOM)
# 📌 Overzicht

Deze BOM bevat alle mechanische, elektrische en besturingscomponenten
voor de automatisatie van de productielijn (transport → mengen → robotarm → controle).

---

# 🏗️ 1. Mechanische Componenten

| ID | Component | Specificatie | Aantal | Leverancier | Link | Artikelnummer | Prijs (€) | Hebben | Opmerking |
|----|------------|--------------|--------|--------------|------|---------------|-----------|--------|------------|
| M-01 | Weegschaal | Industriële platform weegschaal | 1 |  | TBD | TBD | Nee | Gewichtmeting |
| M-02 | DC-Motor Transportband | RS PRO Geared DC Motor, 24 V DC, 20 Ncm, 75 rpm, 5 mm shaft | 1 | RS PRO |  | 258-8316 | TBD | Nee | Conveyor aandrijving |
| M-03 | DC-Motor Mixer | 24V DC | 1 |  | TBD | TBD | Nee | Basis mengmotor |
| M-04 | Servo-Motor Mixer | Industriële servo | 1 | TBD | TBD | TBD | TBD | Nee | Precisie snelheidsregeling |
| M-05 | Servo-Motor Robotarm | High torque servo | 1 | TBD | TBD | TBD | TBD | Ja | Hoofdaandrijving |
| M-06 | Steppermotor Robotarm | drylin E stappenmotor, NEMA 17, gevlochten draad | 3 | igus | | MOT-AN-S-060-005-042-L-A-AAAA | TBD | Ja | Robotarm assen |
| M-07 | Motor Drivers | drylin D7 stepper motor driver (NEMA11/17) | 3 | igus |  | D7_control | TBD | Ja | Driver voor steppermotoren |

---

# 🔍 2. Sensoren & Detectie

| ID | Component | Type | Aantal | Leverancier | Link | Artikelnummer | Prijs (€) | Hebben | Functie |
|----|------------|------|--------|--------------|------|---------------|-----------|--------|----------|
| S-01 | Inductieve Sensor | M12 / PNP / 24V | 1 | TBD | TBD | TBD | TBD | Nee | Positiedetectie |
| S-02 | Lichtsluis | Foto-elektrisch | 1 | TBD | TBD | TBD | TBD | Nee | Objectdetectie |
| S-03 | Batch-reader | GigE Vision | 1 | Beckhoff | TBD | TBD | TBD | Ja | Barcode / batchcontrole |

---

# 🧠 3. Besturing & Elektronica

| ID | Component | Specificatie | Aantal | Leverancier | Link | Artikelnummer | Prijs (€) | Hebben | Functie |
|----|------------|--------------|--------|--------------|------|---------------|-----------|--------|----------|
| C-01 | PLC | Industriële controller | 1 | TBD | TBD | TBD | TBD | Nee | Hoofdbesturing |
| C-02 | PLC Klemmen | Terminal blocks | TBD | TBD | TBD | TBD | TBD | Nee | Bekabeling |
| C-03 | ESP | ESP32 Dev Board | 1 | TBD | TBD | TBD | TBD | Nee | Communicatie |
| C-04 | PSoC | Programmable SoC (robotarm) | 1 | TBD | TBD | TBD | TBD | Ja | Embedded controle |
| C-05 | EL7332 | EtherCAT Terminal, 2-channel motion interface, DC motor, 24 V DC, 1 A | 1 | Beckhoff |  | EL7332 | TBD | Nee | DC motor aansturing |
| C-06 | EL1008 | EtherCAT Terminal, 8-channel digital input, 24 V DC, 3 ms | 1 | Beckhoff |  | EL1008 | TBD | Ja | Digitale ingangen |
| C-07 | EL6001 | EtherCAT Terminal, 1-channel communication interface, serial, RS232 | 1 | Beckhoff |  | EL6001 | TBD | Ja | RS232 communicatie |

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

# 📦 Aankoopstatus

| Component ID | Besteld | Ontvangen | Getest | Opmerking |
|--------------|---------|-----------|--------|------------|
| M-01 | ⬜ | ⬜ | ⬜ | |
| C-01 | ⬜ | ⬜ | ⬜ | |

---

# 📝 Notities

- Controleer spanningscompatibiliteit (24V systeem).
- Drivers moeten overeenkomen met motor type.
- PLC I/O aantal nog te bepalen.
