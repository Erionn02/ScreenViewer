set -e

docker network create --subnet=172.26.10.0/24 test-network || true
docker rm -f test-database
docker run --net test-network --ip 172.26.10.10 -d --rm -p 54325:5432 -v ./database_schema.sql:/docker-entrypoint-initdb.d/database_schema.sql \
 -e POSTGRES_USER="test" -e POSTGRES_DB="screen-viewer" -e POSTGRES_PASSWORD="test" --name "test-database" postgres:14.5

