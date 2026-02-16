# PLC Programma

> Deze map bevat het TwinCAT PLC programma (Structured Text)
> ```
> plc/
> ├── README.md              # Dit bestand
> ├── [ProjectName]/        # TwinCAT solution
> │   ├── [ProjectName].sln
> │   ├── [ProjectName].tsproj
> │   └── PLC/              # PLC project
> │       ├── POUs/         # Program Organization Units
> │       ├── VISUs/        # Visualisaties (optioneel)
> │       └── GVLs/         # Global Variable Lists
> ```


## 💻 Software Stack

| Component | Versie | Doel |
|-----------|--------|------|
| **IDE** | TwinCAT 3 (XAE) | Ontwikkelomgeving |
| **Taal** | Structured Text (ST) | Programmeertaal |
| **Runtime** | TwinCAT 3 Runtime | PLC runtime op Windows PC |

## 🚀 Aan de slag

### Project openen

1. Open TwinCAT XAE (vanuit Visual Studio 2022)
2. File → Open → Project/Solution
3. Navigate to `plc/[ProjectName]/[ProjectName].sln`
4. Open de solution
5. Build → Build Solution (Ctrl+Shift+B)
6. Activate Configuration
7. In Solution Explorer → rechtsklik op PLC Project → Login
8. Start programma

## 🗂️ Project Structuur

### POUs (Program Organization Units)

**MAIN (PRG):**

```
Main programma loop:
- Initialisatie
- State machine (optioneel)
- Hoofdlogica
```

**Andere POUs:**
- `[FunctionBlock1]` (FB): [Functie]
- `[Function1]` (FUN): [Functie]

### GVLs (Global Variable Lists)

- `GVL_Config`: Configuratie parameters
- `GVL_Inputs`: Alle inputs
- `GVL_Outputs`: Alle outputs
- `GVL_Data`: Data variabelen

### State Machine (indien van toepassing)

```
States:
1. INIT      - Initialisatie
2. IDLE      - Wachten op start
3. RUNNING   - Actief proces
4. STOPPING  - Veilig stoppen
5. ERROR     - Fout afhandeling
```

**State overgangen:**

- INIT → IDLE: Na succesvolle initialisatie
- IDLE → RUNNING: Start button ingedrukt
- RUNNING → STOPPING: Stop button ingedrukt
- ANY → ERROR: Bij foutconditie

### Visualisatie (optioneel)

Beschrijf visualisatie indien aanwezig en vermeld als het een target, HMI of andere type visualisatie betreft.

## ⚙️ Configuratie Parameters

### Belangrijke parameters in GVL_Config:

| Parameter | Type | Default | Beschrijving |
|-----------|------|---------|--------------|
| `fCycleTime` | REAL | 0.01 | Cycle tijd (s) |
| ... | ... | ... | ... |

## ⚠️ Veiligheidsopmerkingen

- ⚠️ Controleer noodstop functionaliteit
- ⚠️ Test programma eerst zonder hardware verbonden
- ⚠️ Gebruik safe state bij opstarten
- ⚠️ [Andere veiligheidsaspecten]

## 📚 Resources

- [DMX Controller](/docs/datasheets/DMXSl20.pdf)

## ✅ Checklist

- [ ] TwinCAT 3 XAE geïnstalleerd
- [ ] Project opent zonder errors
- [ ] Project compileert succesvol
- [ ] I/O geconfigureerd en gelinked
- [ ] PLC programma start in RUN mode
- [ ] Basis functionaliteit getest
- [ ] State machine gedocumenteerd (indien van toepassing)
- [ ] I/O mapping tabel ingevuld
- [ ] Controleer of TwinCAT Runtime actief is
- [ ] Check licentie
- [ ] Herstart TwinCAT System
- [ ] Controleer netwerk verbinding (EtherCAT)
- [ ] Check IP configuratie
