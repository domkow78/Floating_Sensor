# Deployment

Ten dokument jest przeznaczony do szybkiego uruchamiania lokalnego środowiska deweloperskiego dla nowego MVP.

Aktualny stan projektu jest zgodny z nową architekturą opisanej w dokumentacji:

- [new_solution/doc/10_Docker_Deployment_Architecture.md](new_solution/doc/10_Docker_Deployment_Architecture.md)
- [new_solution/doc/11_Struktura_Repozytorium.md](new_solution/doc/11_Struktura_Repozytorium.md)
- [new_solution/doc/12_Plan_Implementacji.md](new_solution/doc/12_Plan_Implementacji.md)
- [new_solution/doc/13_Migracja_i_Kontrakty_MVP_v0.1.md](new_solution/doc/13_Migracja_i_Kontrakty_MVP_v0.1.md)

> Wartość historyczna: ten plik jest zbiorem starych instrukcji uruchamiania. Aktualna architektura jest rozwijana w katalogu [new_solution/src](new_solution/src), a poprzedni kod został przeniesiony do [new_solution/src_ref](new_solution/src_ref) jako materiał referencyjny.

---

## Cel dokumentu

- uruchamiać lokalnie środowisko testowe MVP,
- wspierać iteracyjny rozwój bez zależności od starego prototypu,
- nie zastępować formalnej dokumentacji architektonicznej.

---

## Obecna struktura projektu

```text
new_solution/
├── src/         # aktywna implementacja MVP
├── src_ref/     # stary prototyp referencyjny
├── doc/         # dokumentacja źródłowa
├── pcb/
└── ...
```

---

## Wymagania lokalne

- Python 3.11+
- venv
- `pip`
- opcjonalnie: Docker do uruchamiania Mosquitto / InfluxDB

---

## Uruchomienie środowiska Python

Przejdź do katalogu źródłowego:

```powershell
cd "C:\Programs\WorkDirDev\## Git Hub\Floating_Sensor\new_solution\src"
```

Utwórz i aktywuj venv:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
```

Zainstaluj zależności:

```powershell
python -m pip install --upgrade pip
python -m pip install -r requirements.txt --index-url https://pypi.org/simple
```

Uruchom smoke test:

```powershell
python -m pytest tests/test_smoke.py -q
```

---

## Uruchomienie aplikacji

W tej chwili aktywny kod jest zbudowany w iteracyjny sposób, a podstawowy punkt wejścia znajduje się w:

- [new_solution/src/app/main.py](new_solution/src/app/main.py)

Uruchomienie:

```powershell
python app/main.py
```

Dalsze moduły, takie jak MQTT, processing i storage, dodawane są w miarę rozwoju etapu MVP.

---

## Wersja z Docker

Dla pełnego środowiska testowego można użyć konfiguracji opisanej w dokumentacji wdrożeniowej Docker: [new_solution/doc/10_Docker_Deployment_Architecture.md](new_solution/doc/10_Docker_Deployment_Architecture.md).

W aktualnym etapie główny nacisk kładzie się na działające, testowalne etapy w Pythonie, a nie na pełne wdrożenie kontenerowe.

---

## Relewantne zasady

- nie rozwijać prototypu w miejscu aktywnej implementacji,
- nie mieszać legacy z nowym kodem,
- trzymać dokumentację i kod w zgodności z architekturą MVP,
- dodawać katalogi dopiero wtedy, gdy pojawia się realna potrzeba.

---

## Podsumowanie

Ten plik nie jest już głównym źródłem wiedzy o wdrożeniu projektu. Dokumentacja w [new_solution/doc](new_solution/doc) oraz aktywny kod w [new_solution/src](new_solution/src) są aktualnym źródłem prawdy.
