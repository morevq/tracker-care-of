"""Tests for /api/anamnesis (POST) and /api/anamnesis/{id}.

Note: GET /api/anamnesis/{id} treats {id} as a *patient* id and returns the
list of anamnesis records for that patient. PATCH/DELETE treat {id} as the
anamnesis record id. This matches the current handler implementation.
"""

import pytest
import requests

from conftest import AuthSession, url_for


pytestmark = pytest.mark.integration


def _create_anamnesis(auth: AuthSession, base_url: str, patient_id: int, **extra):
    payload = {"patient_id": patient_id, "description": "looks healthy"}
    payload.update(extra)
    return auth.post(url_for("anamnesis-handler", base_url), json=payload)


def test_post_requires_auth(base_url, patient_id: int):
    r = requests.post(
        url_for("anamnesis-handler", base_url),
        json={"patient_id": patient_id, "description": "x"},
        timeout=10,
    )
    assert r.status_code == 401


def test_post_create(base_url, auth: AuthSession, patient_id: int):
    r = _create_anamnesis(auth, base_url, patient_id, photo_url="http://example.com/a.png")
    assert r.status_code == 201


def test_post_unknown_patient_returns_404(base_url, auth: AuthSession):
    r = _create_anamnesis(auth, base_url, 999_999_999)
    assert r.status_code == 404


def test_post_other_users_patient_is_forbidden(
    base_url, auth: AuthSession, second_auth: AuthSession, patient_id: int
):
    r = _create_anamnesis(second_auth, base_url, patient_id)
    assert r.status_code == 403


def test_post_only_post_method_allowed(base_url, auth: AuthSession):
    # The handler is registered for POST only — GET must yield 405 or 404.
    r = auth.get(url_for("anamnesis-handler", base_url))
    assert r.status_code in (404, 405)


def test_get_by_patient_id_lists_records(base_url, auth: AuthSession, patient_id: int):
    assert _create_anamnesis(auth, base_url, patient_id, photo_url="u1").status_code == 201
    assert _create_anamnesis(auth, base_url, patient_id).status_code == 201

    r = auth.get(url_for("anamnesis-by-id-handler", base_url, id=patient_id))
    assert r.status_code == 200
    items = r.json()
    assert len(items) == 2
    descriptions = {item["description"] for item in items}
    assert "looks healthy" in descriptions
    for item in items:
        assert "id" in item and "date" in item


def test_get_unknown_patient_returns_404(base_url, auth: AuthSession):
    r = auth.get(url_for("anamnesis-by-id-handler", base_url, id=999_999_999))
    assert r.status_code == 404


def test_get_other_users_patient_is_forbidden(
    base_url, auth: AuthSession, second_auth: AuthSession, patient_id: int
):
    _create_anamnesis(auth, base_url, patient_id)
    r = second_auth.get(url_for("anamnesis-by-id-handler", base_url, id=patient_id))
    assert r.status_code == 403


def test_patch_and_delete_anamnesis(base_url, auth: AuthSession, patient_id: int):
    assert _create_anamnesis(auth, base_url, patient_id).status_code == 201

    listing = auth.get(url_for("anamnesis-by-id-handler", base_url, id=patient_id))
    assert listing.status_code == 200
    record = listing.json()[0]
    record_id = record["id"]

    patch = auth.patch(
        url_for("anamnesis-by-id-handler", base_url, id=record_id),
        json={"description": "updated", "photo_url": "http://example.com/b.png"},
    )
    assert patch.status_code == 200
    body = patch.json()
    assert body["description"] == "updated"
    assert body["photo_url"] == "http://example.com/b.png"

    delete = auth.delete(url_for("anamnesis-by-id-handler", base_url, id=record_id))
    assert delete.status_code == 204


def test_patch_unknown_anamnesis_returns_404(base_url, auth: AuthSession):
    r = auth.patch(
        url_for("anamnesis-by-id-handler", base_url, id=999_999_999),
        json={"description": "x"},
    )
    assert r.status_code == 404


def test_patch_requires_at_least_one_field(base_url, auth: AuthSession, patient_id: int):
    _create_anamnesis(auth, base_url, patient_id)
    record_id = auth.get(
        url_for("anamnesis-by-id-handler", base_url, id=patient_id)
    ).json()[0]["id"]

    r = auth.patch(url_for("anamnesis-by-id-handler", base_url, id=record_id), json={})
    assert r.status_code >= 400
