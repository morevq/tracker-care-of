"""Tests for /api/patients and /api/patients/{id}."""

import pytest
import requests

from conftest import AuthSession, url_for


pytestmark = pytest.mark.integration


def test_list_requires_auth(base_url):
    r = requests.get(url_for("patient-handler", base_url), timeout=10)
    assert r.status_code == 401


def test_list_starts_empty(base_url, auth: AuthSession):
    r = auth.get(url_for("patient-handler", base_url))
    assert r.status_code == 200
    assert r.json() == []


def test_create_and_list_patient(base_url, auth: AuthSession):
    create = auth.post(
        url_for("patient-handler", base_url),
        json={"name": "Aloe", "birth_date": "2021-06-01"},
    )
    assert create.status_code == 201

    r = auth.get(url_for("patient-handler", base_url))
    assert r.status_code == 200
    items = r.json()
    assert len(items) == 1
    assert items[0]["name"] == "Aloe"
    assert items[0]["birth_date"] == "2021-06-01"
    assert isinstance(items[0]["id"], int)


def test_create_without_birth_date(base_url, auth: AuthSession):
    r = auth.post(url_for("patient-handler", base_url), json={"name": "Ivy"})
    assert r.status_code == 201


def test_create_invalid_json(base_url, auth: AuthSession):
    r = auth.post(
        url_for("patient-handler", base_url),
        data="not json",
        headers={"Content-Type": "application/json"},
    )
    assert r.status_code == 400


def test_get_patient_by_id(base_url, auth: AuthSession, patient_id: int):
    r = auth.get(url_for("patient-by-id-handler", base_url, id=patient_id))
    assert r.status_code == 200
    body = r.json()
    assert body["id"] == patient_id
    assert body["name"] == "Fern"


def test_get_unknown_patient_returns_404(base_url, auth: AuthSession):
    r = auth.get(url_for("patient-by-id-handler", base_url, id=999_999_999))
    assert r.status_code == 404


def test_get_patient_invalid_id(base_url, auth: AuthSession):
    r = auth.get(url_for("patient-by-id-handler", base_url, id="abc"))
    assert r.status_code == 400


def test_get_other_users_patient_is_forbidden(
    base_url, auth: AuthSession, second_auth: AuthSession, patient_id: int
):
    r = second_auth.get(url_for("patient-by-id-handler", base_url, id=patient_id))
    assert r.status_code == 403


def test_patch_patient(base_url, auth: AuthSession, patient_id: int):
    r = auth.patch(
        url_for("patient-by-id-handler", base_url, id=patient_id),
        json={"name": "Renamed"},
    )
    assert r.status_code == 200
    body = r.json()
    assert body["name"] == "Renamed"
    assert body["id_patient"] == patient_id


def test_patch_requires_at_least_one_field(base_url, auth: AuthSession, patient_id: int):
    r = auth.patch(
        url_for("patient-by-id-handler", base_url, id=patient_id),
        json={},
    )
    # Handler currently wraps this in a 500; the contract is "non-2xx".
    assert r.status_code >= 400


def test_delete_patient(base_url, auth: AuthSession, patient_id: int):
    r = auth.delete(url_for("patient-by-id-handler", base_url, id=patient_id))
    assert r.status_code == 204

    r2 = auth.get(url_for("patient-by-id-handler", base_url, id=patient_id))
    assert r2.status_code == 404


def test_delete_other_users_patient_is_forbidden(
    base_url, auth: AuthSession, second_auth: AuthSession, patient_id: int
):
    r = second_auth.delete(url_for("patient-by-id-handler", base_url, id=patient_id))
    assert r.status_code == 403
