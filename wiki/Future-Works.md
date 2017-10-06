# Future Works
This article discusses some future works that improves WISP Extended Runtime.

## WTP
### Compression
To improve the efficiency of sending huge messages on top WTP, compression should be incoperated into the WTP. Because the data being sent on WTP is uncertain, we will have to use an adaptive compression algorithm instead of an ordinary one that works on fixed-size data.

Among many adaptive compression algorithms, the FGK algorithm seems like a good candidate. The FGK algorithm implements adaptive Huffman encoding in a relatively simple way. It encodes and decodes the data using Huffman encoding like its original variant. But the FGK algorithm will constantly adjust the Huffman tree and swap the nodes based on the frequency of symbols. Besides, FGK only assigns codes to existing symbols, and when a new symbol appears, a new code is assigned for that symbol.

The major challenge to implement FGK algorithm is that it can take a lot of spaces to store the nodes inside the Huffman tree. Thus, the structure of the node has to be carefully designed to avoid excessive memory usage.

### Encryption
Encryption serves as an important mechanism to ensure the confidentiality, credibility and integrity of data, and so should be implemented in WTP in the future.

A typical encrypted transport makes use of asymmetrical and symmetrical cryptography. For symmetrical encryption on the WISP, we can probably rely on the MSP430 AES hardware support for less code and faster execution. For asymmetrical encryption, we may need to use ECC and implement all the code by ourselves. In that case, [this paper](https://wisp.wikispaces.com/file/view/Pendl_RFIDsec_2011.pdf/315201010/Pendl_RFIDsec_2011.pdf) seems like a good reference.

Talking about the design of the protocol, the encryption handshake process should happen on top of a reliable transport (e.g. WTP itself). To make the handshake as easy and fast as possible, we can borrow the ideas of a standard TLS 1.3 handshake. The client first sends the type of negotiation and negotiation parameters to the server. For example, if the client wants to negotiate with the server using Diffie-Hellman key exchange, it should sends the DH parameters to the server. When the server receives the negotiation parameters from clients, it sends back the server-side negotiation parameters encrypted with the server's public key. The server can optionally send the public key in case the client does not know it before. Now that both side get the negotiation parameters from each other, they can derive a key from these parameters and communicate with each other using symmetric encryption.

There are two ways to implement encryption on WTP. One is to only encrypt the message data, but keeps the control packets in clear text. The advantage of the approach is that the underlying WTP needs little modification because the encryption happens purely on top of the reliable transport. The downside of the approach is that the control packets can still be sniffed or manipulated. A middleman is able to send a WTP close packet to both sides to close the connection even if it's encrypted in this case. The other way is to encrypt all the data being sent. While this is definitely safer than the previous approach, it does require some modifications to the existing protocol and the encryption machanism has to be adapted to work with unreliable transport.

## WISP Extended Runtime
### Firmware Update
The WISP Extended Runtime should support OTA firmware update. WISP firmware update has already been done before in [Wisent](https://arxiv.org/pdf/1512.04602.pdf) and [Stork](http://www.es.ewi.tudelft.nl/papers/2017-Aantjes-INFOCOM.pdf).
