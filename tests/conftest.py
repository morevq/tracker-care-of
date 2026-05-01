"""Fixtures and helpers for API integration tests.

Endpoints are loaded from `apps/api/static_config.yaml` so the tests stay in
sync with the actual server configuration.
"""

from __future__ import annotations

import os
import uuid
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator

import pytest
import requests
import yaml

ROOT = Path(__file__).resolve().parent.parent
STATIC_CONFIG = ROOT / "apps" / "api" / "static_config.yaml"

DEFAULT_BASE_URL = os.environ.get("API_BASE_URL", "http://localhost:8080")
DEFAULT_TIMEOUT = float(os.environ.get("API_TIMEOUT", "10"))


@dataclass(frozen=True)
class HandlerSpec:
    name: str
    path: str
    methods: tuple[str, ...]


def _load_handlers() -> dict[str, HandlerSpec]:
    with STATIC_CONFIG.open("r", encoding="utf-8") as f:
        cfg = yaml.safe_load(f)

    components = cfg["components_manager"]["components"]
    handlers: dict[str, HandlerSpec] = {}
    for name, body in components.items():
        if not isinstance(body, dict) or "path" not in body or "method" not in body:
            continue
        methods = tuple(m.strip().upper() for m in str(body["method"]).split(","))
        handlers[name] = HandlerSpec(name=name, path=body["path"], methods=methods)
    return handlers


HANDLERS = _load_handlers()


def url_for(handler_name: str, base_url: str, **path_args: str | int) -> str:
    spec = HANDLERS[handler_name]
    path = spec.path
    for key, value in path_args.items():
        path = path.replace("{" + key + "}", str(value))
    if "{" in path:
        raise ValueError(f"Missing path arg for {handler_name}: {path}")
    return base_url.rstrip("/") + path


@pytest.fixture(scope="session")
def base_url() -> str:
    return DEFAULT_BASE_URL


@pytest.fixture(scope="session")
def handlers() -> dict[str, HandlerSpec]:
    return HANDLERS


@pytest.fixture(scope="session", autouse=True)
def _wait_for_api(base_url: str) -> None:
    """Fail fast with a clear message if the API is not reachable."""
    try:
        requests.get(base_url + "/api/patients", timeout=DEFAULT_TIMEOUT)
    except requests.RequestException as exc:
        pytest.skip(f"API at {base_url} is not reachable: {exc}")


def _unique_email() -> str:
    return f"test-{uuid.uuid4().hex}@example.com"


@dataclass
class AuthSession:
    session: requests.Session
    email: str
    password: str
    user_uuid: str

    def get(self, url: str, **kw):
        kw.setdefault("timeout", DEFAULT_TIMEOUT)
        return self.session.get(url, **kw)

    def post(self, url: str, **kw):
        kw.setdefault("timeout", DEFAULT_TIMEOUT)
        return self.session.post(url, **kw)

    def patch(self, url: str, **kw):
        kw.setdefault("timeout", DEFAULT_TIMEOUT)
        return self.session.patch(url, **kw)

    def delete(self, url: str, **kw):
        kw.setdefault("timeout", DEFAULT_TIMEOUT)
        return self.session.delete(url, **kw)


def register_user(base_url: str, email: str | None = None, password: str = "P@ssw0rd!") -> AuthSession:
    s = requests.Session()
    email = email or _unique_email()
    r = s.post(
        url_for("auth-handler", base_url, action="register"),
        json={"email": email, "password": password},
        timeout=DEFAULT_TIMEOUT,
    )
    assert r.status_code == 201, f"register failed: {r.status_code} {r.text}"
    body = r.json()
    return AuthSession(session=s, email=email, password=password, user_uuid=body["user_uuid"])


@pytest.fixture
def auth(base_url: str) -> Iterator[AuthSession]:
    """Fresh registered user; tears itself down via DELETE /api/auth/user."""
    a = register_user(base_url)
    try:
        yield a
    finally:
        try:
            a.delete(url_for("auth-handler", base_url, action="user"))
        except requests.RequestException:
            pass


@pytest.fixture
def second_auth(base_url: str) -> Iterator[AuthSession]:
    """A second independent user (used for cross-user authorization checks)."""
    a = register_user(base_url)
    try:
        yield a
    finally:
        try:
            a.delete(url_for("auth-handler", base_url, action="user"))
        except requests.RequestException:
            pass


@pytest.fixture
def patient_id(auth: AuthSession, base_url: str) -> int:
    """Create a patient owned by `auth` and return its id."""
    r = auth.post(
        url_for("patient-handler", base_url),
        json={"name": "Fern", "birth_date": "2020-01-15"},
    )
    assert r.status_code == 201, r.text

    list_r = auth.get(url_for("patient-handler", base_url))
    assert list_r.status_code == 200, list_r.text
    items = list_r.json()
    assert items, "patient list is empty after POST"
    return items[-1]["id"]
