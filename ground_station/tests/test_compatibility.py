#!/usr/bin/env python3
"""
CircuitPython Compatibility Test Script

This script verifies that the ground station code works both:
1. With Pydantic (CPython/Raspberry Pi mode)
2. Without Pydantic (CircuitPython/Pico mode)

Usage:
    python3 test_compatibility.py
"""

import sys
import os
import builtins
import traceback
import importlib

# Add parent directory to path to import ground_station modules
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Try to import select (not available on all platforms)
try:
    import select
    HAS_SELECT_MODULE = True
except ImportError:
    HAS_SELECT_MODULE = False


def get_models_module():
    """
    Import and return the models module.
    This function exists to centralize module imports for testing.
    """
    import models
    return models

def test_with_pydantic():
    """Test models with Pydantic enabled (Raspberry Pi mode)"""
    print("=" * 60)
    print("TEST 1: Raspberry Pi Mode (WITH Pydantic)")
    print("=" * 60)

    try:
        models = get_models_module()

        assert models.USE_PYDANTIC == True, "Expected Pydantic to be enabled"
        assert models.IS_CIRCUITPYTHON == False, "Expected CPython, not CircuitPython"
        print("✓ Platform detection: USE_PYDANTIC=True, IS_CIRCUITPYTHON=False")

        # Test instance creation
        q = models.ADCSQuaternion(q0=1.0, q1=0.0, q2=0.0, q3=0.0)
        assert abs(q.magnitude - 1.0) < 0.01, f"Quaternion magnitude incorrect: {q.magnitude}"
        print(f"✓ ADCSQuaternion: magnitude={q.magnitude:.2f}")

        beacon = models.BeaconData(state_name='test_state')
        assert beacon.state_name == 'test_state'
        print(f"✓ BeaconData: state={beacon.state_name}")

        stats = models.BeaconStats(reboot_counter=5, battery_voltage=3700, device_status=0x88)
        assert stats.reboot_counter == 5
        assert 'adcs_valid' in stats.device_status_flags
        print(f"✓ BeaconStats: reboots={stats.reboot_counter}, flags={stats.device_status_flags}")

        pkt = models.Packet(dst=255, src=0, data=b'test')
        assert pkt.dst == 255
        assert len(pkt.data) == 4
        print(f"✓ Packet: dst={pkt.dst}, data_len={len(pkt.data)}")

        print("\n✅ All tests PASSED with Pydantic!\n")
        return True

    except Exception as e:
        print(f"\n✗ FAILED: {e}\n")
        traceback.print_exc()
        return False


def test_without_pydantic():
    """Test models without Pydantic (CircuitPython mode)"""
    print("=" * 60)
    print("TEST 2: CircuitPython Mode (WITHOUT Pydantic)")
    print("=" * 60)

    # Mock pydantic not being available
    original_import = builtins.__import__

    def mock_import(name, *args, **kwargs):
        if name == 'pydantic' or name.startswith('pydantic.'):
            raise ImportError('Simulating CircuitPython - no pydantic')
        return original_import(name, *args, **kwargs)

    try:
        # Remove models from cache first
        if 'models' in sys.modules:
            del sys.modules['models']

        # Apply mock
        builtins.__import__ = mock_import

        # Import models module with mocked pydantic import
        models_reloaded = get_models_module()

        assert models_reloaded.USE_PYDANTIC == False, "Expected Pydantic to be disabled"
        print("✓ Platform detection: USE_PYDANTIC=False")

        # Test instance creation
        q = models_reloaded.ADCSQuaternion(q0=1.0, q1=0.0, q2=0.0, q3=0.0)
        assert abs(q.magnitude - 1.0) < 0.01
        print(f"✓ ADCSQuaternion: magnitude={q.magnitude:.2f}")

        beacon = models_reloaded.BeaconData(state_name='test_state')
        assert beacon.state_name == 'test_state'
        print(f"✓ BeaconData: state={beacon.state_name}")

        stats = models_reloaded.BeaconStats(reboot_counter=5, battery_voltage=3700, device_status=0x88)
        assert stats.reboot_counter == 5
        assert 'adcs_valid' in stats.device_status_flags
        print(f"✓ BeaconStats: reboots={stats.reboot_counter}, flags={stats.device_status_flags}")

        pkt = models_reloaded.Packet(dst=255, src=0, data=b'test')
        assert pkt.dst == 255
        assert len(pkt.data) == 4

        # Test dict() method (Pydantic compatibility)
        pkt_dict = pkt.dict()
        assert 'dst' in pkt_dict
        print(f"✓ Packet: dst={pkt.dst}, dict() method works")

        print("\n✅ All tests PASSED without Pydantic (CircuitPython mode)!\n")
        return True

    except Exception as e:
        print(f"\n✗ FAILED: {e}\n")
        traceback.print_exc()
        return False

    finally:
        # Restore original import
        builtins.__import__ = original_import
        # Clean up
        if 'models' in sys.modules:
            del sys.modules['models']


def test_ui_platform_detection():
    """Test UI module platform detection"""
    print("=" * 60)
    print("TEST 3: UI Platform Detection")
    print("=" * 60)

    try:
        # Test platform detection
        IS_CIRCUITPYTHON = sys.implementation.name == 'circuitpython'

        print(f"✓ IS_CIRCUITPYTHON: {IS_CIRCUITPYTHON}")
        print(f"✓ HAS_SELECT: {HAS_SELECT_MODULE}")

        # On CPython, select should be available
        if not IS_CIRCUITPYTHON:
            assert HAS_SELECT_MODULE == True, "select should be available on CPython"
            print("✓ select module available (CPython)")

        print("\n✅ UI platform detection tests PASSED!\n")
        return True

    except Exception as e:
        print(f"\n✗ FAILED: {e}\n")
        traceback.print_exc()
        return False


def main():
    """Run all compatibility tests"""
    print("\n" + "=" * 60)
    print("Ground Station CircuitPython Compatibility Tests")
    print("=" * 60 + "\n")

    results = []

    # Run tests
    results.append(("Pydantic Mode (RPi)", test_with_pydantic()))
    results.append(("CircuitPython Mode", test_without_pydantic()))
    results.append(("UI Platform Detection", test_ui_platform_detection()))

    # Summary
    print("=" * 60)
    print("SUMMARY")
    print("=" * 60)

    for name, passed in results:
        status = "✅ PASS" if passed else "✗ FAIL"
        print(f"{status} - {name}")

    all_passed = all(result for _, result in results)

    if all_passed:
        print("\n🎉 All compatibility tests PASSED!")
        print("The ground station is ready for both RPi and CircuitPython deployment.")
    else:
        print("\n❌ Some tests FAILED. Please review the errors above.")
        sys.exit(1)


if __name__ == "__main__":
    main()
