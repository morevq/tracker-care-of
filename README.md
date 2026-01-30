# tracker-care-of

Приложение-трекер для ухода за пациентами с возможностью регистрации и авторизации пользователей, управлением пациентами(растениями) и ведением их анамнеза. Данные хранятся в PostgreSQL.

Проект построен на C++20 с использованием CMake и vcpkg для управления зависимостями.

## Требования

- CMake 3.25+
- Компилятор с поддержкой C++20 (MSVC, GCC 10+, Clang 12+)
- vcpkg
- PostgreSQL (сервер) с установленными библиотеками libpq

## Быстрый старт

### Установка зависимостей (vcpkg)

Установите все необходимые пакеты:

```powershell
vcpkg install
```

### Настройка базы данных

1. Установите и запустите PostgreSQL
2. Создайте базу данных (например, `tracker_care_of`)
3. Создайте необходимые таблицы (см. схему БД)
4. Настройте переменные окружения для подключения (файл `.env`, образец в `.env.example`)

### Сборка проекта

#### Windows (Visual Studio)

```powershell
# Установите VCPKG_ROOT
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# Генерация проекта
cmake -S . -B out/build/x64-Debug -G "Visual Studio 17 2022" -A x64

# Сборка
cmake --build out/build/x64-Debug
```

Или откройте папку проекта в Visual Studio 2022 (откроется как CMake проект).

#### Windows (Ninja)

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

#### Linux/macOS

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake -S . -B build -G Ninja
cmake --build build
```

### Запуск приложений

#### Автоматический запуск

Используйте PowerShell скрипт для последовательного запуска API и CLI:

```powershell
.\start-apps.ps1
```

</br>

## Возможности

- Регистрация и авторизация пользователей (email/пароль с хешированием Argon2)
- Управление списком пациентов
- Ведение анамнеза для каждого пациента
- Отслеживание частоты полива (для растений)
- Интерактивный консольный интерфейс (CLI)
- API сервер

## Технологии

- Язык: C++20
- Система сборки: CMake (версия 3.25+)
- База данных: PostgreSQL (libpq)
- Хеширование паролей: Argon2 (unofficial-argon2)
- Форматирование: fmt
- HTTP клиент: cpr
- JSON: nlohmann/json
- Менеджер пакетов: vcpkg

## Структура проекта

```
tracker-care-of/
├── apps/                           # Приложения
│   ├── api/                        # API сервер
│   │   ├── src/
│   │   └── dto/
│   └── cli/                        # Консольное приложение
│       ├── include/                # Заголовочные файлы UI
│       └── src/                    # Исходники UI
├── libs/                           # Библиотеки
│   ├── common/                     # Общие утилиты
│   │   ├── include/tracker_common/
│   │   └── src/
│   ├── crypto/                     # Криптография (Argon2)
│   │   ├── include/tracker_crypto/
│   │   └── src/
│   ├── core/                       # Бизнес-логика и модели
│   │   ├── include/tracker/
│   │   │   ├── models/            # Модели данных
│   │   │   └── usecases/          # Сервисы (auth-service)
│   │   └── src/
│   └── db/                         # Работа с БД
│       ├── include/tracker_db/
│       │   ├── repositories/      # Репозитории
│       │   └── postgres-db.h
│       └── src/
├── CMakeLists.txt                  # Конфигурация сборки
├── start-apps.ps1                  # Скрипт запуска приложений
└── README.md
```

## Использование CLI

После запуска CLI приложения:

### Главное меню
- `1) Register` - регистрация нового пользователя
- `2) Login` - вход в систему

### Навигация в интерфейсе
- **Перемещение по меню**: клавиши со стрелками или `W` / `S`
- **Выбор пункта**: `Enter` или `Space`
- **Добавить элемент**: `A` или `+`
- **Назад/Выход**: `Esc`

## Архитектура

Проект организован по модульному принципу:

- **tracker_common** - общие утилиты (парсер env-файлов)
- **tracker_crypto** - криптографические функции (хеширование паролей)
- **tracker_core** - модели данных (Patient, User, Water, Anamnesis)
- **tracker_db** - работа с PostgreSQL, репозитории, auth-service
- **tracker_cli** - консольное приложение
- **tracker_api** - HTTP API сервер (использует `tracker_db`)

## API

CLI взаимодействует с сервером через HTTP (cookie-сессия). В проекте используются DTO на базе `nlohmann::json`.

По умолчанию API сервер стартует на порту `API_PORT` из `.env` (если не задан — `8080`).

CLI подключается к API по `API_URL` из `.env`.


Типовые сценарии:

- Авторизация: register / login / logout
- Пациенты: список, получение по id, создание
- Анамнез: список по пациенту, создание
- Полив: получение данных по поливу

### Зависимости между модулями

```
tracker_cli --> tracker_db --> tracker_core --> tracker_common
                   |             
                   v
            tracker_crypto --> tracker_common
```

## Лицензия

Проект распространяется по лицензии **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)**. Подробности см. в файле `LICENSE`.
