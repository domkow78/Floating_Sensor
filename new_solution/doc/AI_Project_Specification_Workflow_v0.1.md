
# AI Project Specification Workflow
## Wersja 0.1 (Draft)

### Cel dokumentu

Dokument definiuje standard prowadzenia nowych projektów z wykorzystaniem AI.
Jego celem jest doprowadzenie od pomysłu do kompletnej, zatwierdzonej specyfikacji
technicznej przed rozpoczęciem implementacji.

Nie jest to metodyka zarządzania projektem ani framework programistyczny.
Jest to opis procesu wspólnego opracowywania projektu przez człowieka i AI.

---

# 1. Założenia

Proces opiera się na kilku podstawowych zasadach:

- najpierw zrozumienie problemu,
- następnie architektura,
- później specyfikacja,
- implementacja dopiero po zakończeniu projektowania.

Kod nie jest pierwszym rezultatem pracy.
Pierwszym rezultatem jest spójna dokumentacja.

---

# 2. Role

## Użytkownik

Odpowiada za:

- wizję projektu,
- wymagania,
- decyzje biznesowe,
- zatwierdzanie kolejnych etapów.

## AI

Odpowiada za:

- zadawanie pytań,
- porządkowanie informacji,
- proponowanie architektury,
- wskazywanie ryzyk,
- przygotowanie dokumentacji,
- pilnowanie spójności całego projektu.

AI nie podejmuje decyzji za użytkownika.
Każda istotna decyzja wymaga akceptacji.

---

# 3. Zasady współpracy

1. Jeden temat = jedna dyskusja.
2. Każdy rozdział kończy się dokumentem.
3. Dokument podlega przeglądowi.
4. Użytkownik zatwierdza lub odrzuca dokument.
5. Dopiero po zatwierdzeniu rozpoczyna się następny etap.

Nie pomijamy etapów.

---

# 4. Cykl pracy

Pomysł

↓

Analiza problemu

↓

Założenia

↓

Projekt architektury

↓

Specyfikacja techniczna

↓

Przegląd

↓

Akceptacja

↓

Następny rozdział

Po ukończeniu wszystkich rozdziałów:

↓

Development Guide

↓

Backlog implementacyjny

↓

Implementacja

↓

Testy

↓

Rozwój projektu

---

# 5. Struktura dokumentacji

Zalecana kolejność:

1. Wprowadzenie
2. Architektura systemu
3. Komponent referencyjny / urządzenie
4. Firmware lub Backend
5. Komunikacja
6. Logika biznesowa
7. API
8. Magazyn danych
9. Interfejs użytkownika
10. Wdrożenie
11. Struktura repozytorium
12. Plan implementacji
13. Development Guide
14. Backlog

Kolejność może zostać dostosowana do charakteru projektu.

---

# 6. Zawartość każdego rozdziału

Każdy dokument powinien odpowiadać na pytania:

- Za co odpowiada dany element?
- Jak komunikuje się z pozostałymi?
- Jakie posiada granice odpowiedzialności?
- Jakie decyzje architektoniczne zostały podjęte?
- Jakie są kierunki dalszego rozwoju?

---

# 7. Proces akceptacji

Po przygotowaniu każdego rozdziału:

- użytkownik dokonuje przeglądu,
- zgłasza uwagi,
- AI aktualizuje dokument,
- użytkownik zatwierdza dokument.

Akceptacja zamyka etap.

---

# 8. Moment rozpoczęcia implementacji

Implementacja rozpoczyna się dopiero wtedy, gdy:

- architektura jest kompletna,
- dokumentacja została zatwierdzona,
- przygotowano Development Guide,
- istnieje plan implementacji,
- określono Definition of Done.

---

# 9. Zasady jakości

Podczas całego procesu obowiązują:

- spójność dokumentacji,
- iteracyjne dopracowywanie projektu,
- unikanie przedwczesnej implementacji,
- możliwość śledzenia decyzji architektonicznych,
- zgodność kodu z dokumentacją.

---

# 10. Rezultat końcowy

Zakończeniem etapu projektowego powinien być kompletny pakiet obejmujący:

- Specyfikację architektury,
- Development Guide,
- Plan implementacji,
- Backlog,
- standardy kodowania.

Dopiero taki zestaw stanowi punkt startowy do implementacji.

---

# 11. Filozofia

Najważniejszą zasadą tej metody jest:

**Dokumentacja nie powstaje po napisaniu kodu. Dokumentacja prowadzi implementację.**

Architektura jest żywym dokumentem, rozwijanym iteracyjnie wraz z rosnącym zrozumieniem projektu.

Każdy zatwierdzony rozdział zwiększa dojrzałość projektu i zmniejsza ryzyko zmian podczas implementacji.
