# Docs Folder

Deze folder bevat alle documenten met bijhorende media en resource bestanden voor het project.

## Structuur

```
docs/
├── Bomlijst.md
├── Hardware_Diagram.drawio.xml
├── InstallatieTCVision.md
├── MOSQUITTO_SETUP.md
├── PLC_Documentatie.md
├── Report turning cilinder.pdf
├── TASKS.md
├──	weegschaal.md
├── images/              # Afbeeldingen
├── videos/              # Video bestanden
└── datasheets/          # Datasheets
	├── DMXSl20.pdf
	├── DRE_DATA_MOT-AN-S-060-005-042-L-A-AAAA_EN.pdf
	├── Linear_actuator adapters 3D.pdf
	├── Operating Manual dryve D7-1 V1.0 EN.pdf
	├── P5oK6.pdf
	└── Stepmotor_lab.pdf
    ├── Linear_actuator adapters 3D.docx
    ├── Stepmotor_lab.docx
    ├── git-cheat-sheet-education.pdf
	├── Mako G-040_Datasheet.pdf
	├── C6015_Industrial_PC.pdf
```

## Referentie documenten

- `docs/datasheets`: [Datasheets map](./datasheets/)
- `docs/datasheets/DMXSl20.pdf`: [DMXSl20.pdf](./datasheets/DMXSl20.pdf)
- `docs/datasheets/DRE_DATA_MOT-AN-S-060-005-042-L-A-AAAA_EN.pdf`: [DRE_DATA_MOT-AN-S-060-005-042-L-A-AAAA_EN.pdf](./datasheets/DRE_DATA_MOT-AN-S-060-005-042-L-A-AAAA_EN.pdf)
- `docs/datasheets/Linear_actuator adapters 3D.pdf`: [Linear_actuator adapters 3D.pdf](./datasheets/Linear_actuator%20adapters%203D.pdf)
- `docs/datasheets/Operating Manual dryve D7-1 V1.0 EN.pdf`: [Operating Manual dryve D7-1 V1.0 EN.pdf](./datasheets/Operating%20Manual%20dryve%20D7-1%20V1.0%20EN.pdf)
- `docs/datasheets/P5oK6.pdf`: [P5oK6.pdf](./datasheets/P5oK6.pdf)
- `docs/datasheets/Stepmotor_lab.pdf`: [Stepmotor_lab.pdf](./datasheets/Stepmotor_lab.pdf)

## Bronbestanden (DOCX)

- docs/Linear_actuator adapters 3D.docx
- docs/Stepmotor_lab.docx

## Afbeeldingen

**Afbeeldingen in Markdown**

```markdown
# In README.md (root)
![Logo](./docs/images/screenshot-dashboard.png)

# In docs/design.md
![Diagram](../docs/images/diagrams/architecture-v1.png)
```

**Naming Convention:**

```
[type]-[description]-[optional-variant].ext

Voorbeelden:
- screenshot-dashboard-light.png
- diagram-architecture-v2.png
- logo-primary.svg
- icon-user-active.svg
```
```
diagram-[type]-[description]-v[version].png

Voorbeelden:
- diagram-architecture-system-v1.png
- diagram-erd-database-v2.png
- diagram-flow-user-registration-v1.png
```

**Optimalisatie:**

- Comprimeer afbeeldingen voordat je ze commit
- Gebruik tools zoals TinyPNG, ImageOptim
- Houd bestandsgroottes onder 500KB waar mogelijk
- Gebruik voor diagrammen Draw.io of Mermaid (in Markdown)

## Diagrammen

Een goed voorbereid en professioneel gedocumenteerd project bevat relevante diagrammen die de structuur, werking en interacties binnen het systeem verduidelijken. Afhankelijk van de aard van het project kunnen dit onder meer zijn:

- Architecture diagrams
- Database ERD
- User flow diagrams
- Sequence diagrams
- Component diagrams
