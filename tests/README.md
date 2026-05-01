# API tests

Integration-тесты для `tracker_api`. Читают `apps/api/static_config.yaml`,
поднимают сессию через `/api/auth/register` и прогоняют CRUD по каждой ручке.

## Что покрыто

| Файл | Ручка из `static_config.yaml` |
|---|---|
| `test_static_config.py` | проверяет, что все ожидаемые `*-handler` объявлены с нужным path и method |
| `test_auth.py` | `auth-handler` — `/api/auth/{action}` |
| `test_patients.py` | `patient-handler`, `patient-by-id-handler` |
| `test_anamnesis.py` | `anamnesis-handler`, `anamnesis-by-id-handler` |
| `test_water.py` | `water-handler`, `water-by-id-handler` |

`conftest.py` парсит `static_config.yaml` и собирает URL-ы из реальной
конфигурации, поэтому при переименовании ручки или смены пути тесты
сломаются явно (а не молча).

## Запуск

1. Подними сервис со всеми зависимостями:
   ```bash
   docker compose up --build -d
   ```
2. Установи зависимости тестов (Python 3.10+):
   ```bash
   pip install -r tests/requirements.txt
   ```
3. Прогон:
   ```bash
   pytest tests
   ```

По умолчанию тесты идут по `http://localhost:8080`. Поменять адрес можно
через переменные окружения:

- `API_BASE_URL` — базовый URL (по умолчанию `http://localhost:8080`)
- `API_TIMEOUT` — таймаут одного запроса в секундах (по умолчанию `10`)

Если API недоступен, session-фикстура `_wait_for_api` помечает прогон как
`skipped` с понятным сообщением — это лучше, чем красная гора падений по
`ConnectionError`.

## Изоляция

- Каждый тест регистрирует уникального пользователя (`uuid4` в email) и
  удаляет его в teardown через `DELETE /api/auth/user`.
- Из-за этого тесты можно гонять параллельно (`pytest -n auto`) и
  повторно — состояние Postgres/Redis между прогонами не утекает.
