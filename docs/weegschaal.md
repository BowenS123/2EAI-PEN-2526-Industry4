# Weegschaal meten met RS232 interface

Deze weegschaal met RS232 interface stuurt het gemeten gewicht als ASCII-tekst via een seriële verbinding naar een ontvanger (PC / PLC). De communicatie verloopt via UART, wat betekent dat beide apparaten op dezelfde baudrate en dataformaat moeten zijn ingesteld.

## Verbinding (pinout) met PC

Voor een male **9-pins D-sub (DB9)** connector op de weegschaal naar een male **9-pins** op PC/RS232-USB kabel:

Kabel nodig: female to female

| Weegschaal (9p) | PC (9p) | Functie |
|----------------|---------|---------|
| Pin 2 (IN)     | Pin 3   | RX ← TX |
| Pin 3 (OUT)    | Pin 2   | TX → RX |
| Pin 5 (GND)    | Pin 5   | GROUND  |

## Algemene configuratie

- soort communicatie: UART (9600 bps)
- data formaat in ASCII code: 10 bits:
  - 0 = startbit
  - 1 = stopbit
  - 8 bits data

## Data output

| Positie | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---------|---|---|---|---|---|---|---|
| Inhoud  | `+` of `-` | Spatie | Data of `.` | Data of `.` | Data of `.` | Data of `.` | Data of `.` |

| Positie | 8 | 9 | A | B | C | D | E |
|---------|---|---|---|---|---|---|---|
| Inhoud  | Data of `.` | Data of `.` | Data of `.` | Unit 1 | Unit 2 | End | Return |

## Data output settings

### 1. Continu sturen (Continuous output)

1. Press & hold `COU` button tot `C0-OFF` displayed
2. Press `TARE` button & select `C5`
3. Press `COU` button & select `C5-0`
4. Lastly press `TARE` button to enter "continuous output"

### 2. Key output sturen (On demand)

1. Press & hold `COU` button tot `C0-OFF` displayed
2. Press `TARE` button & select `C5-`
3. Press `COU` button & select `C5-2`
4. Lastly press `TARE` button to enter "key output"
    
    Press the `UNIT` button to output weighing data.

### Voorbeeldoutput

```
+ 123.45 g<CR>
```
- Positie 1: `+` (positief)
- Positie 2-A: `   123.45` (getal met decimale punt)
- Positie B-C: `g` (eenheid)
- Positie D: End
- Positie E: Return (CR)

Zie: `\src\Weegschaal\testWeegschaal.py` voor weegschaal te testen met uw PC.

## PLC

### Configuratie PLC
- **PLC**: Beckhoff TwinCAT
- **Communicatiemodule**: EL6002 (Seriële interface)
- **Protocol**: RS-232 serieel
- **Weegschaal**: Seriële digitale weegschaal
- **Tc2_SerialCom**: Beckhoff Library voor seriële communicatie

### Installatie & Configuratie in TwinCAT project
  1. Importeer Beckhoff Serial Communication library
  2. Voeg FB_ParseWeight toe aan je project
  3. Implementeer de Function blocks in uw project zoals in `\src\Weegschaal\WeegschaalPLC\WeegschaalPLC\Weegschaal\POUs\MAIN.TcPOU`
  4. Configureer EL6002 in de I/O tree (link RX & TX)
    
  5. Licentie Vereisten ?

### Function Blocks

#### 1. `SerialLineControl` (Tc2_SerialCom Beckhoff Library)
- **Doel**: Beheert de low-level seriële communicatie
- **Configuratie**: `SERIALLINEMODE_EL6_22B` mode specifiek voor EL6002
- **Cyclisch**: Elke PLC cyclus uitgevoerd

#### 2. `ReceiveString` (Tc2_SerialCom Beckhoff Library)
- **Doel**: Ontvangt en buffert string data van de seriële buffer
- **Timeout**: 1 seconde
- **Data Terminator**: `$R$N` (Carriage Return + Line Feed)

#### 3. `FB_ParseWeight` (Custom)
- **Doel**: Parseert ruwe weegschaal String data (teken, gewicht, eenheid) & controleert meetstabiliteit
- **Features**:
  - String parsing 

