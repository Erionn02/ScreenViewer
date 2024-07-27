set -e


docker run --rm -p 54325:54325 -v ./database_schema.sql:/docker-entrypoint-initdb.d/database_schema.sql \
 -e POSTGRES_USER="test" -e POSTGRES_DB="screen-viewer" -e POSTGRES_PASSWORD="test" --name "test_database" postgres:14.5

