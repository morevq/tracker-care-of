CREATE TABLE IF NOT EXISTS patient(
	id_patient SERIAL PRIMARY KEY,
	user_uuid UUID,
	name TEXT NOT NULL,
	birth_date DATE
);

CREATE TABLE IF NOT EXISTS anamnesis(
	id_anamnesis SERIAL PRIMARY KEY,
	description TEXT,
	date TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
	photo_url TEXT DEFAULT NULL,
	id_patient INTEGER NOT NULL REFERENCES patient(id_patient) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS water(
	id_water SERIAL PRIMARY KEY,
	last_water TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
	id_patient INTEGER NOT NULL REFERENCES patient(id_patient) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS water_frequency(
	id_frequency  SERIAL PRIMARY KEY,
	frequency TEXT DEFAULT NULL,
	frequency_measure TEXT DEFAULT NULL,
	id_patient INTEGER NOT NULL REFERENCES patient(id_patient) ON DELETE CASCADE
);

