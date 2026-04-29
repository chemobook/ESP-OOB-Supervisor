<div align="center">

# 🛰️ ESP-OOB-Supervisor

**Контроллер для удаленного обслуживания узлов LoRa / Meshtastic.**

[![Status: Early Development](https://img.shields.io/badge/Статус-Ранняя_разработка-orange.svg)](#)
[![License: MIT](https://img.shields.io/badge/Лицензия-MIT-blue.svg)](LICENSE)

</div>

Супервизор ESP32 находится в глубоком сне. При получении валидного **LoRa-пакета** он просыпается, подключается к Wi-Fi (или создает свою точку доступа) и открывает Web UI для удаленной прошивки, сброса и настройки целевого узла.

---

## 🔄 Принцип работы

```mermaid
flowchart TD
    A([Получен LoRa-пакет пробуждения]) --> B[ESP32 просыпается]
    B --> C{Есть знакомая Wi-Fi сеть?}
    C -- Да --> D[Подключение к Wi-Fi]
    C -- Нет --> E[Запуск своей точки доступа]
    D --> F[Web UI доступен]
    E --> F
    F --> G[Обслуживание целевого узла]
    G --> H([Возврат в глубокий сон])
    
    style A fill:#4CAF50,stroke:#388E3C,color:white
    style H fill:#2196F3,stroke:#1976D2,color:white
```

---

## 🧩 Аппаратная архитектура

```mermaid
graph LR
    subgraph Канал обслуживания
        A[Сервисный модуль LoRa] -- Wake IRQ --> B[Супервизор ESP32]
    end
    
    subgraph Целевое устройство
        C[Целевой узел LoRa]
    end
    
    B -- UART TX/RX --> C
    B -- RESET / EN --> C
    B -- BOOT --> C
    B -- ПИТАНИЕ / GND --> C
```

**Обязательное подключение пинов:**
* `ESP32 TX` ➔ `Target RX`
* `ESP32 RX` ➔ `Target TX`
* `ESP32 GPIO` ➔ `Target RESET / EN`
* `ESP32 GPIO` ➔ `Target BOOT`
* `GND` ➔ `GND`

---

**Лицензия:** [MIT](LICENSE)    C -- Нет --> E[Запуск собственной точки доступа]
    D --> F[Web UI становится доступен]
    E --> F
    F --> G[Администратор обслуживает целевой узел]
    G --> H([ESP32 возвращается в глубокий сон])
    
    style A fill:#4CAF50,stroke:#388E3C,color:white
    style H fill:#2196F3,stroke:#1976D2,color:white
