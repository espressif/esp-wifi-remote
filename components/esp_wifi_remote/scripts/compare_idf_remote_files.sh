#!/usr/bin/env bash

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WIFI_REMOTE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_DIR="$(cd "${WIFI_REMOTE_DIR}/../.." && pwd)"

if [[ -z "${IDF_REMOTE_DIR:-}" ]]; then
    if [[ -n "${IDF_PATH:-}" && -d "${IDF_PATH}/components/esp_wifi/remote" ]]; then
        IDF_REMOTE_DIR="${IDF_PATH}/components/esp_wifi/remote"
    else
        IDF_REMOTE_DIR="${REPO_DIR}/idf/components/esp_wifi/remote"
    fi
fi

# Hand-maintained Wi-Fi remote sources kept in sync with IDF (not generated per-version files).
CONSTANT_FILES=(
    esp_wifi_remote.c
    esp_wifi_remote_net.c
    esp_wifi_remote_net2.c
    include/esp_wifi_remote.h
)

DIFF_OPTS="${DIFF_OPTS:--u -w -B -I Copyright}"

usage() {
    cat <<EOF
Usage: $(basename "$0") [options]

Compare constant Wi-Fi remote sources and headers between IDF and this component.

Options:
  --idf-remote-dir <dir>     IDF remote directory to compare from
                             (default: \$IDF_PATH/components/esp_wifi/remote, or repo idf checkout)
  -h, --help                 Show this help

Environment:
  IDF_REMOTE_DIR             Same as --idf-remote-dir
  DIFF_OPTS                  Options passed to diff (default: -u -w -B -I Copyright)
EOF
}

require_value() {
    if [[ $# -lt 2 || "$2" == -* ]]; then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --idf-remote-dir)
            require_value "$@"
            IDF_REMOTE_DIR="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ ! -d "$IDF_REMOTE_DIR" ]]; then
    echo "IDF remote directory not found: $IDF_REMOTE_DIR" >&2
    exit 2
fi

diff_count=0
missing_count=0
checked_count=0

for rel in "${CONSTANT_FILES[@]}"; do
    idf_file="${IDF_REMOTE_DIR}/${rel}"
    external_file="${WIFI_REMOTE_DIR}/${rel}"

    if [[ ! -f "$idf_file" ]]; then
        echo "Missing IDF file: $rel" >&2
        missing_count=$((missing_count + 1))
        continue
    fi

    if [[ ! -f "$external_file" ]]; then
        echo "Missing external counterpart: $rel" >&2
        missing_count=$((missing_count + 1))
        continue
    fi

    checked_count=$((checked_count + 1))
    if ! diff $DIFF_OPTS "$external_file" "$idf_file"; then
        diff_count=$((diff_count + 1))
    fi
done

echo
echo "Checked files: $checked_count"
echo "Files with differences: $diff_count"
echo "Missing files: $missing_count"

if (( diff_count > 0 || missing_count > 0 )); then
    exit 1
fi
