# Docs Folder

Deze folder bevat alle documenten met bijhorende media en resource bestanden voor het project.

## Structuur

```
docs/
├── images/              # Afbeeldingen
├── videos/              # Video bestanden
└── datasheets/          # Datasheets
```

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
