#!/bin/bash
set -e

echo "Waiting for database to be ready..."
until pg_isready -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME"; do
  sleep 2
done

echo "Database is ready. Applying migrations..."

PGPASSWORD=$DB_PASSWORD psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" <<-EOSQL
  CREATE TABLE IF NOT EXISTS schema_migrations (
    version VARCHAR(255) PRIMARY KEY,
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
  );
EOSQL

for migration in /docker-entrypoint-initdb.d/*.sql; do
  if [ -f "$migration" ]; then
    version=$(basename "$migration" .sql)

    applied=$(PGPASSWORD=$DB_PASSWORD psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -tAc "SELECT COUNT(*) FROM schema_migrations WHERE version='$version'")

    if [ "$applied" -eq "0" ]; then
      echo "Applying migration: $version"
      PGPASSWORD=$DB_PASSWORD psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -v ON_ERROR_STOP=1 -f "$migration"

      PGPASSWORD=$DB_PASSWORD psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "INSERT INTO schema_migrations (version) VALUES ('$version')"
      echo "Migration $version applied successfully"
    else
      echo "Migration $version already applied, skipping"
    fi
  fi
done

echo "All migrations applied successfully!"
