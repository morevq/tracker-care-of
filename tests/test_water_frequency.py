"""Tests for /api/patients/{patient_id}/water-frequency."""

import pytest
import requests

from conftest import AuthSession, url_for


pytestmark = pytest.mark.integration


def _url(base_url: str, patient_id: int) -> str:
    return url_for("water-frequency-handler", base_url, patient_id=patient_id)


def test_get_requires_auth(base_url, patient_id: int):
    r = requests.get(_url(base_url, patient_id), timeout=10)
    assert r.status_code == 401


def test_get_unset_returns_404(base_url, auth: AuthSession, patient_id: int):
    r = auth.get(_url(base_url, patient_id))
    assert r.status_code == 404


def test_other_users_patient_is_forbidden(
    base_url, auth: AuthSession, second_auth: AuthSession, patient_id: int
):
    r = second_auth.get(_url(base_url, patient_id))
    assert r.status_code == 403


def test_unknown_patient_returns_404(base_url, auth: AuthSession):
    r = auth.get(_url(base_url, 999_999_999))
    assert r.status_code == 404


def test_put_creates_then_get_returns_value(base_url, auth: AuthSession, patient_id: int):
    put = auth.put(
        _url(base_url, patient_id),
        json={"frequency": 3, "frequency_measure": "days"},
    )
    assert put.status_code == 200
    body = put.json()
    assert body == {"frequency": 3, "frequency_measure": "days"}

    get = auth.get(_url(base_url, patient_id))
    assert get.status_code == 200
    assert get.json() == {"frequency": 3, "frequency_measure": "days"}


def test_put_is_upsert(base_url, auth: AuthSession, patient_id: int):
    auth.put(
        _url(base_url, patient_id),
        json={"frequency": 3, "frequency_measure": "days"},
    )
    second = auth.put(
        _url(base_url, patient_id),
        json={"frequency": 5, "frequency_measure": "weeks"},
    )
    assert second.status_code == 200

    get = auth.get(_url(base_url, patient_id))
    assert get.json() == {"frequency": 5, "frequency_measure": "weeks"}


def test_put_validates_frequency_positive(base_url, auth: AuthSession, patient_id: int):
    r = auth.put(
        _url(base_url, patient_id),
        json={"frequency": 0, "frequency_measure": "days"},
    )
    assert r.status_code == 400


def test_put_validates_measure_enum(base_url, auth: AuthSession, patient_id: int):
    r = auth.put(
        _url(base_url, patient_id),
        json={"frequency": 1, "frequency_measure": "months"},
    )
    assert r.status_code == 400


def test_put_missing_fields_returns_400(base_url, auth: AuthSession, patient_id: int):
    r = auth.put(
        _url(base_url, patient_id),
        json={"frequency": 1},
    )
    assert r.status_code == 400


def test_delete_removes_setting(base_url, auth: AuthSession, patient_id: int):
    auth.put(
        _url(base_url, patient_id),
        json={"frequency": 2, "frequency_measure": "hours"},
    )
    d = auth.delete(_url(base_url, patient_id))
    assert d.status_code == 204

    get = auth.get(_url(base_url, patient_id))
    assert get.status_code == 404


def test_delete_other_users_patient_is_forbidden(
    base_url, auth: AuthSession, second_auth: AuthSession, patient_id: int
):
    r = second_auth.delete(_url(base_url, patient_id))
    assert r.status_code == 403
