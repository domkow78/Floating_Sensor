
# Development Guide
**IoT Platform – Development Guide v1.0**

---

# 1. Cel

Development Guide definiuje standardy obowiązujące podczas implementacji platformy IoT.

Dokument uzupełnia specyfikację architektoniczną i opisuje sposób tworzenia,
testowania oraz utrzymywania kodu.

---

# 2. Zasady ogólne

- Architektura jest nadrzędna wobec implementacji.
- Kod ma być prosty i czytelny.
- Preferowane są małe, odpowiedzialne moduły.
- Każda zmiana powinna być możliwa do przetestowania.
- Dokumentacja rozwijana jest równolegle z kodem.

---

# 3. Standardy kodowania

## Python

- PEP 8
- Type hints
- Docstring dla modułów, klas i metod publicznych
- Ruff
- Black
- pytest
- mypy (stopniowo)

## C++ / PlatformIO

- C++17
- Jeden moduł = jedna odpowiedzialność
- Brak globalnych zależności jeśli nie są konieczne
- Czytelne interfejsy
- Pliki .h definiują kontrakt, .cpp implementację

---

# 4. Nazewnictwo

## Python

- pliki: snake_case.py
- klasy: PascalCase
- funkcje: snake_case()
- stałe: UPPER_CASE

## C++

- klasy: PascalCase
- metody: camelCase()
- stałe: UPPER_CASE

---

# 5. Git

Model pracy:

- main
- feature/*
- fix/*
- docs/*

Commit powinien opisywać jedną zmianę.

Przykłady:

- feat: add mqtt client
- fix: reconnect after wifi loss
- docs: update firmware architecture

---

# 6. Testy

Każdy komponent powinien posiadać:

- testy jednostkowe,
- testy integracyjne (jeżeli dotyczą komunikacji),
- możliwość uruchomienia niezależnie.

Nie akceptujemy zmian bez możliwości ich weryfikacji.

---

# 7. Dokumentacja

Każda większa zmiana powinna obejmować:

- aktualizację dokumentacji,
- aktualizację README,
- opis zmian.

Kod i dokumentacja pozostają spójne.

---

# 8. Struktura Pull Request

PR powinien zawierać:

- cel zmiany,
- zakres,
- sposób testowania,
- wpływ na architekturę,
- odwołanie do rozdziału dokumentacji (jeśli dotyczy).

---

# 9. Checklist przed scaleniem

- Kod kompiluje się.
- Testy zakończone sukcesem.
- Brak ostrzeżeń lint.
- Dokumentacja zaktualizowana.
- Konfiguracja nie zawiera danych wrażliwych.
- Zmiana jest zgodna z architekturą.

---

# 10. Narzędzia

## Firmware

- Visual Studio Code
- PlatformIO

## Backend

- Python
- Docker Compose
- pytest
- Ruff
- Black

---

# 11. Zasada architektoniczna

Podczas implementacji obowiązuje zasada:

**Najpierw architektura, potem kod.**

Jeżeli implementacja wymaga zmiany architektury, najpierw aktualizowana jest dokumentacja, a dopiero później kod.

---

# 12. Definition of Ready

Zadanie może zostać rozpoczęte, gdy:

- istnieje opis architektoniczny,
- zdefiniowano interfejsy,
- znane są kryteria zakończenia.

---

# 13. Definition of Done

Zadanie uznaje się za zakończone, gdy:

- kod działa zgodnie z wymaganiami,
- testy zakończyły się powodzeniem,
- dokumentacja została zaktualizowana,
- kod spełnia standardy projektu,
- zmiana została zintegrowana z repozytorium.

---

# 14. Filozofia projektu

Projekt rozwijany jest iteracyjnie.

Każda kolejna funkcjonalność wynika z potrzeb działającego systemu.

Unikamy implementowania funkcji „na zapas”.

Najważniejszym celem jest utrzymanie spójności pomiędzy architekturą, dokumentacją i kodem.
