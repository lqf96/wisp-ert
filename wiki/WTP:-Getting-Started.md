# WTP: Getting Started
This tutorial will help you get started with the WTP API.  
For the design, details and issues of WTP please refer to other wiki articles.

## Create WTP Endpoints
The first step to use WTP is to create WTP endpoints. For the server (computer) side, we create an instance of the [`WTPServer`](https://lqf96.github.io/wisp-ert/server/html/classwtp_1_1server_1_1_w_t_p_server.html) class:

```python
from wtp import WTPServer

# Create WTP server endpoint
server = WTPServer(
    antennas=[1],
    n_tags_per_report=1,
)
```

In the demo code shown above, we create a WTP server endpoint that accepts incoming WTP connections through antenna 1 on the RFID reader and receives 1 tag report each time from the reader.

To start the WTP server, call [`WTPServer.start()`](https://lqf96.github.io/wisp-ert/server/html/classwtp_1_1server_1_1_w_t_p_server.html#a16594dc1435ee887c6ccb8615ab89d32) with the IP address and (optionally) the port of the reader:

```python
# Connect to RFID reader and start
server.start("192.168.1.13")
```

As for the WISP (client) side, we declare the endpoint variable and initialize it with [`wtp_init()`](https://lqf96.github.io/wisp-ert/client/html/endpoint_8h.html#aaf9cee974b6a5732ae8c5bda5aee8716):

```c
wtp_t client;

//Initialize client WTP endpoint
wtp_init(
    &client,
    //EPC data memory
    epc_data_mem,
    //EPC data memory size
    EPC_DATA_MEM_SIZE,
    //Read memory
    read_mem,
    //BlockWrite memory
    blockwrite_mem,
    //Initial sliding window size
    64,
    //Timeout
    10,
    //Buffer size for transmit control
    200,
    //Buffer size for receiving control
    200,
    //Capacity of sending messages
    5,
    //Capacity of receiving messages
    5
);
```

## Hook the RFID Loop
(TODO: Hook the RFID Loop)

## Open and Accept Connection
With the client endpoint we already created, the next step is to connect to the server side. This is done with [`wtp_connect()`](https://lqf96.github.io/wisp-ert/client/html/endpoint_8h.html#a61fef7bc9b6858795e7e67b469ccd94a):

```c
WIO_CALLBACK(on_connected) {
    //Do anything you want after the connection is fully opened
    //...

    return WIO_OK;
}
```

```c
//Add connected event callback
wtp_on_event(&client, WTP_EVENT_OPEN, NULL, on_connected);

//Connect to WTP server
wtp_connect(&client);
```

The demo client code above registers a callback for the connection open event, and then starts to connect to the server side. The event will be triggered when both the upstream and the downstream connection is opened, and the callback will then be invoked.

For the server side, it handles every new incoming WTP connection (or client) through the server `connect` event callback:

```python
@server.on("connect")
def on_connect(connection):
    # Print WISP ID
    print("New client: %d" % connection.wisp_id)
    # Do whatever you want with the connection
    # ...
```

## Send and Receive Data
Now that a bidirectional connection has been open for both sides, it's time to send and receive data between the WISP and the computer. Let's first send some data from the computer to the WISP:

```python
# Send callback
def send_cb(_):
    print("Send succeeded!")

# Send data to WISP
connection.send(b"12345").addCallback(send_cb)
```

Function [`WTPConnection.send()`](https://lqf96.github.io/wisp-ert/server/html/classwtp_1_1connection_1_1_w_t_p_connection.html#a2c318cd6d6351613d3044787c70fb107) takes a bytes-like object and sends its content to the WISP side. Because the sending process is asynchronous, it returns a Twisted Deferred object that will be resolved when the client side reports that the whole message is fully sent.

To receive the message at the WISP side, we use [`wtp_recv()`](https://lqf96.github.io/wisp-ert/client/html/endpoint_8h.html#a31a53e4748fcb58671b94369b78c7934):

```c
//Received bytes counter
uint16_t n_recv_bytes = 0;

WIO_CALLBACK(on_recv) {
    //Received data is wrapped inside a WIO buffer
    wio_buf_t* buf = (wio_buf_t*)result;

    //Update counter
    n_recv_bytes += buf->size;

    return WIO_OK;
}
```

```c
//Receive data from server
wtp_recv(&client, NULL, on_recv);
```

The process is similar when we send data from WISP to computer, except we use [`wtp_send()`](https://lqf96.github.io/wisp-ert/client/html/endpoint_8h.html#a55f0846be36a6bf0cfa6991ad8926d89) at the client side and [`WTPConnection.recv()`](https://lqf96.github.io/wisp-ert/server/html/classwtp_1_1connection_1_1_w_t_p_connection.html#a27fb91e99e0ca310f988a90e250d0e3a) at the server side:

```c
WIO_CALLBACK(on_sent) {
    //Do whatever you want here after the data is sent
    //...

    return WIO_OK;
}
```

```c
//Send data to server
wtp_send(&client, "abcde", 5, NULL, on_sent);
```

```python
# Keeps receiving and printing data from client
def recv_cb(msg_data):
    print("Received %s" % msg_data)
    connection.recv().addCallback(recv_cb)

# Kick start
connection.recv().addCallback(recv_cb)
```
