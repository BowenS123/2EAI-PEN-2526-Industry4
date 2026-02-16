# Firmware

> Deze map bevat alle firmware code voor de microcontroller(s)
> ```
> src/
> ├── README.md
> ├── [project-name]/
> │   ├── main.c
> │   ├── Makefile
> │   └── ...
> └── libs/
> ```
>
> Als er meerdere applicaties zijn, gebruik dan deze README.md als landing-page en voeg folders toe:
> 
> ```
> src/
> ├── backend/          # API server
> │   ├── app.py
> │   └── requirements.txt
> ├── firmware/          # PSoC6 embedded code
> │   ├── README.md
> │   ├── libs/
> │   ├── builds/
> ├── frontend/
> │   ├── README.md
> │   ├── index.html
> │   ├── css/
> └── pc-tools/         # Python data visualisatie
> 	├── dashboard.py
> 	└── requirements.txt
> ```
> 

## 💻 Software Stack

| Component | Versie | Doel |
|-----------|--------|------|
| **IDE** | ModusToolbox | Ontwikkelomgeving |
| **MCU** | PSoC 6 | Target microcontroller |
| **Taal** | C | Programmeertaal |

## 🚀 Aan de slag

### Project openen

1. Open ModusToolbox
2. File → Open → Navigate to `src/[project-name]/`
3. Select the project
4. Build → Build Project (Ctrl+B)
5. Run → Debug (F11)

## 🗂️ Code Structuur

### main.c
```
Main programma loop:
1. Initialisatie
2. Configuratie van peripherals
3. Main loop met functionaliteit
```

### Belangrijke modules:
- **[Module1]:** [Functie]
- **[Module2]:** [Functie]

## 📝 Pinout

> **Documenteer welke pinnen gebruikt worden**

| Pin | Functie | Verbonden met |
|-----|---------|---------------|
| P0_0 | UART TX | USB-Serial |
| P0_1 | UART RX | USB-Serial |
| P5_0 | LED | Onboard LED |
| ... | ... | ... |

## ⚠️ Bekende problemen

**Probleem:** [Beschrijving]
**Oplossing:** [Fix of workaround]

## 📚 Resources

- [PSoC 6 Datasheet](/docs/datasheets/P5oK6.pdf)

## ✅ Checklist

- [ ] ModusToolbox geïnstalleerd
- [ ] Project compileert zonder errors
- [ ] Code upload werkt
- [ ] Basis functionaliteit (LED blink) werkt
- [ ] Seriële communicatie werkt
- [ ] Dependencies gedocumenteerd
- [ ] Pinout tabel ingevuld
