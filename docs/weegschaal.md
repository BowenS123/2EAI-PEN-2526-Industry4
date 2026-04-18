# Weegschaal meten met RS232 interface

Deze weegschaal met RS232 interface stuurt het gemeten gewicht als ASCII-tekst via een seriële verbinding naar een ontvanger (PC / PLC). De communicatie verloopt via UART, wat betekent dat beide apparaten op dezelfde baudrate en dataformaat moeten zijn ingesteld.

## Verbinding (pinout) met PC

Voor een **9-pins D-sub (DB9)** connector op de weegschaal naar een **9-pins** op PC/RS232-USB kabel:

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
| Inhoud  | Data of `.` | Data of `.` | Unit 1 | Unit 2 | Unit 3 | End | Return |

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

123.45 g<CR>
- Positie 1: `+` (positief)
- Positie 2-9: `   123.45` (getal met decimale punt)
- Positie A-C: `g` (eenheid)
- Positie D: End
- Positie E: Return (CR)