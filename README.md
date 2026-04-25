# tracker-care-of

Приложение-трекер для ухода за пациентами(растениями) с возможностью регистрации и авторизации пользователей, управлением пациентами(растениями) и ведением их анамнеза. Данные хранятся в PostgreSQL.

API-сервер построен на C++20 с использованием фреймворка [userver 3.0](https://userver.tech). CLI-приложение собирается отдельно через CMake + vcpkg.

## Требования

- Docker
- Docker Compose

## 🚀 Быстрый старт

### 1. Настройка переменных окружения

Скопируйте `.env.example` в `.env` и настройте параметры.

### 2. Запуск приложения (без CLI)

```bash
docker compose up --build -d
```

Это автоматически:
- Запустит PostgreSQL контейнер
- Запустит Redis контейнер для хранения сессий
- Применит все миграции из [libs/db/migrations/](libs/db/migrations/)
- Соберёт и запустит API-сервер на userver

### 3. Сервисы

- **API**: http://localhost:8080
- **CLI**: локальная сборка и запуск (см. раздел "Запуск CLI")

## Конфигурация API

API-сервер конфигурируется тремя файлами:

| Файл | Назначение |
|---|---|
| [apps/api/static_config.yaml](apps/api/static_config.yaml) | Структура компонентов userver (HTTP-сервер, postgres, redis, хэндлеры) |
| [apps/api/dynamic_config_vars.yaml](apps/api/dynamic_config_vars.yaml) | Runtime-переменные userver |
| [apps/api/secdist.json](apps/api/secdist.json) | Секреты Redis (host/port sentinel-ов, пароль) |

Все три файла монтируются в контейнер как volumes — правки применяются через `docker compose restart api-app` без пересборки. Подключение к PostgreSQL читается из переменной окружения `DATABASE_URL`, которая собирается в [docker-compose.yaml](docker-compose.yaml) из `DB_USER`/`DB_PASSWORD`/`DB_NAME`.

## Запуск CLI

CLI приложение требует локальной сборки с помощью CMake. userver для CLI не нужен — он зависит только от CPR и устанавливается через vcpkg.

### Требования для сборки

- CMake 3.25+
- vcpkg (для управления зависимостями)
- Компилятор с поддержкой C++20

### Сборка и запуск

1. **Настройте vcpkg** (если еще не настроен):
   ```powershell
   git clone https://github.com/microsoft/vcpkg
   .\vcpkg\bootstrap-vcpkg.bat
   ```

2. **Настройте переменную окружения** `VCPKG_ROOT`:
   ```powershell
   $env:VCPKG_ROOT = "C:\path\to\vcpkg"
   ```

3. **Создайте директорию для сборки**:
   ```powershell
   mkdir build
   cd build
   ```

4. **Сконфигурируйте проект**:
   ```powershell
   cmake .. -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
   ```

5. **Соберите CLI**:
   ```powershell
   cmake --build . --config Release --target tracker_cli
   ```

6. **Запустите CLI**:
   ```powershell
   .\apps\cli\Release\tracker_cli.exe
   ```

## Возможности

- Регистрация и авторизация пользователей (email/пароль с хешированием Argon2)
- Управление списком пациентов
- Ведение анамнеза для каждого пациента
- Отслеживание частоты полива (для растений)
- Интерактивный консольный интерфейс (CLI)
- HTTP API на асинхронном фреймворке userver

## Технологии

- Язык: C++20
- Система сборки: CMake (версия 3.25+)
- HTTP-фреймворк API: [userver 3.0](https://userver.tech) (FetchContent)
- База данных: PostgreSQL через `userver::storages::postgres` (асинхронный пул соединений)
- Хранилище сессий: Redis через `userver::storages::redis`
- Хеширование паролей: Argon2
- HTTP клиент (CLI): cpr
- JSON: nlohmann/json
- Форматирование: fmt
- Менеджер пакетов: vcpkg (для CLI и нескольких библиотек API)

## Структура проекта

```
tracker-care-of/
├── apps/                                    # Приложения
│   ├── api/                                 # API-сервер на userver
│   │   ├── src/
│   │   │   ├── main.cpp                    # userver::utils::DaemonMain
│   │   │   ├── component-list.cpp          # Регистрация компонентов
│   │   │   ├── components/                 # SessionStoreComponent
│   │   │   └── handlers/                   # HTTP-хэндлеры (наследники HttpHandlerBase)
│   │   ├── include/
│   │   │   ├── component-list.hpp
│   │   │   ├── components/
│   │   │   └── handlers/                   # AuthenticatedHandlerBase + http-helpers
│   │   ├── dto/                            # Data Transfer Objects
│   │   ├── static_config.yaml              # Конфигурация компонентов userver
│   │   ├── dynamic_config_vars.yaml        # Runtime-переменные
│   │   └── secdist.json                    # Секреты Redis
│   └── cli/                                 # Консольное приложение
│       ├── include/
│       └── src/
├── libs/                                    # Библиотеки
│   ├── common/                              # Общие утилиты (env-parser)
│   ├── crypto/                              # Argon2-хэширование паролей
│   ├── core/                                # Бизнес-модели (Patient, User, Water, Anamnesis)
│   ├── db/                                  # Работа с PostgreSQL через userver
│   │   ├── include/tracker_db/
│   │   │   ├── repositories/               # Репозитории (принимают ClusterPtr)
│   │   │   └── usecases/                   # AuthService
│   │   ├── src/
│   │   └── migrations/                     # SQL-миграции
│   └── session/                             # Управление сессиями
│       ├── include/tracker_session/
│       │   ├── session-store.h             # Интерфейс хранилища
│       │   └── redis-session-store.h       # Реализация на userver::storages::redis
│       └── src/
├── CMakeLists.txt                           # FetchContent userver + vcpkg для CLI
├── Dockerfile                               # Многоступенчатая сборка userver
├── docker-compose.yaml
└── README.md
```

## Использование CLI

После запуска CLI приложения:

### Главное меню

- `1) Register` — регистрация нового пользователя
- `2) Login` — вход в систему

### Навигация в интерфейсе

- **Перемещение по меню**: клавиши со стрелками или `W` / `S`
- **Выбор пункта**: `Enter` или `Space`
- **Добавить элемент**: `+`
- **Удалить элемент**: `Del` (требуется подтверждение `Y`)
- **Назад/Выход**: `Esc`

## Архитектура

Проект организован по модульному принципу:

- **tracker_common** — общие утилиты (парсер env-файлов)
- **tracker_crypto** — криптографические функции (хеширование паролей)
- **tracker_core** — модели данных (Patient, User, Water, Anamnesis)
- **tracker_db** — репозитории и AuthService поверх `userver::storages::postgres::ClusterPtr`
- **tracker_session** — `RedisSessionStore` поверх `userver::storages::redis::Client`
- **tracker_cli** — консольное приложение (vcpkg, без userver)
- **tracker_api** — HTTP API на userver (использует `tracker_db` и `tracker_session`)

### HTTP-хэндлеры

Каждый маршрут — отдельный компонент userver, наследующий `HttpHandlerBase`. Защищённые маршруты наследуются от `AuthenticatedHandlerBase`, который читает session-cookie и резолвит `user_uuid` через `SessionStore`.

| Метод(ы) | Путь | Хэндлер |
|---|---|---|
| GET / POST | `/api/patients` | `PatientListHandler` |
| GET / PATCH / DELETE | `/api/patients/{id}` | `PatientByIdHandler` |
| POST | `/api/auth/{action}` (`register`/`login`/`logout`) | `AuthHandler` |
| GET / POST | `/api/anamnesis` | `AnamnesisListHandler` |
| GET / PATCH / DELETE | `/api/anamnesis/{id}` | `AnamnesisByIdHandler` |
| GET / POST | `/api/water` | `WaterListHandler` |
| GET / DELETE | `/api/water/{id}` | `WaterByIdHandler` |

### Аутентификация

API использует **cookie-based аутентификацию** с HTTP-only cookies (`session_uuid`).

Сессии хранятся в **Redis** с автоматическим TTL. Доступ к Redis-клиенту инкапсулирован в `SessionStoreComponent`, который инжектится в хэндлеры через `ComponentContext`. Подключение к Redis настраивается через `secdist.json` (sentinel-хосты, пароль) и блок `redis-sentinel` в `static_config.yaml` (`groups`/`thread_pools`).

CLI приложение автоматически управляет сессиями при взаимодействии с API через `API_URL` из `.env`.

## Лицензия

Проект распространяется по лицензии **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)**. Подробности см. в файле `LICENSE`.
