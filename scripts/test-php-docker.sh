#!/bin/bash
set -eu

# Test PHP MapScript build locally using Docker (mirrors CI).
# Usage:
#   ./scripts/test-php-docker.sh         # defaults to PHP 8.4
#   ./scripts/test-php-docker.sh 8.5     # test with PHP 8.5

PHP_VERSION="${1:-8.4}"
WORK_DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "=== Testing PHP MapScript with PHP ${PHP_VERSION} ==="
echo "Source: ${WORK_DIR}"

docker run --rm \
    -e WORK_DIR="${WORK_DIR}" \
    -e PHP_VERSION="${PHP_VERSION}" \
    -v "${WORK_DIR}:${WORK_DIR}" \
    ubuntu:24.04 \
    "${WORK_DIR}/scripts/build-mapscript-php.sh"
