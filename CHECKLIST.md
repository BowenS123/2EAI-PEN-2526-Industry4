# 📋 Project Setup Checklist

> **Doel**: Deze checklist helpt jullie stap voor stap door de setup van jullie project
>
> **Werk deze af in de eerste 2 weken!**
>
> Nadien verwijder je deze file!!!

---

## Week 1: Basis Setup

### ✅ Repository Setup

- [ ] Repository naam aangepast naar jullie projectnaam
- [ ] README.md titel vervangen door jullie projectnaam  
- [ ] Teamleden tabel ingevuld met echte namen en GitHub usernames
- [ ] Project beschrijving ingevuld (2-3 zinnen)
- [ ] Overview foto toegevoegd in `docs/images/` (foto van breadboard, schets, of concept)

### ✅ .gitignore Controle

- [ ] `.gitignore` bekeken en aangepast indien nodig
- [ ] Getest dat grote bestanden/build outputs niet worden gecommit

### ✅ Planning

- [ ] Project overzicht geschreven ("Sequence of operation")
- [ ] Rollen verdeeld in het team (wie doet hardware, firmware, software, etc.)
- [ ] Eerste GitHub issues aangemaakt voor de taken

---

## Week 2: Documentatie per Onderdeel

### ✅ Hardware (`hardware/README.md`)

- [ ] Lijst van componenten ingevuld (wat hebben jullie nodig?)
- [ ] Blokdiagram of schets toegevoegd (mag handmatig zijn in week 2) in `docs/diagrams/`
- [ ] Eventuele datasheets toegevoegd in `docs/datasheets/`

### ✅ Software (`src/README.md`)

- [ ] ModusToolbox project aangemaakt
- [ ] Basis project structuur beschreven
- [ ] Dependencies/libraries gedocumenteerd
- [ ] Eerste test code (blink LED) werkend

### ✅ PLC (`plc/README.md`)

- [ ] TwinCAT project aangemaakt
- [ ] Project structuur beschreven
- [ ] Eerste simpel programma (bijv. simpele logic) werkend

---

## Week 3+: Ontwikkeling

### ✅ Git Workflow

> **💡 Voor gedetailleerde git instructies, zie [docs/CONTRIBUTING.md](./docs/CONTRIBUTING.md)**

- [ ] Branches aanmaken voor features (niet direct op main werken)
- [ ] Pull Requests gebruiken
- [ ] Code reviews doen binnen het team

**Quick reference branch namen:**
- `feature/naam` - Voor nieuwe features
- `bugfix/naam` - Voor bug fixes
- `docs/naam` - Voor documentatie updates
- `hardware/naam` - Voor hardware wijzigingen

### ✅ Documentatie Bijhouden

- [ ] README's updaten wanneer er iets verandert
- [ ] Screenshots/foto's toevoegen van resultaten in `docs/images/`
- [ ] Problemen documenteren in GitHub Issues

---

## 🚨 Veelgemaakte Fouten

### ❌ Niet Doen

- Grote binaire bestanden (>10MB) committen
- Wachtwoorden of API keys in code zetten
- Rechtstreeks op `main` branch werken zonder review
- KiCAD backup bestanden committen (`*-bak`, `*-cache.lib`)
- Build outputs committen (alleen source code!)

### ✅ Wel Doen

- Regelmatig committen met duidelijke messages (zie [CONTRIBUTING.md](./docs/CONTRIBUTING.md) voor voorbeelden)
- Branches gebruiken voor nieuwe features
- README's up-to-date houden
- Bestanden lokaal opslaan in `docs/` (geen externe links)
- Pull Requests gebruiken voor code review

---

## 📞 Hulp Nodig?

- **Git problemen?** → Zie [docs/CONTRIBUTING.md](./docs/CONTRIBUTING.md) voor troubleshooting
- **Project vragen?** → Maak een GitHub Issue met label `question`
- Tag je docent (en teamgenoten) in issues of PR comments
- Check de README's in elke folder voor specifieke instructies

---

## ✅ Klaar voor Review?

Als alle Week 1 en Week 2 items zijn afgevinkt:

1. Maak een Pull Request naar `main`
2. Titel: "Initial project setup and documentation"
3. Tag je docent voor review
4. Wacht op feedback voordat je merged

**Succes! 🚀**
