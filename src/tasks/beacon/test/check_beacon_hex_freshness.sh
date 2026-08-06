#!/bin/bash
# Verifies that the checked-in beacon_packet.hex matches the output
# of the current beacon_test binary.
#
# This test catches format drift between the C serialization and the
# ground station's expected packet format.
set -euo pipefail

BEACON_TEST="$1"
CHECKED_IN_HEX="$2"

# Run the beacon test binary, which writes hex to TEST_UNDECLARED_OUTPUTS_DIR
export TEST_UNDECLARED_OUTPUTS_DIR="${TEST_TMPDIR:-/tmp}"
"$BEACON_TEST"

GENERATED_HEX="${TEST_UNDECLARED_OUTPUTS_DIR}/beacon_packet.hex"

if [[ ! -f "$GENERATED_HEX" ]]; then
    echo "ERROR: beacon_test did not produce beacon_packet.hex"
    exit 1
fi

if ! diff -u "$CHECKED_IN_HEX" "$GENERATED_HEX"; then
    echo ""
    echo "ERROR: Checked-in beacon_packet.hex is stale!"
    echo "The beacon packet format has changed. Run:"
    echo "  ./scripts/update_beacon_test_data.sh"
    echo "to regenerate ground_station/tests/test_data/beacon_packet.hex"
    exit 1
fi

echo "beacon_packet.hex is up to date."
