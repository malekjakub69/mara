# Dokumentace Backend API pro Histogram Viewer

## Obsah

1. [Přehled](#1-přehled)
2. [Specifikace API](#2-specifikace-api)
    - [Základní URL](#základní-url)
    - [Koncové body API](#koncové-body-api)
3. [Implementační detaily](#3-implementační-detaily)
    - [Datový model](#datový-model)
    - [Logika generování dat](#logika-generování-dat)
4. [Příklad implementace](#4-příklad-implementace)
5. [Poznámky k nasazení](#5-poznámky-k-nasazení)

## 1. Přehled

Backend musí poskytovat REST API pro generování a správu histogramových dat. Frontend komunikuje s backendem pomocí HTTP požadavků na několik koncových bodů. Backend by měl být schopen generovat náhodná data pro histogram, konfigurovat parametry histogramu a řídit proces generování dat.

## 2. Specifikace API

### Základní URL

```
http://127.0.0.1:8000
```

### Koncové body API

#### 1. Získání aktuálních dat histogramu

-   **Endpoint:** `/`
-   **Metoda:** `GET`
-   **Popis:** Vrací aktuální data histogramu.
-   **Odpověď:** JSON pole dvojic `[hodnota, počet]`
-   **Příklad odpovědi:**
    ```json
    [
        [0, 5],
        [1, 12],
        [2, 8],
        [3, 15],
        [4, 6],
        [5, 3]
    ]
    ```

#### 2. Konfigurace histogramu

-   **Endpoint:** `/config`
-   **Metoda:** `POST`
-   **Popis:** Nastavuje parametry pro generování histogramu.
-   **Požadavek:** JSON objekt s parametry
    ```json
    {
        "start_value": 0,
        "stop_value": 10,
        "step_value": 1
    }
    ```
-   **Odpověď:** JSON objekt potvrzující nastavené parametry
    ```json
    {
        "start_value": 0,
        "stop_value": 10,
        "step_value": 1
    }
    ```

#### 3. Spuštění generování dat

-   **Endpoint:** `/start`
-   **Metoda:** `POST`
-   **Popis:** Spouští proces generování dat s danou periodou aktualizace.
-   **Požadavek:** JSON objekt s periodou aktualizace
    ```json
    {
        "count_period": 5
    }
    ```
-   **Odpověď:** JSON objekt potvrzující nastavení
    ```json
    {
        "count_period": 5,
        "status": "started"
    }
    ```

#### 4. Zastavení generování dat

-   **Endpoint:** `/stop`
-   **Metoda:** `POST`
-   **Popis:** Zastavuje proces generování dat.
-   **Požadavek:** Prázdný
-   **Odpověď:** JSON objekt potvrzující zastavení
    ```json
    {
        "status": "stopped"
    }
    ```

#### 5. Vyčištění dat

-   **Endpoint:** `/clear`
-   **Metoda:** `POST`
-   **Popis:** Vymaže všechna aktuální data histogramu.
-   **Požadavek:** Prázdný
-   **Odpověď:** JSON objekt potvrzující vyčištění
    ```json
    {
        "status": "cleared"
    }
    ```

## 3. Implementační detaily

### Datový model

Backend by měl udržovat následující stav:

1. **Konfigurační parametry histogramu:**

    - `start_value`: Počáteční hodnota rozsahu histogramu
    - `stop_value`: Koncová hodnota rozsahu histogramu
    - `step_value`: Krok mezi jednotlivými hodnotami (šířka sloupce)

2. **Parametry generování:**

    - `count_period`: Perioda aktualizace dat v sekundách
    - `is_running`: Příznak, zda generování běží

3. **Data histogramu:**
    - Pole nebo slovník mapující hodnoty na počty
