# WTP: Design
This article covers the design of the WISP Transmission Protocol.

## Connection Establishment and Termination
WTP's connection establishment is inspired by TCP's 3-way handshake, but differs a little bit in the actual implementation. In WTP, the uplink and the downlink are opened separatedly to decouple the transmission of one direction from another.

Since the connection establishment is always initiated by the client, the client first sends a WTP open packet to the server through EPC-96 data field. The server then receives the connection open request, opens the uplink and sends back an acknowledgement with sequence number 0. To open the downlink connection, the server sends a WTP open packet back to the client. These two packets will be sent in a single BlockWrite as a measure to improve efficiency. The client opens the downlink once it receives the open request, and then it sends an acknowledgement with sequence number 0 back to the server.

## Sending Data on Downlink
Before a message can be transmitted from the computer to the WISP, it needs to be fragmented because in many situations the message itself is too big to be included inside a single BlockWrite. Besides, BlockWrite is the only way to send data to WISP, so control packets (like WTP open, close and acknowledgement packets) will need to share spaces with the data packets, which further complicates the problem.

To schedule the control packets and message data to send, WTP uses the following mechanism: all control packets and messages are put in queues pending for transmission. When the previous AccessSpec for a WISP finished, the WTP server-side code begins constructing the content of a new BlockWrite by fetching and poping packets from the control packets queue. If the control packets queue gets empty, then the remaining space inside the BlockWrite can be used to send message data. The WTP server-side code first looks for the oldest message fragment that times out and needs retransmission, and then wraps the content of the fragment inside a WTP data packet and appends it to the BlockWrite data. If no such fragment is found, then a new message fragment is made from the message that is currently being sent.

Some people might wonder why the priority of control packets is always higher than data packets. This is because the acknowledgement packet should be sent back to the sender of the message fragment as soon as possible to avoid timeout and retransmission.

## Sending Data on Uplink
For uplink, there are two ways to send data to the computer. Using the EPC-96 data field, the WISP can send data as it wishs, but since the EPC-96 data field is small (10 bytes), it is only used for control packets. Additionally, the WISP can send large chunks of data (maximum 32 bytes) by responding to the Read command, which is suitable for the data packets. The downside of this method is that a Read operation must be initiated by the computer, not the WISP.

To solve the problem, the WISP go through the process of "requesting uplink" before sending the message data to the server. The client-side WTP code estimates the count and the size of Read it needs to send the message. It then sends a "Requesting Uplink" packet to the server with the count and the size of Read operations. When the server-side WTP program receives the packet, it carries out the Read operations on behalf of the WISP, through which the WISP will be able to send message fragments to the server.

The message fragmentation process of the uplink is similar to that of the downlink, and hence we will not discuss it here again.

## Out-of-Order Delivery & Acknowledgement
WTP supports out-of-order delivery by using a sliding window. When a WTP endpoint receives a message fragment from the other, it checks if the fragment's byte range falls within the sliding window. If part of the fragment is out of the window, it is considered invalid and gets dropped.

The fragment is then inserted to a received fragments linked list, which is ordered by the sequence number of the fragments. After that, the WTP client-side code walks through the linked list from the beginning to the end, and trys to fetch as many consecutive fragments as possible. If enough fragments are gathered to assemble a message, the WTP client-side code notifies the upper-level programs with the message received.

Finally, the client calculates the sequence number of the ending byte of the last fragment fetched, and send this sequence number in an acknowledgement packet to the other side.

## Retransmission
For both the uplink and the downlink, when a fragment is about to be transmitted, an associated timer will be enabled to trigger retransmission in case of a timeout. When a WTP endpoint receives an acknowledgement packet, all fragments whose sequence number is smaller will be destroyed and their associated timers will be disabled.

When a fragment times out, it will be retranmitted using the sending machanisms described above. In WTP, existing fragments have higher priorities than making new fragments, so the WTP library will temporarily suspend the transmission of new message data, until all existing fragments are successfully retransmitted.

## OpSpec Size Control
Because the complexity and instability of wireless environments, Read and BlockWrite larger data sometimes fail. To maximum the transmission efficiency in such an environment, the WTP server-side library keeps track on all AccessSpecs and their results, and adjust the maximum size of Read and BlockWrite respectively.

Currently, the WTP server-side code implements a naive, demonstration "OpSpec Size Control". When a Read or a BlockWrite operation failed, the maximum size is reduced by 2, causing the WTP connection to be throttled. When a Read or a BlockWrite succeeds, and the size of the operation is the maximum allowed Read/BlockWrite size, it means the current maximum size is good and can be improved. The maximum OpSpec size is thus increased by 2.
