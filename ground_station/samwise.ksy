meta:
  id: beacon_task
  endian: le
  encoding: UTF-8

seq:
  # State name (null-terminated string)
  - id: name
    type: strz
    encoding: UTF-8

  # Beacon statistics struct
  - id: reboot_counter
    type: u4

  - id: time
    type: u8

  - id: rx_bytes
    type: u4

  - id: rx_packets
    type: u4

  - id: rx_backpressure_drops
    type: u4

  - id: rx_bad_packet_drops
    type: u4

  - id: tx_bytes
    type: u4

  - id: tx_packets
    type: u4

  - id: battery_voltage
    type: u2
  
  - id: battery_current
    type: u2

  # Legacy combined solar data (Panel A for backward compatibility)
  - id: solar_voltage
    type: u2

  - id: solar_current
    type: u2

  # Individual panel data
  - id: panel_A_voltage
    type: u2

  - id: panel_A_current
    type: u2

  - id: panel_B_voltage
    type: u2

  - id: panel_B_current
    type: u2

  - id: device_status
    type: u1

  # ADCS telemetry packet
  - id: adcs_w
    type: f4

  - id: adcs_q0
    type: f4

  - id: adcs_q1
    type: f4

  - id: adcs_q2
    type: f4

  - id: adcs_q3
    type: f4

  - id: adcs_state
    type: s1

  - id: adcs_boot_count
    type: u4

  # Callsign (KC3WNY)
  - id: callsign
    type: strz
    encoding: UTF-8
