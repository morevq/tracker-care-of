"""Sanity checks on the static_config.yaml — verifies every expected
handler is declared with the expected path and HTTP methods.

These tests do not hit the network; they protect us from accidentally
removing or renaming a handler in static_config.yaml.
"""

import pytest

EXPECTED = {
    "auth-handler":           ("/api/auth/{action}",   {"GET", "POST", "PATCH", "DELETE"}),
    "patient-handler":        ("/api/patients",        {"GET", "POST"}),
    "patient-by-id-handler":  ("/api/patients/{id}",   {"GET", "PATCH", "DELETE"}),
    "anamnesis-handler":      ("/api/anamnesis",       {"POST"}),
    "anamnesis-by-id-handler":("/api/anamnesis/{id}",  {"GET", "PATCH", "DELETE"}),
    "water-handler":          ("/api/water",           {"GET", "POST"}),
    "water-by-id-handler":    ("/api/water/{id}",      {"GET", "DELETE"}),
}


@pytest.mark.parametrize("name,expected", list(EXPECTED.items()))
def test_handler_declared(handlers, name, expected):
    expected_path, expected_methods = expected
    assert name in handlers, f"{name} missing from static_config.yaml"
    spec = handlers[name]
    assert spec.path == expected_path
    assert set(spec.methods) == expected_methods


def test_no_unexpected_handlers(handlers):
    """Catch newly added routes so this list stays authoritative."""
    extra = set(handlers) - set(EXPECTED)
    assert not extra, f"unexpected handler(s) in static_config.yaml: {extra}"
