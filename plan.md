# Миграция tracker-care-of на userver 3.0

## Контекст проекта

**Репозиторий:** `tracker-care-of` — трекер ухода за растениями (API + CLI).

**Текущий стек API:**
- HTTP-фреймворк: Crow (`crow::SimpleApp`, `CROW_ROUTE`, `Blueprint`)
- PostgreSQL: raw `libpq` (`PQexecParams`, `PGconn*`, RAII-обёртки в `db_utils`)
- Redis: `redis-plus-plus` (синхронный API)
- Конфигурация: `std::getenv` в `apps/api/src/config.cpp`
- Сборка: CMake + vcpkg

**Цель:** заменить Crow + libpq + redis-plus-plus на userver 3.0, оставив `apps/cli` и `libs/core` нетронутыми.

**Файлы, которые НЕ меняются:**
- `apps/cli/` — полностью независим
- `libs/core/include/tracker/models/` — чистые struct, без зависимостей
- `libs/core/src/models/patient.cpp` — вспомогательная логика
- `libs/crypto/` — Argon2, userver-аналога нет
- `libs/db/migrations/` — SQL-схема не меняется
- `docker-compose.yaml` — PostgreSQL и Redis-сервисы остаются

---

## Среда сборки: Docker

userver не поддерживает нативную сборку на Windows. Вся сборка ведётся через Docker.

**Базовый образ builder:** `ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest`

### Структура Dockerfile (уже готов)

```
builder stage:
  1. RUN vcpkg install              ← кешируется отдельно
  2. COPY CMakeLists.txt .
     RUN cmake -B build ...         ← тяжёлый слой: скачивает и собирает userver (~30 мин)
                                       кешируется пока не меняется CMakeLists.txt
  3. COPY . .
     RUN cmake --build ...          ← быстрый слой: только наш код
                                       пересобирается при изменении исходников

api stage:
  4. Копирует бинарь + конфиги, запускает tracker_api
```

### Рабочий цикл разработки

```bash
# Первая сборка (долго — собирается userver):
docker build --target builder .

# После изменения исходников (быстро — layers 1 и 2 закешированы):
docker build --target builder .

# Полная сборка с runtime-образом:
docker build .

# Запуск стека:
docker compose up --build
```

### Проверка cmake configure (Stage 0)

```bash
docker build --target builder .
```

Первые два RUN (vcpkg + cmake configure) должны завершиться без ошибок.
`cmake --build` будет падать до завершения Этапов 2–7 — это ожидаемо.

---

## Этап 0 — Настройка сборки (CMakeLists.txt + vcpkg.json) ✅

### Контекст

Текущий `CMakeLists.txt` использует vcpkg для всех зависимостей. userver 3.0 не поддерживает vcpkg — его нужно подключать через `FetchContent`. При этом vcpkg нужно **оставить** для `tracker_cli` (он использует `cpr`).

### Задачи

**0.1 Подключить userver через FetchContent** ✅

В начало `CMakeLists.txt` добавлено после `project(...)`:

```cmake
include(FetchContent)

FetchContent_Declare(
    userver
    GIT_REPOSITORY https://github.com/userver-framework/userver.git
    GIT_TAG        v3.0
)

set(USERVER_FEATURE_POSTGRESQL   ON  CACHE BOOL "" FORCE)
set(USERVER_FEATURE_REDIS        ON  CACHE BOOL "" FORCE)
set(USERVER_FEATURE_GRPC         OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_MONGODB      OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_CLICKHOUSE   OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_PATCH_LIBPQ  OFF CACHE BOOL "" FORCE)
# PATCH_LIBPQ=OFF: vcpkg создаёт CURL::libcurl как imported target, который
# недоступен в try_compile-контексте pq-extra — это вызывает сбой cmake configure.

FetchContent_MakeAvailable(userver)
```

**0.2 Обновить `vcpkg.json`** ✅

Удалены пакеты, которые userver поставляет сам:
```json
{
  "name": "tracker-care-of",
  "dependencies": [
    "fmt",
    "argon2",
    "nlohmann-json",
    "cpr"
  ]
}
```

**0.3 Убрать `find_package` для удалённых зависимостей** ✅

Удалены: `find_package(Crow)`, `find_package(hiredis)`, `find_package(redis++)`, `find_package(PostgreSQL)`.

**0.4 Обновить `tracker_db`** ✅

```cmake
target_link_libraries(tracker_db PUBLIC tracker_core tracker_crypto userver::postgresql)
```

**0.5 Обновить `tracker_session`** ✅

```cmake
target_link_libraries(tracker_session PUBLIC userver::redis)
```

**0.6 Обновить `tracker_api`** ✅

```cmake
add_executable(tracker_api
    apps/api/src/main.cpp
    apps/api/src/component-list.cpp
    apps/api/src/controllers/auth-controller.cpp
    apps/api/src/controllers/patient-controller.cpp
    apps/api/src/controllers/anamnesis-controller.cpp
    apps/api/src/controllers/water-controller.cpp
)
target_link_libraries(tracker_api PRIVATE
    tracker_db tracker_session
    userver::core userver::postgresql userver::redis
    nlohmann_json::nlohmann_json fmt::fmt
)
```

**0.7 Dockerfile с userver базовым образом** ✅

Переписан `Dockerfile`: builder использует `ubuntu-22.04-userver-base`, cmake configure и cmake build — отдельные RUN-слои для кеширования.

### Проверка

```bash
docker build --target builder .
```

cmake configure проходит без ошибок. `cmake --build` падает — ожидаемо (исходники ещё ссылаются на Crow/libpq).

---

## Этап 1 — Конфигурация (static_config.yaml + dynamic_config_vars.yaml) ✅

### Контекст

userver требует два YAML-файла:
- `static_config.yaml` — структура компонентов, адреса, порты
- `dynamic_config_vars.yaml` — переменные, изменяемые в runtime

Текущая конфигурация читается из env-переменных в `apps/api/src/config.cpp`. Этот файл **удаляется**, логика переходит в YAML с поддержкой подстановки env через `$env{VAR}`.

### Задачи

**1.1 Создать `apps/api/static_config.yaml`** ✅

**1.2 Создать `apps/api/dynamic_config_vars.yaml`** ✅

**1.3 Обновить `.env.example`** ✅

Добавлена переменная `DATABASE_URL`:

```
DATABASE_URL=postgresql://postgres:password@localhost:5432/tracker
REDIS_HOST=127.0.0.1
REDIS_PORT=6379
REDIS_PASSWORD=
API_PORT=8080
```

**1.4 Удалить файлы конфигурации Crow**

- Удалить `apps/api/src/config.cpp`
- Удалить `apps/api/include/config.h`

### Проверка

```bash
python3 -c "import yaml; yaml.safe_load(open('apps/api/static_config.yaml'))"
```

---

## Этап 2 — Точка входа и структура компонентов (main.cpp + component-list.cpp)

### Контекст

Текущий `apps/api/src/main.cpp` создаёт `Config::getInstance()` и `ApiServer`. В userver вместо этого есть `userver::utils::DaemonMain` и список компонентов. `apps/api/src/api-server.cpp` **удаляется**.

### Задачи

**2.1 Переписать `apps/api/src/main.cpp`**

```cpp
#include "component-list.hpp"
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    auto component_list = MakeComponentList();
    return userver::utils::DaemonMain(argc, argv, component_list);
}
```

**2.2 Создать `apps/api/src/component-list.cpp` и `apps/api/include/component-list.hpp`**

```cpp
// component-list.hpp
#pragma once
#include <userver/components/component_list.hpp>

userver::components::ComponentList MakeComponentList();
```

```cpp
// component-list.cpp
#include "component-list.hpp"

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/clients/http/component.hpp>

#include "controllers/auth-controller.hpp"
#include "controllers/patient-controller.hpp"
#include "controllers/anamnesis-controller.hpp"
#include "controllers/water-controller.hpp"

userver::components::ComponentList MakeComponentList() {
    auto components = userver::components::MinimalServerComponentList();

    components
        .Append<userver::components::Postgres>("postgres-db")
        .Append<userver::storages::redis::ServiceComponent>("redis-sentinel")
        .Append<AuthHandler>()
        .Append<PatientHandler>()
        .Append<PatientByIdHandler>()
        .Append<AnamnesisHandler>()
        .Append<AnamnesisByIdHandler>()
        .Append<WaterHandler>()
        .Append<WaterByIdHandler>();

    return components;
}
```

**2.3 Удалить файлы**

- `apps/api/src/api-server.cpp`
- `apps/api/include/api-server.h`
- `apps/api/src/config.cpp`
- `apps/api/include/config.h`
- `apps/api/src/controllers/swagger-controller.cpp`
- `apps/api/include/controllers/swagger-controller.h`

### Проверка

```bash
docker build --target builder .
```

`cmake --build` всё ещё будет падать (репозитории не переписаны), но на других ошибках чем раньше.

---

## Этап 3 — Слой common: убрать зависимость от Crow

### Контекст

`libs/common/include/tracker_common/http-responses.h` включает `<crow.h>` и возвращает `crow::response`. Нужно убрать Crow из общего заголовка.

### Задачи

**3.1 Удалить `libs/common/include/tracker_common/http-responses.h` полностью**

Этот файл больше не нужен — ответы в userver формируются иначе. Логика хелперов переедет в базовый класс хэндлеров (Этап 6).

**3.2 Обновить `CMakeLists.txt` для `tracker_common`**

```cmake
add_library(tracker_common STATIC
    libs/common/src/env-parser.cpp
)
target_include_directories(tracker_common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/common/include
)
# Убрать зависимость от Crow
```

**3.3 Проверить `libs/common/src/env-parser.cpp`**

Файл читает `.env`-файл и возвращает `std::map<std::string, std::string>`. Это не зависит от Crow — оставить без изменений.

### Проверка

```bash
docker build --target builder .
```

`tracker_common` собирается без ошибок.

---

## Этап 4 — Слой БД: libs/db (libpq → userver::storages::postgres)

### Контекст

Все репозитории (`PatientRepository`, `UserRepository`, `WaterRepository`, `AnamnesisRepository`) и `AuthService` принимают `db_utils::PGconnPtr` (= `shared_ptr<PGconn>`). В userver работа с БД идёт через `storages::postgres::ClusterPtr` — асинхронный пул соединений.

**Ключевые изменения:**
- Удалить `libs/db/include/tracker_db/db-utils.h`
- Удалить `libs/db/src/postgres-db.cpp` и `libs/db/include/tracker_db/postgres-db.h` (проверить, не использует ли CLI)
- Репозитории принимают `storages::postgres::ClusterPtr` в конструктор

> **Важно:** перед удалением `postgres-db.cpp` выполнить:
> `grep -r "PostgreDB\|postgres-db" apps/cli/`

### Задачи

**4.1 Обновить интерфейс `PatientRepository`**

Файл: `libs/db/include/tracker_db/repositories/patient-repository.h`

```cpp
#pragma once

#include <userver/storages/postgres/cluster.hpp>
#include <tracker/models/patient.h>
#include <optional>
#include <vector>

class PatientRepository {
public:
    explicit PatientRepository(userver::storages::postgres::ClusterPtr cluster);

    std::vector<Patient> getByUserUUID(const std::string& user_uuid);
    std::optional<Patient> getByID(int id_patient);
    void createPatient(const std::string& user_uuid, const std::string& name,
                       std::optional<std::string> birth_date);
    void updatePatient(int id_patient, std::optional<std::string> name,
                       std::optional<std::string> birth_date);
    void deletePatient(int id_patient);

private:
    userver::storages::postgres::ClusterPtr cluster_;
};
```

**4.2 Реализация `PatientRepository`**

Полностью переписать `libs/db/src/repositories/patient-repository.cpp`. Пример для `getByUserUUID`:

```cpp
std::vector<Patient> PatientRepository::getByUserUUID(const std::string& user_uuid) {
    auto result = cluster_->Execute(
        pg::ClusterHostType::kSlave,
        "SELECT id_patient, user_uuid, name, birth_date "
        "FROM patient WHERE user_uuid = $1 AND is_deleted = FALSE",
        user_uuid
    );

    std::vector<Patient> patients;
    for (auto row : result) {
        Patient p;
        p.id_patient = row["id_patient"].As<int>();
        p.user_uuid  = row["user_uuid"].As<std::string>();
        p.name       = row["name"].As<std::string>();
        p.birth_date = row["birth_date"].As<std::optional<std::string>>();
        patients.push_back(std::move(p));
    }
    return patients;
}
```

**4.3 Аналогично переписать остальные репозитории**

- `libs/db/src/repositories/user-repository.cpp`
- `libs/db/src/repositories/water-repository.cpp`
- `libs/db/src/repositories/anamnesis-repository.cpp`

И обновить их заголовки: заменить `db_utils::PGconnPtr` на `userver::storages::postgres::ClusterPtr`, убрать `#include <libpq-fe.h>`.

**4.4 Переписать `AuthService`**

`libs/db/include/tracker_db/usecases/auth-service.h`:

```cpp
#pragma once
#include <userver/storages/postgres/cluster.hpp>
#include <optional>
#include <string>

class AuthService {
public:
    explicit AuthService(userver::storages::postgres::ClusterPtr cluster);

    std::optional<std::string> registerUser(const std::string& email,
                                             const std::string& password);
    std::optional<std::string> loginUser(const std::string& email,
                                          const std::string& password);
    userver::storages::postgres::ClusterPtr getCluster() const;

private:
    userver::storages::postgres::ClusterPtr cluster_;
};
```

**4.5 Удалить устаревшие файлы**

- `libs/db/include/tracker_db/db-utils.h`
- `libs/db/include/tracker_db/postgres-db.h`
- `libs/db/src/postgres-db.cpp`

**4.6 Обновить `tracker_db` в CMakeLists.txt**

```cmake
add_library(tracker_db STATIC
    libs/db/src/repositories/anamnesis-repository.cpp
    libs/db/src/repositories/patient-repository.cpp
    libs/db/src/repositories/user-repository.cpp
    libs/db/src/repositories/water-repository.cpp
    libs/db/src/usecases/auth-service.cpp
    # убрать: libs/db/src/postgres-db.cpp
)
```

### Проверка

```bash
docker build --target builder .
```

`tracker_db` собирается без ошибок.

---

## Этап 5 — Слой сессий: libs/session (redis-plus-plus → userver::storages::redis)

### Контекст

`RedisSessionStore` использует синхронный `sw::redis::Redis`. В userver работа с Redis идёт через `storages::redis::ClientPtr`.

### Задачи

**5.1 Обновить `libs/session/include/tracker_session/redis-session-store.h`**

```cpp
#pragma once

#include "session-store.h"
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <memory>

namespace tracker_session {

class RedisSessionStore : public SessionStore {
public:
    explicit RedisSessionStore(
        std::shared_ptr<userver::storages::redis::Client> client);

    std::string createSession(const std::string& userUuid,
                              int ttlSeconds) override;
    std::optional<std::string> resolveUserUuid(
        const std::string& sid) override;
    void destroySession(const std::string& sid) override;

private:
    static std::string makeKey(const std::string& sid);

    std::shared_ptr<userver::storages::redis::Client> client_;
};

} // namespace tracker_session
```

**5.2 Переписать `libs/session/src/redis-session-store.cpp`**

```cpp
std::string RedisSessionStore::createSession(const std::string& userUuid,
                                              int ttlSeconds) {
    std::string sid = secureRandomHex(32);
    std::string key = makeKey(sid);

    client_->Set(key, userUuid, redis::CommandControl{}).Get();
    client_->Expire(key, std::chrono::seconds(ttlSeconds),
                    redis::CommandControl{}).Get();
    return sid;
}

std::optional<std::string> RedisSessionStore::resolveUserUuid(
    const std::string& sid)
{
    auto result = client_->Get(makeKey(sid), redis::CommandControl{}).Get();
    if (!result) return std::nullopt;
    return *result;
}

void RedisSessionStore::destroySession(const std::string& sid) {
    client_->Del({makeKey(sid)}, redis::CommandControl{}).Get();
}
```

**5.3 Оставить без изменений**

- `libs/session/include/tracker_session/session-store.h`
- `libs/session/src/secure-random.cpp`
- `libs/session/src/cookie.cpp`

### Проверка

```bash
docker build --target builder .
```

`tracker_session` собирается без ошибок.

---

## Этап 6 — Базовый класс хэндлеров (замена AuthMiddleware)

### Контекст

`AuthMiddleware` — статический класс с `static shared_ptr<SessionStore> store_`. В userver авторизация реализуется через базовый класс хэндлера, который получает `RedisSessionStore` через ComponentContext.

### Задачи

**6.1 Создать `apps/api/include/handlers/authenticated-handler.hpp`**

```cpp
#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/components/component_context.hpp>
#include <tracker_session/session-store.h>
#include <tracker_session/cookie.h>
#include <optional>
#include <string>

namespace tracker_api {

class AuthenticatedHandlerBase
    : public userver::server::handlers::HttpHandlerBase
{
public:
    using HttpHandlerBase::HttpHandlerBase;

protected:
    explicit AuthenticatedHandlerBase(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context,
        tracker_session::SessionStore& session_store);

    std::optional<std::string> GetCurrentUserUuid(
        const userver::server::http::HttpRequest& request) const;

    std::optional<std::string> GetSessionId(
        const userver::server::http::HttpRequest& request) const;

    static constexpr const char* kCookieName = "__Host-session";
    static constexpr int kSessionTtlSeconds = 86400;
    static constexpr bool kCookieSecure = true;
    static constexpr const char* kSameSite = "Strict";

private:
    tracker_session::SessionStore& session_store_;
};

} // namespace tracker_api
```

**6.2 Создать `apps/api/src/handlers/authenticated-handler.cpp`**

**6.3 Создать `apps/api/include/handlers/http-helpers.hpp`**

Замена для удалённого `libs/common/include/tracker_common/http-responses.h`.

**6.4 Удалить**

- `apps/api/src/middleware/auth-middleware.cpp`
- `apps/api/include/middleware/auth-middleware.h`

### Проверка

```bash
docker build --target builder .
```

---

## Этап 7 — HTTP-хэндлеры (Crow controllers → userver handlers)

### Контекст

В userver каждый маршрут — отдельный компонент, наследующий `HttpHandlerBase`. Метод `HandleRequestThrow` возвращает `std::string`.

**Маппинг контроллеров на хэндлеры:**

| Crow route | userver handler class | path в config |
|---|---|---|
| POST/GET `/api/patients` | `PatientListHandler` | `/api/patients` |
| GET/PATCH/DELETE `/api/patients/<id>` | `PatientByIdHandler` | `/api/patients/{id}` |
| POST `/api/auth/register` | `AuthHandler` | `/api/auth/{action}` |
| PATCH/DELETE `/api/auth/user` | `AuthUserHandler` | `/api/auth/user` |
| GET/POST `/api/anamnesis` | `AnamnesisListHandler` | `/api/anamnesis` |
| GET/PATCH/DELETE `/api/anamnesis/<id>` | `AnamnesisHandler` | `/api/anamnesis/{id}` |
| GET/POST `/api/water` | `WaterListHandler` | `/api/water` |
| GET/DELETE `/api/water/<id>` | `WaterHandler` | `/api/water/{id}` |

### Задачи

**7.1 Шаблон хэндлера (PatientListHandler как пример)**

```cpp
// apps/api/include/controllers/patient-handler.hpp
class PatientListHandler final : public AuthenticatedHandlerBase {
public:
    static constexpr std::string_view kName = "patient-handler";

    PatientListHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    std::string HandleGet(const userver::server::http::HttpRequest& req) const;
    std::string HandlePost(const userver::server::http::HttpRequest& req) const;

    PatientRepository patient_repo_;
};
```

Конструктор получает `ClusterPtr` из ComponentContext:

```cpp
PatientListHandler::PatientListHandler(...)
    : AuthenticatedHandlerBase(config, context,
          *context.FindComponent<RedisSessionStoreComponent>().GetStore()),
      patient_repo_(
          context.FindComponent<userver::components::Postgres>("postgres-db")
                 .GetCluster())
{}
```

**7.2 PatientByIdHandler**

`GET/PATCH/DELETE /api/patients/{id}`. Параметр пути: `request.GetPathArg("id")`.

**7.3 AuthHandler**

`/api/auth/{action}`, где `action` = `register` | `login` | `logout`.

Установка куки вместо `res.add_header`:
```cpp
req.GetHttpResponse().SetHeader(
    "Set-Cookie",
    tracker_session::cookie::buildSetCookie(
        kCookieName, sid, kSessionTtlSeconds,
        true, kCookieSecure, kSameSite, "/"
    )
);
```

**7.4 AuthUserHandler**

`PATCH/DELETE /api/auth/user`.

**7.5 AnamnesisListHandler и AnamnesisByIdHandler**

**7.6 WaterListHandler и WaterByIdHandler**

**7.7 Компонент RedisSessionStoreComponent**

```cpp
// apps/api/include/components/session-component.hpp
class RedisSessionStoreComponent final
    : public userver::components::ComponentBase
{
public:
    static constexpr std::string_view kName = "session-store";

    RedisSessionStoreComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context);

    tracker_session::SessionStore* GetStore() const;

private:
    std::unique_ptr<tracker_session::RedisSessionStore> store_;
};
```

Добавить в `component-list.cpp`:
```cpp
.Append<RedisSessionStoreComponent>()
```

**7.8 Удалить Crow-контроллеры**

- `apps/api/src/controllers/auth-controller.cpp` и `.h`
- `apps/api/src/controllers/patient-controller.cpp` и `.h`
- `apps/api/src/controllers/water-controller.cpp` и `.h`
- `apps/api/src/controllers/anamnesis-controller.cpp` и `.h`

### Проверка

```bash
docker build .
```

Полная сборка проходит без ошибок.

---

## Этап 8 — Финальная проверка Docker

### Контекст

Dockerfile уже переписан в рамках Этапа 0. Осталось убедиться, что полный стек поднимается.

### Задачи

**8.1 Проверить `docker-compose.yaml`**

В сервисе `api-app` уже добавлен `DATABASE_URL`. Убедиться что `.env` содержит все нужные переменные (`DB_USER`, `DB_PASSWORD`, `DB_NAME`).

**8.2 Поднять стек**

```bash
docker compose up --build
```

### Проверка

API отвечает на `http://localhost:8080`.

---

## Порядок выполнения

```
Этап 0: CMakeLists.txt + vcpkg.json + Dockerfile   ✅ done
Этап 1: static_config.yaml                         ✅ done
Этап 2: main.cpp + component-list.cpp              ~0.5 дня
Этап 3: libs/common (убрать Crow)                  ~0.5 дня
Этап 4: libs/db → userver::postgres                ~3-4 дня   ← самый объёмный
Этап 5: libs/session → userver::redis              ~1-2 дня
Этап 6: AuthenticatedHandlerBase                   ~0.5 дня
Этап 7: HTTP-хэндлеры                              ~3-4 дня
Этап 8: Финальная проверка Docker                  ~0.5 дня
```

**Итого:** ~10–12 рабочих дней.

---

## Что НЕ меняется

| Компонент | Причина |
|---|---|
| `apps/cli/` | Независим, использует CPR через vcpkg |
| `libs/core/include/tracker/models/` | Чистые struct без зависимостей |
| `libs/core/src/models/patient.cpp` | Чистая логика |
| `libs/crypto/` | Argon2, нет userver-аналога |
| `libs/db/migrations/` | SQL-схема не меняется |
| `libs/session/src/cookie.cpp` | Чистый C++ |
| `libs/session/src/secure-random.cpp` | Чистый C++ |
| `docker-compose.yaml` (PostgreSQL + Redis сервисы) | Сервисы не меняются |

---

## Известные риски

1. **Время сборки userver** — 20–40 минут на первом запуске. Docker кеширует слой после cmake configure — повторные сборки быстрые.

2. **Конфликт fmt** — userver подтягивает свою версию `fmt`. Если возникают ошибки линковки — убрать `fmt` из `vcpkg.json` и использовать userver-ный через `userver::fmt`.

3. **`postgres-db.cpp` и CLI** — перед удалением `postgres-db.cpp` проверить: `grep -r "PostgreDB\|postgres-db" apps/cli/`.

4. **userver Redis API** — точный API `storages::redis::Client` для SET/GET/EXPIRE/DEL нужно сверить с документацией userver 3.0. Сигнатуры могли измениться.

5. **`__Host-` куки и HTTPS** — кука `__Host-session` с `Secure=true` работает только по HTTPS. В development без TLS нужно убрать `__Host-` префикс или отключить `Secure` через env-переменную.

6. **AuthenticatedHandlerBase и DI** — `RedisSessionStoreComponent` должен быть зарегистрирован раньше хэндлеров в `component-list.cpp`.
