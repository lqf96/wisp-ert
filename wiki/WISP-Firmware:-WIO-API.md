# WISP Firmware: WIO API
This article introduces the WIO API

## Types
* `wio_status_t`: A `uint8_t` representing the status of an operation. `WIO_OK` (or 0) indicates that an operation succeeded, while other values indicate failure.
* `wio_callback_t`: Alias for function pointer type `wio_status_t (*)(void*, wio_status_t, void*)`. Used for representing a WIO callback, in which the first parameter is the closure data of the callback, the second parameter contains the status code of the operation, and the third parameter points to the result of the operation.

## Constants
* Status Codes
  - `WIO_OK`: Indicates a successful operation.
  - `WIO_ERR_OUT_OF_RANGE`: An out-of-range memory access happens.
  - `WIO_ERR_NO_MEMORY`: No more memory available for allocation.
  - `WIO_ERR_ALREADY`: The operation requested has already been done.
  - `WIO_ERR_INVALID`: Program receives invalid parameters or inputs.
  - `WIO_ERR_EMPTY`: Cannot operate on empty data structures.

## Helper Marcos
* `WIO_TRY`: Emulate exception handling in C.

(TODO: Descriptions & Examples)
* `WIO_CALLBACK`: Shorthand marco for declaring or defining a WIO callback.

(TODO: Descriptions)
* `WIO_INST_PTR`: Create an anonymous variable on stack and returns a pointer to the variable.

(TODO: Descriptions & Examples)
* `WIO_RETURN`: Check and return value through pointer parameters.
* `WIO_MIN`: Get minimal value of two numbers.

```c
//Output: 2
printf("%d\n", WIO_MIN(2, 4));
```
* `WIO_MIN3`: Get minimal value of three numbers.

```c
//Output: 5
printf("%d\n", WIO_MIN3(9, 5, 11));
```

## Buffer API
Type `wio_buf_t` serves two purposes. It can be used as a binary stream for reading and writing, or it can be used as a circular buffer for memory allocation.

A WIO buffer is initialized with a piece of memory and its size. The read and write cursor of the new buffer are both set to 0.

```c
//Buffer memory
uint8_t example_mem[16];
//WIO buffer
wio_buf_t* example_buf = WIO_INST_PTR(wio_buf_t);

//Initialize buffer
WIO_TRY(wio_buf_init(example_buf, example_mem, sizeof(example_mem)));
```

To write to the buffer, call [`wio_write()`](https://lqf96.github.io/wisp-ert/client/html/buf_8h.html#a5b880b576e79955232894956d94cf154). Notice that the write cursor of the buffer (`pos_b`) will be advanced.

```c
//Write 1 byte
uint8_t tmp_u8;
WIO_TRY(wio_write(example_buf, &tmp_u8, 1))

//Write 2 bytes
uint16_t tmp_u16;
WIO_TRY(wio_write(example_buf, &tmp_u16, 2))
```

To read from the buffer, call [`wio_read()`](https://lqf96.github.io/wisp-ert/client/html/buf_8h.html#adcdf707969bf279c2c15bf59979b87fa). Similarly, the read cursor of the buffer (`pos_a`) will be advanced.

```c
//Read 1 byte
uint8_t tmp_u8;
WIO_TRY(wio_read(example_buf, &tmp_u8, 1))

//Read 2 bytes
uint16_t tmp_u16;
WIO_TRY(wio_read(example_buf, &tmp_u16, 2))
```

To copy data from one buffer to another, use [`wio_copy()`](https://lqf96.github.io/wisp-ert/client/html/buf_8h.html#aff699efa14ed37fdb1f6baed8f8bd3ea). This is equivalent to reading from the source buffer and then writing to the target buffer.

(TODO: Allocation)

(TODO: Free)

## Timer API
Type `wio_timer_t` represents a WIO software timer. To initialize it, call [`wio_timer_init()`](https://lqf96.github.io/wisp-ert/client/html/wio_2timer_8c.html#a960735b2d13c97b7a53c3b8b66c3b876):

```c
//Example timer
wio_timer_t* example_timer = WIO_INST_PTR(wio_timer_t);

//Initialize timer
WIO_TRY(wio_timer_init(example_timer));
```

(TODO: Set timeout)

(TODO: Wait for timeout)

(TODO: Clear timeout)

## Queue API
Type `wio_queue_t` represents a circular queue in the WIO API. To initialize it, call [`wio_queue_init()`](https://lqf96.github.io/wisp-ert/client/html/queue_8h.html#a7d23e8008a226c8451caa9304bbe5d49) with queue capacity and size of each queue item:

```c
//A number queue
wio_queue_t* num_queue = WIO_INST_PTR(wio_queue_t);

//Initialize queue
WIO_TRY(wio_queue_init(num_queue, sizeof(uint16_t), 5))
```

Use [`wio_queue_push()`](https://lqf96.github.io/wisp-ert/client/html/queue_8h.html#aedab04393ff4975624ddb824438a4b38) to push items into queue. Similarly, use [`wio_queue_pop()`](https://lqf96.github.io/wisp-ert/client/html/queue_8h.html#a8a8d2b1585ecbd42725efadd6272894b) to pop items from queue.

```c
//Push 3 into queue
uint16_t tmp = 3;
WIO_TRY(wio_queue_push(num_queue, &tmp))
//Push 5 into queue
tmp = 5;
WIO_TRY(wio_queue_push(num_queue, &tmp))

//Pop an item from queue
WIO_TRY(wio_queue_pop(num_queue, NULL))
//Pop another item from queue
WIO_TRY(wio_queue_pop(num_queue, &tmp))
//Popped value is 5
assert(tmp==5);
```

(TODO: WIO_QUEUE_* Marco)

(TODO: Traverse through the queue)

Finally, don't forget to release the memory of the queue after using it:

```c
//Release queue memory
WIO_TRY(wio_queue_fini(num_queue))
```
