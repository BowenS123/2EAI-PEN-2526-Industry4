# C6015 IPC Herinstallatie - Contact met Beckhoff

Auteur: Bowen Liang  
Opleiding: Student Hogeschool PXL

## Situatie

Ik werk met een Beckhoff C6015 fanless ultra-compact Industrial PC.

Wat er gebeurd is:

1. Oorspronkelijk stond TwinCAT 3 op het toestel.
2. Nadien is per ongeluk TwinCAT 2 geinstalleerd.
3. Hierdoor is er vermoedelijk een conflict tussen TwinCAT 2 en TwinCAT 3 runtimes/drivers.
4. Deze installatie was bedoeld om een GigE Vision camera te gebruiken en te testen.
5. Momenteel kan het systeem niet naar Run Mode en blijft het in Config Mode hangen.

## Actie die ik heb ondernomen

Ik heb Beckhoff gecontacteerd met de vraag om hulp bij een correcte herinstallatie van de C6015.

Context uit de hulpvraag:

- Ik vond een installatiepad voor de C6915 en vroeg of dit bruikbaar is voor mijn C6015.
- Link die ik heb meegestuurd:
  - https://download.beckhoff.com/download/Software/embPC-Control/C6915/0010/CE/TC3/
- In het labo ben ik gewend te flashen via SD-kaart, maar de C6015 heeft geen SD-kaartslot.
- Daarom vroeg ik wat de correcte herinstallatieprocedure is voor dit toestel.

## Verzonden e-mail aan Beckhoff

Onderwerp: C6015 IPC herinstallatie

Beste Beckhoff,

Ik ben student aan PXL en werk met een C6015 fanless ultra-compact Industrial PC.
Oorspronkelijk stond er TwinCAT 3 op het toestel. Nadien heb ik onbedoeld TwinCAT 2 geinstalleerd. Ik heb vernomen dat dit mogelijk conflicten kan veroorzaken tussen de runtimes en drivers van TwinCAT 2 en TwinCAT 3.

Deze installatie heb ik uitgevoerd in het kader van het gebruik van een GigE Vision camera (Vision componenten), om deze te kunnen laten werken en testen.
Momenteel ondervind ik een probleem waarbij ik het toestel niet naar Run Mode kan schakelen. Het systeem blijft in Config Mode hangen.

Daarom probeer ik het systeem opnieuw te installeren op de C6015. Ik heb hiervoor volgende installatie gevonden:
https://download.beckhoff.com/download/Software/embPC-Control/C6915/0010/CE/TC3/

In het labo ben ik gewend om een installatie via een SD-kaart te flashen, maar de C6015 beschikt niet over een SD-kaartslot. Hierdoor ben ik niet zeker wat de correcte procedure is om dit toestel opnieuw te installeren. Ik heb de handleiding geraadpleegd, maar kon hier geen duidelijke instructies over terugvinden.

Alvast bedankt voor jullie hulp.

Met vriendelijke groeten,
Bowen Liang
Student Hogeschool PXL

## Doel

Via Beckhoff bevestigen:

1. De juiste methode om een C6015 opnieuw te installeren.
2. Hoe TwinCAT 2/TwinCAT 3 conflicten volledig te verwijderen.
3. Welke TwinCAT 3 + Vision componenten exact opnieuw nodig zijn om terug naar Run Mode te kunnen.