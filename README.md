# ArduinOS

Een klein besturingssysteem voor de Arduino Uno. Het wordt **één keer** op de
Arduino geflasht en geeft de chip daarna:

- een **command line interface** (typen in de seriële terminal),
- een **bestandssysteem** in het EEPROM (blijft bewaard na stroomuitval),
- **geheugenbeheer** met variabelen,
- **multitasking**: meerdere programma's tegelijk draaien.

De programma's die het OS uitvoert zijn geen C, maar **bytecode** (zie
[`include/instruction_set.h`](include/instruction_set.h)). Het OS is dus eigenlijk
een kleine *virtuele machine* die bytecode-instructies één voor één uitvoert.
Bytecode-programma's maak je op de pc met de **converter** (zie `converter/`) en
stuur je via de seriële poort naar het bestandssysteem.

```
PC: programma.txt  ──convert──►  bytes  ──serieel──►  STORE  ──►  EEPROM-bestand
                                                                     │
                                          typ "run"  ──►  proces  ──►  interpreter
```

---

## Bestandsoverzicht

Elk onderdeel heeft een header (`.h`, de *wat*) en een implementatie (`.cpp`, de *hoe*).

| Bestand | Verantwoordelijk voor |
|---|---|
| [`src/main.cpp`](src/main.cpp) | Opstarten (`setup`) en de hoofd-loop (`loop`) |
| [`include/config.h`](include/config.h) | Alle instellingen/limieten (groottes, aantallen) op één plek |
| [`src/cli.cpp`](src/cli.cpp) | Command line: invoer lezen en commando's herkennen |
| [`src/filesystem.cpp`](src/filesystem.cpp) | Bestandssysteem (FAT) in het EEPROM |
| [`src/memory.cpp`](src/memory.cpp) | Werkgeheugen + variabelen (memory table) |
| [`src/stack.cpp`](src/stack.cpp) | De stack: getypte waarden tijdelijk opslaan |
| [`src/process.cpp`](src/process.cpp) | Process table: processen starten/pauzeren/stoppen |
| [`src/instructions.cpp`](src/instructions.cpp) | De bytecode-interpreter (voert instructies uit) |
| [`include/instruction_set.h`](include/instruction_set.h) | De opcodes (instructienummers), gelijk aan de converter |

---

## De onderdelen, één voor één

### 1. `config.h` — alle instellingen
Hier staan de constanten zodat nergens in de code een "toevallig getal" staat:
`MAX_PROCESSES 10`, `MEMORYSIZE 256`, `MEMTABLE_SIZE 25`, `STACKSIZE 32`,
`MAX_FILES 10`, en de proces-toestanden `RUNNING ('r')`, `PAUSED ('p')`,
`TERMINATED (0)`.

### 2. `main.cpp` — het startpunt
```c
void setup() {
  Serial.begin(9600);
  Serial.setTimeout(-1); // STORE blijft wachten op binnenkomende data
  fsBegin();             // bestandssysteem klaarzetten (alleen 1e keer formatteren)
  cliBegin();            // print "ArduinOS 1.0 ready"
}

void loop() {
  handleCLI();    // lees een commando (als er een is) en voer het uit
  runProcesses(); // geef elk draaiend proces één instructie
}
```
De `loop()` doet elke ronde twee dingen: kijken of je iets typte, en alle
processen een stapje laten zetten. Dat herhaalt zich duizenden keren per seconde.

### 3. `cli.cpp` — de command line
- **Non-blocking lezen:** `readToken()` leest invoer **letter voor letter**. Zo
  blokkeert het OS nooit terwijl je typt; de processen blijven doorlopen.
- **Commando's herkennen:** alle commando's staan in een array van
  `{naam, functie}`. `dispatch()` zoekt de getypte naam op en roept de bijbehorende
  functie aan. Onbekend? Dan print het de lijst met geldige commando's.
- **Argumenten lezen:** een commando als `run` leest zijn argument (de bestandsnaam)
  zelf met `waitForToken()`, dat blijft wachten én ondertussen de processen laat lopen.

De commando's: `store`, `retrieve`, `erase`, `files`, `freespace`, `run`, `list`,
`suspend`, `resume`, `kill`.

### 4. `filesystem.cpp` — het bestandssysteem (EEPROM)
Layout van het EEPROM: `[ FAT: 10 entries ][ noOfFiles ][ bestandsdata... ]`.
- De **FAT** (File Allocation Table) onthoudt per bestand: naam, startadres, lengte.
- `noOfFiles` staat zélf in het EEPROM (via `EERef`), zodat ook de telling
  stroomuitval overleeft.
- **Ruimte zoeken** gebeurt met een *gap search*: sorteer de FAT op startadres en
  zoek de eerste gat dat groot genoeg is (First Fit). Bestanden hoeven niet
  aaneengesloten te staan.
- Op een nieuwe chip wordt het bestandssysteem één keer geformatteerd (herkend aan
  een "magic byte").

### 5. `memory.cpp` — werkgeheugen + variabelen
- Een array van 256 bytes is het "RAM".
- De **memory table** onthoudt per variabele: naam, type, adres, grootte, en **bij
  welk proces** hij hoort (`processId`). Daardoor kunnen twee processen
  onafhankelijk een variabele met dezelfde naam hebben.
- Opslaan zoekt vrije ruimte met dezelfde gap search als het bestandssysteem.
- Als een proces stopt, geeft `clearProcessVariables()` al zijn variabelen vrij.

### 6. `stack.cpp` — de getypte stack
Elk proces heeft zijn eigen stack van 32 bytes. Waarden worden gepusht als
**eerst de bytes, dan een type-tag bovenop**. Bij het poppen lees je dus eerst het
type en weet je hoeveel bytes erbij horen. `selectStack()` wijst de push/pop-functies
naar de stack van het juiste proces, vlak voordat er een instructie draait.

### 7. `process.cpp` — de process table
- Een vaste **array van 10** `ProcessType`-structs. Elk struct = één proces, met
  o.a. `pid`, `state`, `pc` (program counter), `fp` (file pointer), `sp`, een
  loop-register en zijn eigen stack.
- Een leeg/vrij vakje herken je aan `state == TERMINATED (0)`.
- Hier zitten de commando's `run`, `list`, `suspend`, `resume`, `kill`.

### 8. `instructions.cpp` — de interpreter
`execute(index)` leest één byte (de opcode) van de program counter van dat proces
en voert via een grote `switch` de bijbehorende actie uit: een waarde op de stack
zetten, rekenen, printen, een variabele opslaan, springen (flow control), naar een
bestand schrijven, een ander proces forken, enzovoort. `runProcesses()` roept
`execute()` aan voor elk draaiend proces — dát is de multitasking.

---

## Stap voor stap: wat gebeurt er als ik `run test_vars` typ?

Stel je hebt `test_vars` al op de Arduino staan (via de converter). Je typt
`run test_vars` en drukt Enter. Dit gebeurt er, van begin tot eind:

**A. Het commando wordt gelezen** ([`cli.cpp`](src/cli.cpp))
1. In `loop()` draait `handleCLI()`. `readToken()` leest `r`, `u`, `n`, en stopt
   bij de spatie → het token `"run"` is compleet.
2. `dispatch("run")` zoekt `"run"` in de commando-array en roept `runCommand()` aan.

**B. Het proces wordt aangemaakt** ([`process.cpp`](src/process.cpp))
3. `runCommand()` roept `waitForToken(name)` aan en leest het argument
   `"test_vars"` (ondertussen blijven andere processen lopen).
4. `runCommand()` roept `startProcess("test_vars")` aan.
5. `startProcess()`:
   - `findFreeSlot()` zoekt een vrij vakje in de process table (state TERMINATED).
   - `getFileInfo("test_vars", start, size)` zoekt het bestand op in de FAT en
     vindt het **startadres** in het EEPROM.
   - Het vult het vakje in: naam, een uniek `pid` (`nextPid++`), `state = RUNNING`,
     **`pc = start`** (de program counter wijst naar de eerste byte van het bestand),
     `fp = start`, `sp = 0`.
   - Geeft het `pid` terug.
6. `runCommand()` print `Started process <pid>`. Het proces staat nu in de tabel
   met status RUNNING.

**C. Het proces wordt uitgevoerd, één instructie per ronde** ([`instructions.cpp`](src/instructions.cpp))
7. Bij elke `loop()` draait `runProcesses()`. Die loopt door de process table en
   ziet dat dit proces RUNNING is → roept `execute(index)` aan.
8. `execute()`:
   - `cur = &processTable[index]` (dit proces).
   - `selectStack(cur->stack, &cur->sp)` (gebruik de stack van dít proces).
   - Lees één byte op `cur->pc` (de opcode) en verhoog `pc`.
   - `switch` op de opcode → voer de actie uit.

   De bytecode van `test_vars` begint met `"test" SET s`. In bytes:
   `STRING 't' 'e' 's' 't' 0 SET 's'`. Dus:
   - **Ronde 1:** opcode = `STRING`. `readValue()` pusht de letters + lengte + type
     op de stack van het proces. `pc` staat nu na de string.
   - **Ronde 2:** opcode = `SET`. Leest de variabelenaam `'s'`, en `storeVariable('s', pid)`
     popt de string van de stack en slaat hem op als variabele `s` van dít proces.
   - **Volgende rondes:** zo gaat het verder — getallen pushen, `INCREMENT`,
     `PRINTLN` (print een waarde), enzovoort.
9. Tussen al die rondes door draaien `handleCLI()` en eventuele **andere** processen
   ook telkens één stap → alles lijkt tegelijk te lopen.

**D. Het proces stopt** ([`instructions.cpp`](src/instructions.cpp) + [`memory.cpp`](src/memory.cpp))
10. De laatste instructie van `test_vars` is `STOP`. Die roept
    `clearProcessVariables(pid)` aan (alle variabelen van dit proces vrijgeven) en
    zet `state = TERMINATED`.
11. Het vakje in de process table is daarmee weer vrij voor een volgend proces.

> Bij een oneindig programma (zoals `test_loop`) is er geen `STOP`; dat proces
> blijft draaien tot je het met `kill <id>` stopt — wat hetzelfde opruimwerk doet.

---

## Bouwen en uploaden

```
pio run -e uno            # compileren
pio run -e uno -t upload  # naar de Arduino flashen
```

Open daarna de seriële monitor op **9600 baud**; je ziet `ArduinOS 1.0 ready`.

## Bytecode-programma's erop zetten

Vanuit de `converter/`-map (seriële monitor dicht):
```
.\convert.exe test_vars COM3
```
Daarna in de monitor: `files` om te checken, en `run test_vars` om uit te voeren.
