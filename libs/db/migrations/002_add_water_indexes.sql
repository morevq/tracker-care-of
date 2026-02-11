CREATE INDEX IF NOT EXISTS water_patient_lastwater_idx
ON water (id_patient, last_water DESC);

CREATE INDEX IF NOT EXISTS patient_user_uuid_idx
ON patient (user_uuid);

CREATE INDEX IF NOT EXISTS water_frequency_patient_idx
ON water_frequency (id_patient);
