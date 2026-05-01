-- Required for PUT /api/patients/{id}/water-frequency upsert (one row per patient).
DELETE FROM water_frequency wf
WHERE wf.id_frequency NOT IN (
    SELECT MAX(id_frequency) FROM water_frequency GROUP BY id_patient
);

ALTER TABLE water_frequency
    ADD CONSTRAINT water_frequency_id_patient_key UNIQUE (id_patient);
