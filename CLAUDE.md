# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project shape

Two C++20 binaries built from one CMake tree:

- **`tracker_api`** — userver 3.0 HTTP service (PostgreSQL + Redis). Pulled in via `FetchContent` and **only built when `BUILD_API=ON`** (the default on Linux/WSL, **forced OFF on Windows** in `CMakeLists.txt:17-19` because userver does not build natively on Windows).
- **`tracker_cli`** — terminal client using `cpr` + `nlohmann_json`. Has no userver dependency, builds natively on Windows. This is the only target available in a Windows-host build.

When working on Windows, only the CLI compiles — to touch API code you need WSL/Linux or the Docker build. Don't try to "fix" the `if(WIN32) ... BUILD_API OFF` block; it's deliberate (see `memory/project_userver_windows.md`).

## Common commands

### Build (Linux/WSL — full build)

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build --target tracker_api -j$(nproc)
cmake --build build --target tracker_cli
```

### Build (Windows — CLI only)

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"     # must be re-exported per shell
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release --target tracker_cli
```

### Run the API stack

```bash
docker compose up --build -d        # starts postgres, redis, runs migrations, builds + starts api
docker compose restart api-app      # picks up edits to static_config.yaml / dynamic_config_vars.yaml / secdist.json (mounted as volumes — no rebuild needed)
docker compose logs -f api-app
```

`.env` is required: `DB_USER`, `DB_PASSWORD`, `DB_NAME`, optional `API_PORT` (default 8080). `DATABASE_URL` is assembled in `docker-compose.yaml`.

### Tests

Integration tests in `tests/` run against a live API (default `http://localhost:8080`). Bring the stack up first.

```bash
pip install -r tests/requirements.txt
pytest tests                                    # all
pytest tests/test_water.py                      # one file
pytest tests/test_water.py::test_create_water   # one test
pytest tests -n auto                            # parallel — safe, see "Test isolation" below
API_BASE_URL=http://staging:8080 pytest tests   # different host
```

If the API is unreachable, `_wait_for_api` in `tests/conftest.py` skips the run with a clear message rather than letting it cascade into red `ConnectionError`s.

### DB migrations

SQL files in `libs/db/migrations/` are applied by the `migrations` service in `docker-compose.yaml` via `apply_migrations.sh`. To add one, drop a numbered `.sql` file in that directory and `docker compose up migrations` (or just restart the stack). There's no migration tool — just ordered SQL files.

## Architecture

### Layered libraries (`libs/`)

```
tracker_common ──► tracker_crypto ──┐
       │                            ├──► tracker_db ──┐
       └────────► tracker_core ─────┘                 ├──► tracker_api
                                    tracker_session ──┘
                                                      └──► tracker_cli (no userver)
```

- `tracker_common` — env-file parser, only stdlib.
- `tracker_crypto` — Argon2 password hashing (`PasswordHasher::hashPassword/verifyPassword`).
- `tracker_core` — POD models (`Patient`, `User`, `Water`, `Anamnesis`). No DB or HTTP.
- `tracker_db` — repositories take `userver::storages::postgres::ClusterPtr` by value; `AuthService` is the only "use case" wrapper. Queries are inline `R"(... )"` strings, parametrized.
- `tracker_session` — `SessionStore` interface + `RedisSessionStore` (`userver::storages::redis::Client`). Used by API only.

### API request lifecycle (userver pattern)

Every route is a userver **component** that subclasses `HttpHandlerBase`. Authentication-required handlers subclass `AuthenticatedHandlerBase` (`apps/api/include/handlers/authenticated-handler.hpp`), which:

1. Gets ctor-injected with `SessionStore&` from `SessionStoreComponent`.
2. Reads `session_uuid` cookie via `GetSessionId(request)`.
3. Resolves it to `user_uuid` via `GetCurrentUserUuid(request)` → returns `std::nullopt` if missing/invalid → handler returns `http::Unauthorized(...)`.

Adding a new handler requires **three** edits in lockstep — forgetting any one causes a silent 404 or a ctor crash at startup:

1. Create `apps/api/include/handlers/<name>.hpp` + `apps/api/src/handlers/<name>.cpp`.
2. Register the class in `apps/api/src/component-list.cpp` (`.Append<...>()`).
3. Declare the route + methods in `apps/api/static_config.yaml` under `components:` (the component name in YAML must match the class's `kName`).

### Configuration files (`apps/api/`)

All three are mounted as volumes — edit and `docker compose restart api-app`, no rebuild:

- `static_config.yaml` — userver components, HTTP routes, task processors, postgres/redis wiring.
- `dynamic_config_vars.yaml` — runtime knobs.
- `secdist.json` — Redis sentinel hosts/password (mounted at `/etc/tracker/secdist.json`).

`DATABASE_URL` is read from env (`dbconnection#env: DATABASE_URL`).

### Auth model

Cookie-based. On `register`/`login`, `AuthHandler` calls `session_store().createSession(user_uuid, ttl)` and writes a `session_uuid` HTTP-only cookie via `tracker_session::cookie::buildSetCookie`. Sessions live in Redis with TTL = `kSessionTtlSeconds` (24h). `AuthHandler` redacts the `password` field in `GetRequestBodyForLogging` — preserve that when changing the handler.

`apps/api/static_config.yaml` routes all auth verbs through one path `/api/auth/{action}` (action ∈ `register|login|logout|user`); the dispatch by `action`+method is inside `AuthHandler::HandleRequestThrow`.

### CLI ↔ API

`tracker_cli` talks to `${API_URL}` from `.env` via `cpr`. Cookies are extracted manually in `apps/cli/src/api-client.cpp::extractSessionCookie` because cpr's auto-cookie-jar misses some `Set-Cookie` formats — leave the three-tier fallback (cpr cookies → headers map → raw_header parse).

### Test design

`tests/conftest.py` parses `apps/api/static_config.yaml` and exposes `url_for(handler_name, base_url, **path_args)` so tests reference handlers by their YAML component name, not hardcoded URLs. Renaming a handler or its `path` will break tests loudly (which is the point). When you change a route, update `static_config.yaml` first — tests pick it up automatically.

**Test isolation**: every test registers a unique user (`uuid4`-based email), and the `auth` / `second_auth` fixtures tear down via `DELETE /api/auth/user` — that's why `pytest -n auto` is safe and why deleting a user must cascade-delete patients/anamnesis/water (which it does via `ON DELETE CASCADE` in `001_init_schema.sql`).

## Known API contract issues

There's a roadmap in `API_ROADMAP.md` documenting known gaps and inconsistencies (e.g. `/api/anamnesis/{id}` and `/api/water/{id}` use `{id}` as `patient_id` for GET but `id_anamnesis`/`id_water` for PATCH/DELETE — semantic conflict; `water_frequency` table has no write endpoint). Read it before designing changes to those handlers.

## Conventions worth knowing

- Repositories use **inline raw-string SQL**, not query builders. Casts like `last_water::text` and `$1::uuid` are intentional — they keep userver's PG type mapping happy.
- Handlers return `std::string` from `HandleRequestThrow` and use the helpers in `apps/api/include/handlers/http-helpers.hpp` (`http::Ok`, `http::BadRequest`, `http::Unauthorized`, `http::Forbidden`, `http::NotFound`, `http::NoContent`, `http::Created`, `http::InternalError`, `http::SetStatus`) — don't manually set status + body.
- Path args are read with `request.GetPathArg("id")` and parsed with `std::stoi` — wrap in try/catch and return `http::BadRequest` (see `patient-by-id-handler.cpp:38-43` for the pattern).
- Soft-delete columns (`is_deleted`) exist in `users`, `patient`, `anamnesis` but **are not used** by any current code path. Don't assume they filter — they don't.
