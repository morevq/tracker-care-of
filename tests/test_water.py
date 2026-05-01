"""Tests for the water routes:
- POST/GET on /api/patients/{patient_id}/water (nested list/create)
- GET/DELETE on /api/water/{id} (single record by id_water)
"""

import pytest
import requests

from conftest import AuthSession, url_for


pytestmark = pytest.mark.integration

LAST_WATER = "2026-04-20"


def _add_water(auth: AuthSession, base_url: str, patient_id: int, last_water: str = LAST_WATER):
    return auth.post(
        url_for("patient-water-handler", base_url, patient_id=patient_id),
        json={"last_water": last_water},
    )


def _list_water(auth: AuthSession, base_url: str, patient_id: int):
    return auth.get(url_for("patient-water-handler", base_url, patient_id=patient_id))


def test_list_requires_auth(base_url, patient_id: int):
    r = requests.get(
        url_for("patient-water-handler", base_url, patient_id=patient_id),
        timeout=10,
    )
    assert r.status_code == 401


def test_list_starts_empty(base_url, auth: AuthSession, patient_id: int):
    r = _list_water(auth, base_url, patient_id)
    assert r.status_code == 200
    assert r.json() == []


def test_post_missing_fields_returns_400(base_url, auth: AuthSession, patient_id: int):
    r = auth.post(
        url_for("patient-water-handler", base_url, patient_id=patient_id),
        json={},
    )
    assert r.status_code == 400


def test_post_other_users_patient_is_forbidden(
    base_url, second_auth: AuthSession, patient_id: int
):
    r = _add_water(second_auth, base_url, patient_id)
    assert r.status_code == 403


def test_post_unknown_patient_returns_404(base_url, auth: AuthSession):
    r = _add_water(auth, base_url, 999_999_999)
    assert r.status_code == 404


def test_create_and_list_water(base_url, auth: AuthSession, patient_id: int):
    create = _add_water(auth, base_url, patient_id)
    assert create.status_code == 201, create.text
    created = create.json()
    assert created["id_patient"] == patient_id
    assert isinstance(created["id_water"], int)

    listing = _list_water(auth, base_url, patient_id)
    assert listing.status_code == 200
    items = listing.json()
    assert len(items) == 1
    item = items[0]
    assert item["id_water"] == created["id_water"]
    assert item["id_patient"] == patient_id
    assert "last_water" in item


def test_get_by_id(base_url, auth: AuthSession, patient_id: int):
    create = _add_water(auth, base_url, patient_id).json()
    id_water = create["id_water"]

    r = auth.get(url_for("water-by-id-handler", base_url, id=id_water))
    assert r.status_code == 200
    body = r.json()
    assert body["id_water"] == id_water
    assert body["id_patient"] == patient_id


def test_get_unknown_id_returns_404(base_url, auth: AuthSession):
    r = auth.get(url_for("water-by-id-handler", base_url, id=999_999_999))
    assert r.status_code == 404


def test_get_other_users_record_is_forbidden(
    base_url, auth: AuthSession, second_auth: AuthSession, patient_id: int
):
    id_water = _add_water(auth, base_url, patient_id).json()["id_water"]
    r = second_auth.get(url_for("water-by-id-handler", base_url, id=id_water))
    assert r.status_code == 403


def test_delete_water(base_url, auth: AuthSession, patient_id: int):
    id_water = _add_water(auth, base_url, patient_id).json()["id_water"]

    r = auth.delete(url_for("water-by-id-handler", base_url, id=id_water))
    assert r.status_code == 204

    r2 = auth.get(url_for("water-by-id-handler", base_url, id=id_water))
    assert r2.status_code == 404


def test_delete_other_users_record_is_forbidden(
    base_url, auth: AuthSession, second_auth: AuthSession, patient_id: int
):
    id_water = _add_water(auth, base_url, patient_id).json()["id_water"]
    r = second_auth.delete(url_for("water-by-id-handler", base_url, id=id_water))
    assert r.status_code == 403


def test_invalid_id(base_url, auth: AuthSession):
    r = auth.get(url_for("water-by-id-handler", base_url, id="abc"))
    assert r.status_code == 400
