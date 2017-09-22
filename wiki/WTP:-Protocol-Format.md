# WTP: Protocol Format
This article describes WTP protocol format.

## Ways to Send Data
* BlockWrite: Used for downlink. Xor checksum appended after each packet as failed or partial transmission could easily happen.
* EPC-96: Used for uplink. Initiated by WISP. Used for sending control packets.
* Read: Used for uplink. Initiated by computer. Used for sending data packets.

## WTP Packet Types
In WTP packets can be divided into two categories: control packets and data packets. As shown in the following list, Begin Message Packet and Continue Message Packet are data packets, while all other packets are control packets.
* `0x00`: End of Packets Packet  
Indicates there aren't any packets after this packet.
* `0x01`: Open Connection Packet  
Sent by WISP to open upstream connection and by computer to open downstream connection.
* `0x02`: Close Connection Packet  
Sent by WISP to close upstream connection and by computer to open downstream connection.
* `0x03`: Acknowledgement Packet  
Used for acknowledging message data sent from the other side.
* `0x04`: Begin Message Packet  
Used for sending the first fragment of a message.
* `0x05`: Continue Message Packet  
Used for sending subsequent message fragments.
* `0x06`: Request Uplink Packet  
Sent by WISP to request Read to send packet data.
* `0x07`: Set Parameter Packet  
Used to set connection parameters on the remote endpoint.

## WTP Parameters
In WTP some configurations need to be synchronized between two endpoints. These configurations are represented by WTP parameters and can be set on the remote endpoint by sending set parameter packet.
* `0x00`: Sliding window size  
(TODO: Parameter introduction)
* `0x01`: Desired Read size  
(TODO: Parameter introduction)

## WTP Packet Formats
* `0x00`: End of Packets Packet
* `0x01`: Open Connection Packet
* `0x02`: Close Connection Packet
* `0x03`: Acknowledgement Packet
  - 2-byte sequence number
* `0x04`: Begin Message Packet
  - 2-byte message size
  - 2-byte sequence number
  - 1-byte packet payload data size
  - Payload data
* `0x05`: Continue Message Packet
  - 2-byte sequence number
  - 1-byte packet payload data size
  - Payload data
* `0x06`: Request Uplink Packet
  - 1-byte Number of Read requested
  - 1-byte Size of each Read
* `0x07`: Set Parameter Packet
  - 1-byte parameter type
  - Parameter value
