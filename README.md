# Projekt 2: IPK24-server

Tento dokument popisuje návrh a implementaci serverové aplikace schopné komunikovat se vzdálenými klienty pomocí protokolu IPK24-CHAT.

## Abstraktní návrh

Před samotnou implementací je klíčové mít nějakou představu o tom, jak začít. Prvním krokem je řešit příjem zpráv od více klientů najednou, a to jak pomocí UDP, tak i TCP protokolu, což vyžaduje asynchronní přístup. Dále je nutné každému klientovi přiřadit jedinečný identifikátor a spravovat jejich různé stavy, což můžeme řešit pomocí pole, seznamu nebo struktury informací. Takto se můžeme rozhodnout, komu poslat kterou zprávu. Kontrola formátu přijatých zpráv je dalším důležitým krokem, který lze provést pomocí regulárních výrazů. A pozdější problémy se vyřešily za chodu samotné implementace.

## Chování programu
#### `main()`
- Program se spustí a čeká na události na TCP a UDP sockety.
- Při příchodu příchozích zpráv na UDP socket nebo na nový port UDP provede odpovídající operace.
- Při stisknutí klávesové zkratky pro ukončení (Ctrl+C) uzavře všechny sockety.

#### `udp_client()`
- Funkce přijímá zprávy od klientů na UDP soketu.
- Zpracovává přijaté zprávy v souladu se stavem klienta.
- Odpovídá na zprávy, zasílá potvrzení a aktualizuje stav klienta.
- V případě chyby komunikace nebo odpojení klienta ukončuje spojení a odstraňuje klienta z globálního pole.

## Implementace

#### Popis `main()` funkce:

1. **Registrace signálu SIGINT:**
   - Funkce `signal()` registruje `SIGINT` pomocí funkce `ctrlc()`.
   - `ctrlc()` pošle všem klientům zprávu `BYE` a uzavře sockety.

2. **Parsování argumentů:**
   - Funkce `args_parse()` zpracuje argumenty předané programu a uloží je do struktury `server_info`.

3. **Definice socketů:**
   - Vytvoří TCP, UDP sockety pomocí funkcí `socket()`.
   - Vytvoří se i třetí socket `bind_socket` pro změnu portu.
   - Inicializuje se a nastaví adresu serveru pro komunikaci pro UDP a TCP.
   - Pro `bind_socket` se nastaví jiný server s jiným dostupným portem.
   - Sockety se navážou na příslušné adresy.
   - Při TCP se vyskytuje funkce `listen()`, která naslouchá příchozím spojením.

4. **Asynchronní přístup:**
   - Inicializuje se pole struktur `pollfd` pro asynchronní I/O operace pomocí funkce `poll()`.
   - Každá struktura reprezentuje jeden socket, který bude monitorován.

#### Popis funkce `udp_client()` v `main()`:

1. **Inicializace proměnných:**
   - Inicializuje potřebné proměnné pro zpracování příchozí zprávy.

2. **Přijetí zprávy:**
   - Pokud je klient nový, přijme zprávu na původním UDP socketu.
   - Pokud klient již existuje, přijme zprávu na novém bind socketu.
   - Zpráva obsahuje identifikátor klienta, který je uložen do struktury `msg`.

3. **Zpracování klienta:**
   - Zjišťuje, zda je klient již registrován.
   - Pokud neexistuje, přidá nového klienta do globálního pole.
   - Pokud klient existuje, načte informace o klientovi.

4. **Rozhodování podle stavu klienta:**
   - Na základě stavu klienta zpracovává přijatou zprávu.
   - Existují 2 stavy: `S_START`, `S_OPEN`.

5. **Zpracování zpráv:**
   - `S_START`: zpracování zprávy AUTH, odeslání potvrzení nebo chybové zprávy, zpracování informací o klientovi, odeslání odpovědi nebo potvrzení a v případě chyby ukončení spojení.
   - `S_OPEN`: zpracování zpráv MSG, JOIN, ERR a BYE, odeslání potvrzení, zpracování informací o zprávě a odeslání zpráv.

