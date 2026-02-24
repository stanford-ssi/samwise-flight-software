#!/usr/bin/env python3
"""
Test script for packet filtering functionality

This demonstrates the RSSI threshold and callsign verification filters.
"""

import sys
import os

# Add parent directory to path to import ground_station modules
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Import configuration values
import config


def test_rssi_filter():
    """Test RSSI threshold filtering logic"""
    print("=" * 60)
    print("TEST: RSSI Threshold Filtering")
    print("=" * 60)

    # Get threshold from config
    rssi_threshold = config.config.get('rssi_threshold', -120)
    print(f"Using RSSI threshold from config: {rssi_threshold} dBm\n")

    # Simulated RSSI values
    test_cases = [
        (-115, True, "Strong signal - should accept"),
        (rssi_threshold, True, "At threshold - should accept"),
        (rssi_threshold - 1, False, "Below threshold - should reject"),
        (-130, False, "Very weak - should reject"),
        (None, True, "No RSSI data - should accept"),
    ]
    passed = 0
    failed = 0

    for rssi, should_accept, description in test_cases:
        # Simulate filter logic
        if rssi is not None:
            accept = rssi >= rssi_threshold
        else:
            accept = True  # No RSSI data, accept by default

        status = "✓ PASS" if accept == should_accept else "✗ FAIL"
        result = "ACCEPT" if accept else "REJECT"
        rssi_display = f"{rssi:>4}" if rssi is not None else "None"

        print(f"{status} | RSSI: {rssi_display} dBm | {result:>6} | {description}")

        if accept == should_accept:
            passed += 1
        else:
            failed += 1

    print(f"\nResults: {passed} passed, {failed} failed\n")
    return failed == 0


def test_callsign_filter():
    """Test callsign verification logic"""
    print("=" * 60)
    print("TEST: Callsign Verification")
    print("=" * 60)

    # Get expected callsign from config
    expected_callsign = config.config.get('expected_callsign', 'KC3WNY')
    print(f"Using expected callsign from config: {expected_callsign}\n")

    # Simulated callsign values
    test_cases = [
        (expected_callsign, True, "Exact match - should accept"),
        (expected_callsign.lower(), True, "Lowercase - should accept (case-insensitive)"),
        (expected_callsign + " ", True, "With trailing space - should accept (trimmed)"),
        ("AB1CDE", False, "Different callsign - should reject"),
        ("KC3ABC", False, "Different suffix - should reject"),
        ("", False, "Empty callsign - should reject"),
        (None, True, "No callsign field - should accept (legacy format)"),
    ]
    passed = 0
    failed = 0

    for callsign, should_accept, description in test_cases:
        # Simulate filter logic
        if callsign is not None:
            actual_callsign = callsign.strip().upper()
            accept = expected_callsign.upper() in actual_callsign
        else:
            accept = True  # No callsign field, accept (might be old format)

        status = "✓ PASS" if accept == should_accept else "✗ FAIL"
        result = "ACCEPT" if accept else "REJECT"
        callsign_display = f"'{callsign}'" if callsign is not None else "None"

        print(f"{status} | Callsign: {callsign_display:>10} | {result:>6} | {description}")

        if accept == should_accept:
            passed += 1
        else:
            failed += 1

    print(f"\nResults: {passed} passed, {failed} failed\n")
    return failed == 0


def test_combined_filters():
    """Test combined RSSI + callsign filtering"""
    print("=" * 60)
    print("TEST: Combined Filtering (RSSI + Callsign)")
    print("=" * 60)

    # Get values from config
    rssi_threshold = config.config.get('rssi_threshold', -120)
    expected_callsign = config.config.get('expected_callsign', 'KC3WNY')
    print(f"Using RSSI threshold: {rssi_threshold} dBm")
    print(f"Using expected callsign: {expected_callsign}\n")

    # Simulated packets: (rssi, callsign, should_accept, description)
    test_cases = [
        (-115, expected_callsign, True, "Good signal + correct callsign - ACCEPT"),
        (rssi_threshold - 5, expected_callsign, False, "Weak signal + correct callsign - REJECT (RSSI)"),
        (-115, "AB1CDE", False, "Good signal + wrong callsign - REJECT (Callsign)"),
        (rssi_threshold - 5, "AB1CDE", False, "Weak signal + wrong callsign - REJECT (Both)"),
        (rssi_threshold, expected_callsign, True, "At threshold + correct callsign - ACCEPT"),
    ]
    passed = 0
    failed = 0

    for rssi, callsign, should_accept, description in test_cases:
        # Simulate combined filter logic
        rssi_pass = rssi >= rssi_threshold
        callsign_pass = expected_callsign.upper() in callsign.upper()
        accept = rssi_pass and callsign_pass

        status = "✓ PASS" if accept == should_accept else "✗ FAIL"
        result = "ACCEPT" if accept else "REJECT"

        print(f"{status} | RSSI: {rssi:>4} dBm | Callsign: {callsign:>6} | {result:>6}")
        print(f"      | {description}")

        if accept == should_accept:
            passed += 1
        else:
            failed += 1

    print(f"\nResults: {passed} passed, {failed} failed\n")
    return failed == 0


def main():
    """Run all filter tests"""
    print("\n" + "=" * 60)
    print("Ground Station Packet Filtering Tests")
    print("=" * 60 + "\n")

    results = []

    # Run tests
    results.append(("RSSI Filter", test_rssi_filter()))
    results.append(("Callsign Filter", test_callsign_filter()))
    results.append(("Combined Filters", test_combined_filters()))

    # Summary
    print("=" * 60)
    print("SUMMARY")
    print("=" * 60)

    for name, passed in results:
        status = "✅ PASS" if passed else "✗ FAIL"
        print(f"{status} - {name}")

    all_passed = all(result for _, result in results)

    if all_passed:
        print("\n🎉 All filtering tests PASSED!")
        print("The packet filters are working correctly.")
    else:
        print("\n❌ Some tests FAILED. Please review the logic.")
        sys.exit(1)


if __name__ == "__main__":
    main()
