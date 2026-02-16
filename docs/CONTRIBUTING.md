# Hoe werken met dit project?

> **Voor studenten die nieuw zijn met Git/GitHub**

---

## 🎯 Doel

Deze guide helpt je om te werken met Git en GitHub voor dit project, zonder ingewikkelde IT-zaken.

---

## 📝 Basis Git Workflow

### 1. Repository clonen (eerste keer)

```bash
git clone [repository-url]
cd [project-name]
```

### 2. Nieuwe feature/wijziging maken

**Maak ALTIJD een nieuwe branch aan:**

```bash
# Zorg dat je op main staat
git checkout main

# Haal laatste wijzigingen op
git pull origin main

# Maak nieuwe branch
git checkout -b feature/mijn-wijziging
```

**Branch namen:**
- `feature/naam` - Voor nieuwe features
- `bugfix/naam` - Voor bug fixes
- `docs/naam` - Voor documentatie updates
- `hardware/naam` - Voor hardware wijzigingen

Enkele goede voorbeelden zijn:

- feature/add-sensor-reading
- bugfix/fix-pwm-timing
- docs/update-hardware-readme

### 3. Werk aan je code

Maak je wijzigingen in de bestanden...

### 4. Commit je wijzigingen

```bash
# Zie welke bestanden gewijzigd zijn
git status

# Voeg bestanden toe om te committen
git add [bestandsnaam]
# Of alles: git add .

# Commit met duidelijke boodschap
git commit -m "Beschrijf wat je gedaan hebt"
```

**Goede commit messages:**
- ✅ "Add temperature sensor to hardware schematic"
- ✅ "Fix PWM frequency calculation in firmware"
- ✅ "Update README with setup instructions"
- ❌ "update"
- ❌ "fixed stuff"
- ❌ "asdf"


### 5. Push naar GitHub

```bash
git push origin feature/mijn-wijziging
```

### 6. Maak een Pull Request

1. Ga naar GitHub repository in je browser
2. Je ziet een banner: "Compare & pull request" - klik daarop
3. Vul de Pull Request template in:
   - Beschrijf wat je gedaan hebt
   - Vink de checklist items af
   - Voeg screenshots toe indien relevant
4. Klik "Create pull request"

### 7. Code review

1. Teamgenoot bekijkt je code
2. Eventueel feedback verwerken
3. Na approval: Merge de Pull Request

---

## 🔍 Code Review Checklist

**Wanneer je een Pull Request van een teamgenoot reviewt, check:**

- [ ] Code compileert/draait zonder errors
- [ ] Bestand namen en structuur kloppen
- [ ] README's zijn geüpdatet (indien nodig)
- [ ] Geen debug prints of test code achtergebleven
- [ ] Geen wachtwoorden of keys in de code
- [ ] Begrijp je wat de code doet? (anders vragen om uitleg)

**Tips voor goede feedback:**
- Wees constructief: "Dit werkt, maar zou X beter zijn?"
- Stel vragen: "Waarom heb je dit zo gedaan?"
- Geef complimenten: "Goed opgelost!"

---

## 🔄 Dagelijkse workflow

**Begin van de dag:**
```bash
git checkout main
git pull origin main
```

**Werk aan feature:**
```bash
git checkout feature/mijn-feature
# werk...
git add .
git commit -m "Duidelijke beschrijving"
git push origin feature/mijn-feature
```

**Einde van de dag:**
- Push je werk naar GitHub
- Maak PR als je feature klaar is

---

## 🚫 Wat NIET te doen

### ❌ Direct op main werken

**Fout:**
```bash
git checkout main
# wijzigingen maken
git commit -m "changes"
git push origin main
```

**Goed:**
```bash
git checkout -b feature/mijn-feature
# wijzigingen maken
git commit -m "Add feature X"
git push origin feature/mijn-feature
# Maak PR op GitHub
```

### ❌ Grote binaire bestanden committen

Niet committen:
- Compiled code (.exe, .o, .bin)
- Build outputs
- Grote afbeeldingen (>5MB)
- Video bestanden
- ZIP/RAR archieven

### ❌ Wachtwoorden/keys in code

**Fout:**
```python
API_KEY = "abc123supersecret"  # ❌ NOOIT DOEN
```

**Goed:**
```python
# Gebruik environment variables
API_KEY = os.getenv('API_KEY')
```

---

## 🆘 Veelvoorkomende problemen

### "Merge conflict"

**Wat is het?**
Twee mensen hebben hetzelfde bestand gewijzigd.

**Oplossing:**
1. `git pull origin main` om laatste versie te halen
2. Open het bestand met conflict
3. Zoek naar `<<<<<<<` markers
4. Kies welke versie je wilt houden
5. Verwijder de conflict markers
6. `git add [bestand]` en `git commit`

### "Can't push - rejected"

**Oplossing:**
```bash
git pull origin main
git push origin feature/mijn-feature
```

### "Vergeten een branch aan te maken"

**Je hebt wijzigingen op main:**
```bash
# Maak nieuwe branch met je wijzigingen
git checkout -b feature/mijn-feature
git push origin feature/mijn-feature
```

---

## 📚 Handige Git commando's

```bash
# Status bekijken
git status

# Zie wat er gewijzigd is
git diff

# Log van commits
git log --oneline

# Branch overzicht
git branch -a

# Wissel van branch
git checkout [branch-naam]

# Laatste commit ongedaan maken (voorzichtig!)
git reset --soft HEAD~1

# Alle lokale wijzigingen weggooien (voorzichtig!)
git reset --hard HEAD
```

---

## 🎓 Git leren

**Basis tutorials:**
- [GitHub Git Guide](https://github.com/git-guides)
- [Git Cheat Sheet](https://education.github.com/git-cheat-sheet-education.pdf)

**Interactieve tutorials:**
- [Learn Git Branching](https://learngitbranching.js.org/)

---

## 💡 Tips voor samenwerking

1. **Communiceer:** Zeg tegen je team waar je aan werkt
2. **Pull regelmatig:** Haal dagelijks de laatste wijzigingen op
3. **Kleine commits:** Commit vaak met duidelijke messages
4. **Review elkaars code:** Leer van elkaar
5. **Documenteer:** Update README's wanneer je iets wijzigt

---

## ❓ Vragen?

- Maak een GitHub Issue met label `question`
- Vraag je teamgenoten
- Check de README's in elke map

**Succes! 🚀**
