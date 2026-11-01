# Beckhoff PLC

De hoofdsolution staat in
`plc/PannenkoekenSysteem/Pannenkoek/Pannenkoek.sln` en bevat de TwinCAT 3
Structured Text-besturing, GVL’s, visualisatie en `FB_QrVision`.

## Openen en uitvoeren

1. Installeer TwinCAT 3 XAE in Visual Studio.
2. Open de solution hierboven.
3. Controleer de TwinCAT Vision-package en de hardwaretarget.
4. Build de solution en activeer de configuratie.
5. Login naar de PLC-runtime en start het programma.
6. Teach de posities `Camera`, `Scale`, `Mixer`, `Inductive` en `Discard`.

Test eerst zonder product en controleer altijd de noodstop. De volledige
procesintegratie is nog niet gevalideerd onder continue belasting.

## Belangrijke onderdelen

- `POUs/MAIN.TcPOU` — centrale proceslogica en state machine
- `POUs/FB_QrVision.TcPOU` — GIGE-camera, QR-decoding en contourweergave
- `VISUs/PannenkoekenDashboard.TcVIS` — operatorinterface
- `GVLs/` — gedeelde procesdata en configuratie

Zie [`docs/PLC_Documentatie.md`](../docs/PLC_Documentatie.md) voor de
communicatie, gebruikersbediening en foutafhandeling.
