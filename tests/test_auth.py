"""Tests for /api/auth/{action} — register / login / logout / user."""

import uuid

import pytest
import requests

from conftest import AuthSession, register_user, url_for


pytestmark = pytest.mark.integration


def test_register_sets_session_cookie(base_url):
    email = f"test-{uuid.uuid4().hex}@example.com"
    s = requests.Session()
    r = s.post(
        url_for("auth-handler", base_url, action="register"),
        json={"email": email, "password": "P@ssw0rd!"},
        timeout=10,
    )
    assert r.status_code == 201
    body = r.json()
    assert body["user_uuid"]
    assert "session_uuid" in s.cookies
    s.delete(url_for("auth-handler", base_url, action="user"), timeout=10)


def test_register_duplicate_email_rejected(base_url, auth: AuthSession):
    r = requests.post(
        url_for("auth-handler", base_url, action="register"),
        json={"email": auth.email, "password": "another"},
        timeout=10,
    )
    assert r.status_code == 400


def test_login_success(base_url, auth: AuthSession):
    s = requests.Session()
    r = s.post(
        url_for("auth-handler", base_url, action="login"),
        json={"email": auth.email, "password": auth.password},
        timeout=10,
    )
    assert r.status_code == 200
    body = r.json()
    assert body["user_uuid"] == auth.user_uuid
    assert "session_uuid" in s.cookies


def test_login_wrong_password(base_url, auth: AuthSession):
    r = requests.post(
        url_for("auth-handler", base_url, action="login"),
        json={"email": auth.email, "password": "wrong"},
        timeout=10,
    )
    assert r.status_code == 401


def test_login_unknown_user(base_url):
    r = requests.post(
        url_for("auth-handler", base_url, action="login"),
        json={"email": f"nope-{uuid.uuid4().hex}@example.com", "password": "x"},
        timeout=10,
    )
    assert r.status_code == 401


def test_logout_clears_session(base_url):
    a = register_user(base_url)
    r = a.post(url_for("auth-handler", base_url, action="logout"))
    assert r.status_code == 200

    # Cookie no longer authorises requests.
    r2 = a.get(url_for("patient-handler", base_url))
    assert r2.status_code == 401


def test_unknown_action_returns_405(base_url, auth: AuthSession):
    r = auth.post(url_for("auth-handler", base_url, action="hack"), json={})
    assert r.status_code == 405


def test_get_method_is_not_supported(base_url, auth: AuthSession):
    """GET is declared in static_config but not implemented for any action."""
    r = auth.get(url_for("auth-handler", base_url, action="login"))
    assert r.status_code == 405


def test_update_user_email(base_url, auth: AuthSession):
    new_email = f"renamed-{uuid.uuid4().hex}@example.com"
    r = auth.patch(
        url_for("auth-handler", base_url, action="user"),
        json={"email": new_email},
    )
    assert r.status_code == 200
    body = r.json()
    assert body["email"] == new_email
    assert body["user_uuid"] == auth.user_uuid


def test_update_user_requires_at_least_one_field(base_url, auth: AuthSession):
    r = auth.patch(url_for("auth-handler", base_url, action="user"), json={})
    assert r.status_code == 400


def test_update_user_email_conflict(base_url, auth: AuthSession, second_auth: AuthSession):
    r = auth.patch(
        url_for("auth-handler", base_url, action="user"),
        json={"email": second_auth.email},
    )
    assert r.status_code == 409


def test_update_user_unauthorized(base_url):
    r = requests.patch(
        url_for("auth-handler", base_url, action="user"),
        json={"email": "x@x.x"},
        timeout=10,
    )
    assert r.status_code == 401


def test_delete_user_kills_session(base_url):
    a = register_user(base_url)
    r = a.delete(url_for("auth-handler", base_url, action="user"))
    assert r.status_code == 204

    # Re-using the now-defunct session must fail.
    r2 = a.get(url_for("patient-handler", base_url))
    assert r2.status_code == 401
