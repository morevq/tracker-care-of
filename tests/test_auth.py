"""Tests for /api/auth/{action} — register / login / logout / me /
change-password / user (PATCH/DELETE)."""

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

    r2 = a.get(url_for("patient-handler", base_url))
    assert r2.status_code == 401


def test_me_requires_auth(base_url):
    r = requests.get(url_for("auth-handler", base_url, action="me"), timeout=10)
    assert r.status_code == 401


def test_me_returns_profile(base_url, auth: AuthSession):
    r = auth.get(url_for("auth-handler", base_url, action="me"))
    assert r.status_code == 200
    body = r.json()
    assert body["user_uuid"] == auth.user_uuid
    assert body["email"] == auth.email
    assert body["created_at"]


def test_unknown_action_returns_405(base_url, auth: AuthSession):
    r = auth.post(url_for("auth-handler", base_url, action="hack"), json={})
    assert r.status_code == 405


def test_get_login_returns_405(base_url, auth: AuthSession):
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


def test_update_user_rejects_password(base_url, auth: AuthSession):
    r = auth.patch(
        url_for("auth-handler", base_url, action="user"),
        json={"password": "newPass!1"},
    )
    assert r.status_code == 400


def test_change_password_requires_auth(base_url):
    r = requests.post(
        url_for("auth-handler", base_url, action="change-password"),
        json={"current_password": "x", "new_password": "y"},
        timeout=10,
    )
    assert r.status_code == 401


def test_change_password_wrong_current_returns_401(base_url, auth: AuthSession):
    r = auth.post(
        url_for("auth-handler", base_url, action="change-password"),
        json={"current_password": "definitely-wrong", "new_password": "newPa$$1"},
    )
    assert r.status_code == 401


def test_change_password_blank_new_returns_400(base_url, auth: AuthSession):
    r = auth.post(
        url_for("auth-handler", base_url, action="change-password"),
        json={"current_password": auth.password, "new_password": "   "},
    )
    assert r.status_code == 400


def test_change_password_invalidates_old_session(base_url):
    a = register_user(base_url)

    # A second concurrent session for the same user.
    other = requests.Session()
    login = other.post(
        url_for("auth-handler", base_url, action="login"),
        json={"email": a.email, "password": a.password},
        timeout=10,
    )
    assert login.status_code == 200

    new_password = "Brand-new!2026"
    r = a.post(
        url_for("auth-handler", base_url, action="change-password"),
        json={"current_password": a.password, "new_password": new_password},
    )
    assert r.status_code == 200

    # Old parallel session must be invalidated.
    r_other = other.get(url_for("patient-handler", base_url), timeout=10)
    assert r_other.status_code == 401

    # The session that performed the change is re-issued and stays valid.
    r_self = a.get(url_for("patient-handler", base_url))
    assert r_self.status_code == 200

    # Login with the new password works; old password no longer.
    fresh = requests.Session()
    bad = fresh.post(
        url_for("auth-handler", base_url, action="login"),
        json={"email": a.email, "password": a.password},
        timeout=10,
    )
    assert bad.status_code == 401
    good = fresh.post(
        url_for("auth-handler", base_url, action="login"),
        json={"email": a.email, "password": new_password},
        timeout=10,
    )
    assert good.status_code == 200

    # Tear down using the new password's fresh session.
    fresh.delete(url_for("auth-handler", base_url, action="user"), timeout=10)


def test_delete_user_kills_session(base_url):
    a = register_user(base_url)
    r = a.delete(url_for("auth-handler", base_url, action="user"))
    assert r.status_code == 204

    r2 = a.get(url_for("patient-handler", base_url))
    assert r2.status_code == 401
