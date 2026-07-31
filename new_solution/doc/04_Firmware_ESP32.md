
# Rozdział 4 – Architektura Firmware ESP32
**IoT Platform Architecture Specification v1.0**

---

# 4.1 Cel rozdziału

Celem firmware jest zapewnienie stabilnej, przewidywalnej i łatwej w rozbudowie pracy urządzenia Floating Sensor.

Firmware odpowiada wyłącznie za obsługę sprzętu, komunikację oraz przygotowanie danych pomiarowych do publikacji. Nie realizuje logiki biznesowej ani analizy danych.

---

# 4.2 Założenia projektowe

Firmware powinien:

- pracować w sposób ciągły,
- być modułowy,
- umożliwiać łatwe dodawanie nowych czujników,
- być odporny na chwilowe błędy komunikacji,
- minimalizować zależności pomiędzy modułami.

---

# 4.3 Architektura logiczna

```text
Application
│
├── Configuration
├── System Manager
├── Sensor Manager
│   ├── BME688 Driver
│   └── LSM6DSV Driver
├── WiFi Manager
├── MQTT Client
├── Diagnostics
└── Logger
```

Każdy moduł posiada jasno określoną odpowiedzialność i komunikuje się z pozostałymi poprzez zdefiniowane interfejsy.

---

# 4.4 Odpowiedzialność modułów

| Moduł | Odpowiedzialność |
|-------|-------------------|
| Application | Główna pętla programu i inicjalizacja |
| Configuration | Parametry konfiguracyjne |
| System Manager | Zarządzanie stanem urządzenia |
| Sensor Manager | Odczyt wszystkich czujników |
| WiFi Manager | Połączenie z siecią WiFi |
| MQTT Client | Publikacja danych |
| Diagnostics | Informacje diagnostyczne |
| Logger | Rejestrowanie zdarzeń systemowych |

---

# 4.5 Sekwencja uruchamiania

1. Inicjalizacja sprzętu.
2. Wczytanie konfiguracji.
3. Inicjalizacja magistrali I²C.
4. Inicjalizacja czujników.
5. Uruchomienie WiFi.
6. Połączenie z brokerem MQTT.
7. Przejście do głównej pętli programu.

Każdy etap powinien zwracać jednoznaczną informację o powodzeniu lub błędzie.

---

# 4.6 Główna pętla

```text
while (running)
    ├─ odczyt czujników
    ├─ walidacja danych
    ├─ przygotowanie komunikatu
    ├─ publikacja MQTT
    ├─ diagnostyka
    └─ oczekiwanie do następnego cyklu
```

Pętla nie powinna zawierać kodu specyficznego dla pojedynczych czujników – za to odpowiada Sensor Manager.

---

# 4.7 Obsługa błędów

Firmware powinien wykrywać i raportować m.in.:

- brak odpowiedzi czujnika,
- utratę połączenia WiFi,
- utratę połączenia MQTT,
- błędy odczytu danych.

W przypadku błędów komunikacyjnych system powinien podejmować automatyczne próby ponownego połączenia bez restartu urządzenia.

---

# 4.8 Zarządzanie konfiguracją

Wszystkie parametry konfiguracyjne powinny być zgromadzone w jednym module.

Przykłady:

- identyfikator urządzenia,
- dane sieci WiFi,
- adres brokera MQTT,
- interwał pomiarowy,
- poziom logowania.

---

# 4.9 Zasady implementacyjne

- Moduły nie powinny znać swojej wewnętrznej implementacji.
- Sterowniki czujników nie komunikują się bezpośrednio z MQTT.
- MQTT Client nie odczytuje danych z czujników.
- Logger nie wpływa na logikę programu.
- Firmware pozostaje niezależny od IoT Core.

---

# 4.10 Przygotowanie do rozwoju

Architektura umożliwia w przyszłości dodanie:

- nowych czujników,
- Deep Sleep,
- OTA,
- Bluetooth,
- lokalnego buforowania danych.

Elementy te nie są implementowane w wersji MVP, ale obecny podział na moduły pozwala na ich późniejsze wprowadzenie bez przebudowy całego firmware.
