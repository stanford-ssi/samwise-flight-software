#!/bin/bash
# Regenerates the beacon packet hex artifact from the C test and copies it
# to the ground station test data directory.
#
# Usage: ./scripts/update_beacon_test_data.sh
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="${REPO_ROOT}/ground_station/tests/test_data/beacon_packet.hex"
TESTLOGS_HEX="${REPO_ROOT}/bazel-testlogs/src/tasks/beacon/beacon_test/test.outputs/beacon_packet.hex"

echo "Running beacon test..."
bazel test //src/tasks/beacon:beacon_test --test_output=errors

if [[ ! -f "$TESTLOGS_HEX" ]]; then
    echo "ERROR: beacon_packet.hex not found at ${TESTLOGS_HEX}"
    echo "The beacon test may not have produced the hex artifact."
    exit 1
fi

mkdir -p "$(dirname "$DEST")"
cp "$TESTLOGS_HEX" "$DEST"
echo "Updated ${DEST}"
