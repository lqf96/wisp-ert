# WTP: Design
This article covers the design of the WISP Transmission Protocol.

## Ways to Send Data


## Connection Establishment and Termination
WTP's connection establishment is inspired by TCP's 3-way handshake, but differs a little bit in the actual implementation. In WTP, the uplink and the downlink are opened separatedly to decouple the transmission of one direction from another.

Since the connection establishment is always initiated by the client, the client first sends a WTP open packet to the server through EPC-96 data field. The server then receives the connection open request, opens the uplink and sends back an acknowledgement with sequence number 0. To open the downlink connection, the server sends a WTP open packet back to the client. These two packets will be sent in a single BlockWrite as a measure to improve efficiency. The client opens the downlink once it receives the open request, and then it sends an acknowledgement with sequence number 0 back to the server.

## Sending Data on Downlink
Before a message can be transmitted from the computer to the WISP, it needs to be fragmented because in many situations the message itself is too big to be included inside a single BlockWrite. Besides, BlockWrite is the only way to send data to WISP, so control packets (like WTP open, close and acknowledgement packets) will need to share spaces with the data packets, which further complicates the problem.

To schedule the control packets and message data to send, WTP uses the following mechanism: all control packets and messages are put in queues pending for transmission. When the previous AccessSpec for a WISP finished, the WTP server-side code begins constructing the content of a new BlockWrite by fetching and poping packets from the control packets queue. If the control packets queue gets empty, then the remaining space inside the BlockWrite can be used to send message data. The WTP server-side code first looks for the oldest message fragment that times out and needs retransmission, and then wraps the content of the fragment inside a WTP data packet and appends it to the BlockWrite data. If no such fragment is found, then a new message fragment is made from the message that is currently being sent.

Some people might wonder why the priority of control packets is always higher than data packets. This is because the acknowledgement packet should be sent back to the sender of the message fragment as soon as possible to avoid timeout and retranmission.

## Sending Data on Uplink

## Out-of-Order Delivery

## Acknowledge and Retransmission


## OpSpec Size Control
