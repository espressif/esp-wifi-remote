#!/bin/bash

# Check version consistency between header and component file
# Extract version from header file
HEADER_MAJOR=$(grep -E '#define ESP_WIFI_REMOTE_VERSION_MAJOR' components/esp_wifi_remote/include/esp_wifi_remote_version.h | awk '{print $3}')
HEADER_MINOR=$(grep -E '#define ESP_WIFI_REMOTE_VERSION_MINOR' components/esp_wifi_remote/include/esp_wifi_remote_version.h | awk '{print $3}')
HEADER_PATCH=$(grep -E '#define ESP_WIFI_REMOTE_VERSION_PATCH' components/esp_wifi_remote/include/esp_wifi_remote_version.h | awk '{print $3}')
HEADER_VERSION="${HEADER_MAJOR}.${HEADER_MINOR}.${HEADER_PATCH}"

# Extract version from yml file
YML_VERSION=$(grep -E '^version:' components/esp_wifi_remote/idf_component.yml | awk '{print $2}' | tr -d '"')

echo "Header version: ${HEADER_VERSION}"
echo "YML version: ${YML_VERSION}"

if [ "${HEADER_VERSION}" != "${YML_VERSION}" ]; then
  echo "Error: Version mismatch!"
  echo "  Header file version: ${HEADER_VERSION}"
  echo "  Component YML version: ${YML_VERSION}"
  exit 1
else
  echo "✓ Versions match: ${HEADER_VERSION}"
fi
