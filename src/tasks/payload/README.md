# Payload Overview
## Turning the Payload On and Off programatically
To turn the payload on, please run the payload_uart.c function called "payload_turn_on". Similarly, to turn off the payload please run the payload_uart.c function called "payload_turn_off".
## Turning the Payload On and Off via ground station commands
To turn the payload on, simply send the command ID "3" to the PiCubed. To turn turn the payload off, simply send the command ID "4" to the PiCbed. These operations will be done via the 433 radio, utilizing the proper packet structure as stated in the command task and the radio task.
