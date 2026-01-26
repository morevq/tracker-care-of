# tracker-care-of

Приложение-трекер для ухода за пациентами с возможностью регистрации и авторизации пользователей, управлением пациентами и ведением их анамнеза. Данные хранятся в PostgreSQL.

Проект построен на C++20 с использованием CMake и vcpkg для управления зависимостями.

## Возможности

- Регистрация и авторизация пользователей (email/пароль с хешированием Argon2)
- Управление списком пациентов
- Ведение анамнеза для каждого пациента
- Отслеживание частоты полива (для растений)
- Интерактивный консольный интерфейс (CLI)
- API сервер (в разработке)

## Технологии

- Язык: C++20
- Система сборки: CMake (версия 3.25+)
- База данных: PostgreSQL (libpq)
- Хеширование паролей: Argon2 (unofficial-argon2)
- Форматирование: fmt
- Менеджер пакетов: vcpkg

## Структура проекта

```
tracker-care-of/
├── apps/                           # Приложения
│   ├── api/                        # API сервер
│   │   └── src/main.cpp
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

## Требования

- CMake 3.25+
- Компилятор с поддержкой C++20 (MSVC, GCC 10+, Clang 12+)
- vcpkg
- PostgreSQL (сервер) с установленными библиотеками libpq
- Переменная окружения `VCPKG_ROOT`, указывающая на корень vcpkg

## Установка зависимостей (vcpkg)

Установите необходимые пакеты:

```powershell
vcpkg install fmt unofficial-argon2 postgresql
```

Для конкретной платформы:

```powershell
vcpkg install fmt unofficial-argon2 postgresql --triplet x64-windows
```

## Настройка базы данных

1. Установите и запустите PostgreSQL
2. Создайте базу данных (например, `tracker_care_of`)
3. Создайте необходимые таблицы (см. схему БД)
4. Настройте переменные окружения для подключения (файл `.env` или переменные системы)

## Сборка проекта

### Windows (Visual Studio)

```powershell
# Установите VCPKG_ROOT
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# Генерация проекта
cmake -S . -B out/build/x64-Debug -G "Visual Studio 17 2022" -A x64

# Сборка
cmake --build out/build/x64-Debug
```

Или откройте папку проекта в Visual Studio 2022 (откроется как CMake проект).

### Windows (Ninja)

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

### Linux/macOS

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake -S . -B build -G Ninja
cmake --build build
```

## Запуск приложений

### Вариант 1: Автоматический запуск (рекомендуется)

Используйте PowerShell скрипт для последовательного запуска API и CLI:

```powershell
.\start-apps.ps1
```

Скрипт запустит:
1. **API Server** - сервер приложения
2. **CLI Client** (через 2 секунды) - консольный интерфейс

### Вариант 2: Запуск из Visual Studio

1. На панели инструментов найдите выпадающий список рядом с зеленой кнопкой запуска
2. Выберите **"API Server"** и нажмите **F5**
3. Выберите **"CLI Client"** и нажмите **Ctrl+F5**

### Вариант 3: Ручной запуск

```powershell
# Запустите API
.\out\build\x64-Debug\tracker_api.exe

# В другом окне запустите CLI
.\out\build\x64-Debug\tracker_cli.exe
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
- **tracker_api** - API сервер (в разработке)

### Зависимости между модулями

```
tracker_cli --> tracker_db --> tracker_core --> tracker_common
                   |             
                   v
            tracker_crypto --> tracker_common
```

## Разработка

### Добавление новых функций

1. Модели данных -> `libs/core/include/tracker/models/`
2. Репозитории -> `libs/db/include/tracker_db/repositories/`
3. UI компоненты -> `apps/cli/include/`

### Соглашения о коде

- Используйте существующие библиотеки (fmt для форматирования)
- Следуйте структуре include путей проекта
- Модели и бизнес-логика в `tracker_core`
- Работа с БД в `tracker_db`

## Лицензия

Проект распространяется по лицензии **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)**. Подробности см. в файле `LICENSE`.

## Авторы

(Добавьте информацию об авторах)
