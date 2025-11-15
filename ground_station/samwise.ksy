meta:
  id: beacon_task
  endian: be
  encoding: UTF-8

seq:
  - id: name
    type: strz
    encoding: UTF-8

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

  - id: solar_voltage
    type: u2

  - id: solar_current
    type: u2

  - id: device_status
    type: u1