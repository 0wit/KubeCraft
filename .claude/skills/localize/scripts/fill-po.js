const fs = require('fs');
const cs = {
  "Open or close the pause menu": "Otevřít nebo zavřít menu pauzy",
  "Language": "Jazyk", "Back": "Zpět", "Create": "Vytvořit", "Load World": "Načíst svět",
  "New World": "Nový svět", "Kubecraft": "Kubecraft", "Settings": "Nastavení", "Quit": "Ukončit",
  "World name": "Název světa", "Seed": "Seed", "Seed {Seed}  |  {SavedAt}": "Seed {Seed}  |  {SavedAt}",
  "No saved worlds": "Žádné uložené světy", "Paused": "Pauza", "Resume": "Pokračovat",
  "Save": "Uložit", "Save & Quit": "Uložit a odejít", "+": "+", "<": "<", ">": ">",
  "Accessibility": "Přístupnost", "Apply": "Použít", "Defaults": "Výchozí",
  "Keep displaying these settings?": "Ponechat toto nastavení obrazu?", "Keep": "Ponechat", "Revert": "Vrátit",
  "Video": "Grafika", "Resolution": "Rozlišení", "Window mode": "Režim okna", "Fullscreen": "Celá obrazovka",
  "Windowed fullscreen": "Okno bez okrajů", "Windowed": "V okně", "Quality": "Kvalita",
  "Low": "Nízká", "Medium": "Střední", "High": "Vysoká", "Epic": "Epická",
  "View distance (chunks)": "Dohled", "Chunks per frame": "Chunky za snímek",
  "Load": "Načíst", "Delete": "Smazat", "Can't be loaded": "Nelze načíst",
};
const file = process.argv[2];
const lines = fs.readFileSync(file, 'utf8').split(/(\r?\n)/);
let id = null, missing = new Set(), filled = 0;
for (let i = 0; i < lines.length; i++) {
  const m = lines[i].match(/^msgid "(.*)"$/);
  if (m) { id = m[1]; continue; }
  if (lines[i] === 'msgstr ""' && id) {
    if (id in cs) { lines[i] = 'msgstr "' + cs[id] + '"'; filled++; }
    else missing.add(id);
    id = null;
  }
}
fs.writeFileSync(file, lines.join(''));
console.log('filled', filled, 'missing', JSON.stringify([...missing]));
