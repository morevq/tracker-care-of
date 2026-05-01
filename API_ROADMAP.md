# План доработки API

Документ описывает пробелы в текущем API `tracker-care-of` и план их закрытия. Основан на аудите состояния ветки `feature/migrate-to-userver` на 2026-05-01.

Структура: каждая часть — отдельный логический блок (security, REST-консистентность, фичи, операционная зрелость, документация). Внутри части — конкретные задачи с критериями готовности и ссылками на файлы, которые предстоит править.

---

## Часть 0. Контекст и текущее состояние

### Существующие эндпоинты

| Метод | Путь | Хэндлер |
|---|---|---|
| POST | `/api/auth/register` | `AuthHandler::Register` |
| POST | `/api/auth/login` | `AuthHandler::Login` |
| POST | `/api/auth/logout` | `AuthHandler::Logout` |
| PATCH | `/api/auth/user` | `AuthHandler::UpdateUser` |
| DELETE | `/api/auth/user` | `AuthHandler::DeleteUser` |
| GET / POST | `/api/patients` | `PatientListHandler` |
| GET / PATCH / DELETE | `/api/patients/{id}` | `PatientByIdHandler` |
| POST | `/api/anamnesis` | `AnamnesisListHandler` |
| GET / PATCH / DELETE | `/api/anamnesis/{id}` | `AnamnesisByIdHandler` |
| GET / POST | `/api/water` | `WaterListHandler` |
| GET / DELETE | `/api/water/{id}` | `WaterByIdHandler` |

### Ключевые архитектурные проблемы

1. `GET /api/anamnesis/{id}` трактует `{id}` как `patient_id`, а `PATCH/DELETE` — как `anamnesis_id`. Один URI обозначает разные ресурсы.
2. `GET /api/water/{id}` тоже принимает `patient_id`, а не `id_water` — название параметра врёт.
3. Таблица `water_frequency` есть в схеме и читается репозиторием, но через API в неё ничего записать нельзя — ключевая фича из README не работает.
4. `PATCH /api/auth/user` меняет пароль без проверки текущего — компрометация cookie ведёт к моментальной потере аккаунта.
5. Колонки `is_deleted` есть в `users`, `patient`, `anamnesis`, но ни выставляются, ни используются — мёртвая часть схемы.

---

## Часть 1. Security и корректность контракта (P0)

Цель — закрыть очевидные дыры и устранить семантические конфликты до того, как поверх API начнут писать клиенты.

### 1.1 Смена пароля с проверкой текущего

**Что**: новый эндпоинт `POST /api/auth/change-password`.

**Тело**: `{ current_password, new_password }`.

**Поведение**:
- проверка `current_password` через `PasswordHasher::verifyPassword`;
- хеширование `new_password`, обновление `users.password_hash`;
- инвалидация **всех** сессий пользователя, кроме текущей (или всех — настройка).

**Изменения**:
- `apps/api/src/handlers/auth-handler.cpp` — новая ветка `action == "change-password"`;
- `apps/api/dto/auth-dto.h` — `ChangePasswordRequest`;
- `libs/session/include/tracker_session/session-store.h` — метод `destroyAllSessionsForUser(user_uuid)` (новая ответственность стора);
- `apps/api/static_config.yaml` — путь уже покрыт шаблоном `/api/auth/{action}`.

**Готово, когда**: интеграционный тест в `tests/test_auth.py` проверяет (а) отказ при неверном current_password → 401, (б) успешную смену + инвалидацию старых cookie.

### 1.2 Запретить смену пароля через PATCH /api/auth/user

После 1.1 убрать обработку `password` из `UpdateUser` ([apps/api/src/handlers/auth-handler.cpp:170](apps/api/src/handlers/auth-handler.cpp)). Оставить только смену email. Если в теле пришёл `password` — вернуть 400 со ссылкой на `change-password`.

### 1.3 GET /api/auth/me

**Что**: вернуть профиль текущего пользователя.

**Ответ**: `{ user_uuid, email, created_at }`.

**Изменения**:
- ветка `action == "me"` в `AuthHandler` с методом GET;
- метод роутера `/api/auth/{action}` — добавить `GET` в список methods в `static_config.yaml`.

### 1.4 Развести семантику `/api/anamnesis/{id}` и `/api/water/{id}`

Это **breaking change** в URL — лучше делать одним PR и сразу обновить CLI.

**Новая схема**:

| Метод | Путь | Назначение |
|---|---|---|
| GET | `/api/patients/{patient_id}/anamnesis` | список анамнезов пациента |
| POST | `/api/patients/{patient_id}/anamnesis` | создать анамнез (вместо POST `/api/anamnesis` с patient_id в теле) |
| GET | `/api/anamnesis/{id}` | одна запись по `id_anamnesis` |
| PATCH | `/api/anamnesis/{id}` | обновить запись |
| DELETE | `/api/anamnesis/{id}` | удалить запись |
| GET | `/api/patients/{patient_id}/water` | история поливов пациента |
| POST | `/api/patients/{patient_id}/water` | отметить полив |
| GET | `/api/water/{id}` | одна запись по `id_water` |
| PATCH | `/api/water/{id}` | поправить timestamp полива |
| DELETE | `/api/water/{id}` | удалить запись о поливе |

**Изменения**:
- разбить `AnamnesisListHandler` и `AnamnesisByIdHandler`, добавить `PatientAnamnesisHandler` (nested);
- то же для water;
- репозиторий `AnamnesisRepository` уже имеет `getByID` — достаточно скорректировать handler;
- `WaterRepository` нужно дополнить `getByID(int id_water)` (сейчас есть только `getByPatientID`);
- `apps/cli/src/api-client.cpp` — переписать `getAnamnesisByPatient`, `deleteWater` под новые URL.

**Готово, когда**: все интеграционные тесты `tests/test_anamnesis.py` и `tests/test_water.py` адаптированы под новые маршруты и зелёные.

### 1.5 CRUD для `water_frequency`

Ключевая фича README, которая сейчас сломана.

| Метод | Путь | Тело / эффект |
|---|---|---|
| GET | `/api/patients/{patient_id}/water-frequency` | `{ frequency, frequency_measure }` или 404 |
| PUT | `/api/patients/{patient_id}/water-frequency` | upsert: `{ frequency, frequency_measure }` |
| DELETE | `/api/patients/{patient_id}/water-frequency` | удалить настройку |

**Изменения**:
- `WaterRepository`: добавить `setFrequency`, `getFrequency`, `deleteFrequency`;
- новый хэндлер `WaterFrequencyHandler` (или расширение существующих);
- регистрация в `static_config.yaml` и `component-list.cpp`.

**Валидация**: `frequency > 0`, `frequency_measure ∈ {'days','weeks','hours'}` (зафиксировать enum в DTO).

---

## Часть 2. Управление пользователем и сессиями (P1)

### 2.1 Восстановление пароля

```
POST /api/auth/forgot-password   { email }                 → 204 (всегда, чтобы не палить, какие email есть в системе)
POST /api/auth/reset-password    { token, new_password }   → 200
```

**Архитектура**:
- таблица `password_reset_tokens (token UUID PK, user_uuid, expires_at, used_at)`;
- миграция `003_password_reset.sql`;
- доставка письма — пока stub в логах (`tracker_common`); вынести интерфейс `MailerComponent`, реализация SMTP — на потом;
- TTL токена — 1 час, одноразовый.

**Готово, когда**: запрос forgot-password создаёт токен в БД и логирует ссылку; reset-password по токену меняет пароль и инвалидирует все сессии.

### 2.2 Список и управление сессиями

```
GET    /api/auth/sessions            → [{ session_id, created_at, last_seen, user_agent, ip }]
DELETE /api/auth/sessions/{sid}      → 204 (только свои)
POST   /api/auth/logout-all          → 204
```

**Изменения**:
- `RedisSessionStore` хранит сейчас `sid → user_uuid`. Нужно дополнительно: `user_uuid → set<sid>` (Redis SET) + per-session metadata hash;
- при логине сохранять `User-Agent` и IP-источник;
- хэндлер `SessionsHandler`.

### 2.3 Email-верификация (опционально, P2)

Если решим, что нужна:
```
POST /api/auth/verify-email     { token }
POST /api/auth/resend-verification
```
Колонка `users.email_verified_at TIMESTAMPTZ`. Логин не блокируется до верификации (ленивый подход), но критичные эндпоинты (например, password reset) её требуют.

---

## Часть 3. CRUD-полнота и масштабирование (P1)

### 3.1 Пагинация и поиск

`GET /api/patients`, `GET /api/water`, `GET /api/patients/{id}/anamnesis` сейчас возвращают всё одним запросом. Добавить:

- `?limit=` (default 50, max 200)
- `?offset=` (или `?cursor=` на base64-encoded `(created_at, id)` — предпочтительнее для стабильности)
- `?search=` для patients (по `name`, ILIKE)
- `?from=` / `?to=` для water и anamnesis (фильтр по дате)

**Ответ**: обернуть в `{ items: [...], next_cursor: "..." | null, total?: N }`. Обсудить, нужен ли total (дорогой COUNT).

### 3.2 PATCH /api/water/{id}

Сейчас полив можно создать или удалить, но нельзя поправить опечатку в `last_water`. Добавить PATCH с полем `last_water`. Та же валидация: дата не раньше `patient.birth_date`.

### 3.3 «Кому пора поливать»

```
GET /api/water/due        → [{ patient_id, name, last_water, frequency, due_at, overdue_days }]
```

Опирается на `water_frequency`. Без неё (см. 1.5) не имеет смысла. Это фича-флагман для UX.

---

## Часть 4. Soft-delete: довести или убрать

Колонки `is_deleted` есть в `users`, `patient`, `anamnesis`, но ни выставляются, ни читаются. Два пути — выбрать один:

### Вариант A: довести soft-delete

- DELETE на /api/patients/{id} проставляет `is_deleted = TRUE`, не удаляет физически;
- все SELECT добавляют `WHERE is_deleted = FALSE`;
- новый эндпоинт `POST /api/patients/{id}/restore`;
- крон-задание (отдельный сервис или userver-component с `Cron`) физически удаляет записи старше 30 дней;
- `GET /api/patients?include_deleted=true` для UI «корзины».

### Вариант B: убрать колонки

- миграция `004_drop_is_deleted.sql`;
- удалить упоминания из репозиториев (если есть).

**Рекомендация**: вариант A, если планируется UX «отмена удаления». Иначе B — мёртвый код в схеме хуже отсутствия фичи.

---

## Часть 5. Operational зрелость (P2)

### 5.1 Health и readiness

```
GET /health   → 200 если процесс жив
GET /ready    → 200 если PG отвечает + Redis отвечает, иначе 503
```

Использовать `userver::components::TestsuiteSupport` уже не подходит — нужен отдельный handler без аутентификации, который пингует `cluster_->Execute(... 'SELECT 1')` и `redis_client->Ping()`.

### 5.2 Метрики

userver даёт `handler-server-monitor` из коробки — добавить компонент в `static_config.yaml` на отдельном порту (например, 8081), не выставлять наружу.

### 5.3 Версия / build info

```
GET /api/version → { version, git_sha, built_at }
```

Прокидывать `GIT_SHA` и `BUILD_DATE` через CMake `add_compile_definitions` в `apps/api/CMakeLists.txt`.

### 5.4 Rate-limiting на `/api/auth/*`

Брутфорс `login` сейчас не ограничен. Реализация: счётчик в Redis с TTL 60 секунд per-`(ip, email)`, лимит — 5 попыток. Аналогично для `forgot-password` per-email.

При превышении — 429 с `Retry-After`.

### 5.5 Структурированные ошибки

Сейчас текстовые сообщения. Привести к единому виду:
```json
{ "error": { "code": "INVALID_CREDENTIALS", "message": "...", "details": {} } }
```
Сделать helper в `handlers/http-helpers.hpp`.

---

## Часть 6. Документация и discoverability (P2)

### 6.1 OpenAPI / Swagger

- завести `apps/api/openapi.yaml` вручную (на текущий момент ≈15 эндпоинтов, поддерживать вручную дешевле кодогенерации);
- эндпоинт `GET /api/openapi.yaml` — отдаёт статический файл;
- (опционально) Swagger UI как отдельный handler с HTML.

### 6.2 CORS / OPTIONS

Если планируется браузерный фронт — добавить middleware/handler для preflight. Пока только CLI на cpr — можно отложить, но решение зафиксировать.

### 6.3 Обновить README

- Перерисовать таблицу эндпоинтов после рефакторинга 1.4;
- Добавить раздел «Аутентификация → cookie + change-password flow»;
- Описать формат ошибок (см. 5.5).

---

## Часть 7. Загрузка файлов (P3)

`anamnesis.photo_url` хранит строку, но загружать файл некуда.

### Решение
- выбрать backend: локальный диск (`/var/data/photos`), S3-совместимое (MinIO в docker-compose), или оставить «внешний URL» и явно задокументировать;
- если локально/MinIO:
  - `POST /api/anamnesis/{id}/photo` (multipart) → `{ photo_url }`;
  - `DELETE /api/anamnesis/{id}/photo`;
  - валидация content-type (image/png, image/jpeg), max 5 MB;
  - имена файлов — UUID, не оригинальные (защита от path traversal).

Решение требует обсуждения — это влияет на инфраструктуру и docker-compose.

---

## Дорожная карта по спринтам

### Спринт 1 (P0 — security/корректность)

- [ ] 1.1 `POST /api/auth/change-password`
- [ ] 1.2 запрет смены пароля через PATCH user
- [ ] 1.3 `GET /api/auth/me`
- [ ] 1.4 рефакторинг `/anamnesis` и `/water` на nested-маршруты + обновление CLI
- [ ] 1.5 CRUD для `water_frequency`

### Спринт 2 (P1 — UX и масштабирование)

- [ ] 2.1 forgot-password / reset-password (со stub-mailer)
- [ ] 2.2 список и отзыв сессий, logout-all
- [ ] 3.1 пагинация на list-эндпоинтах
- [ ] 3.2 PATCH water
- [ ] 3.3 `GET /api/water/due`

### Спринт 3 (P2 — операционная зрелость)

- [ ] 4.x soft-delete: решить вариант A или B и применить
- [ ] 5.1 `/health` и `/ready`
- [ ] 5.2 метрики
- [ ] 5.3 `/api/version`
- [ ] 5.4 rate-limit на auth
- [ ] 5.5 единый формат ошибок
- [ ] 6.1 OpenAPI
- [ ] 6.3 README

### Бэклог (P3)

- [ ] 2.3 email-верификация
- [ ] 6.2 CORS, если появится фронт
- [ ] 7.x загрузка фотографий

---

## Точки, требующие решения до старта

1. **Soft-delete**: вариант A (доделать) или B (выкинуть)?
2. **Хранилище фото**: локальный диск, MinIO, внешний URL?
3. **Email-доставка**: сразу подключать SMTP или жить со stub-mailer?
4. **Email-верификация**: нужна или можно без неё?
5. **CORS**: будет ли в обозримом будущем браузерный клиент?
6. **Breaking change в URL (1.4)**: версионировать API (`/api/v1/...`) или просто переехать с обновлением CLI?
