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

Beside reading and writing, a WIO buffer can also be used for memory allocation. Function [`wio_alloc()`](https://lqf96.github.io/wisp-ert/client/html/buf_8h.html#ad449c3e55563dbed0da890d035187c28) allocates memory from the WIO buffer and advances the write cursor (`pos_b`). Function [`wio_free()`](https://lqf96.github.io/wisp-ert/client/html/buf_8h.html#a19ead31e3e1c8e68a267b64afd6aade6) releases memory from the WIO buffer and advances the read cursor (`pos_a`).

```c
//Allocate buffer
char* alloc_buf;
WIO_TRY(wio_alloc(example_buf, 9, &alloc_buf))

//Use the buffer
scanf("%8s", alloc_buf);

//Release buffer
WIO_TRY(wio_free(example_buf, 9))
```

The allocation happens in a circular manner. If the write cursor reachs the end of the buffer and there is insufficient memory for allocation, the remaining memory at the end of the buffer will be skipped, and allocation will happen at the beginning of the buffer. Similarly, the end of the buffer will also be skipped when a corresponding free happends.

## Timer API
Type `wio_timer_t` represents a WIO software timer, which is implemented on top of MSP430 hardware timer using a linked list.

Before using the timer API, call [`wio_timer_subsys_init()`](https://lqf96.github.io/wisp-ert/client/html/wio_2timer_8h.html#aea40eae34fea7b302540ab29ff3ea7dd) at the beginning of your program. To initialize a single timer, call [`wio_timer_init()`](https://lqf96.github.io/wisp-ert/client/html/wio_2timer_8c.html#a960735b2d13c97b7a53c3b8b66c3b876):

```c
//Example timer
wio_timer_t* example_timer = WIO_INST_PTR(wio_timer_t);

//Initialize timer
WIO_TRY(wio_timer_init(example_timer));
```

Setting a timeout with corresponding callback can be done with [`wio_set_timeout()`](https://lqf96.github.io/wisp-ert/client/html/wio_2timer_8h.html#a85dea7d26ead0b55646f7a035e231740). The following program toggles WISP's LED light every 1 second:

```c
//Timer
wio_timer_t timer;
//WISP LED state
bool led_state = false;

WIO_CALLBACK(on_timeout) {
    if (led_state) {
        BITCLR(PLED1OUT, PIN_LED1);
        led_state = false;
    } else {
        BITSET(PLED1OUT, PIN_LED1);
        led_state = true;
    }

    WIO_TRY(wio_set_timeout(&timer, 50, NULL, on_timeout))

    return WIO_OK;
}
```

```c
//Initialize timer API and timer
WIO_TRY(wio_timer_subsys_init())
WIO_TRY(wio_timer_init(&timer))

//Kick start the program
WIO_TRY(wio_set_timeout(&timer, 50, NULL, on_timeout))
```

Alternatively, you can synchronously wait for the timer to complete using [`wio_wait4_timeout()`](https://lqf96.github.io/wisp-ert/client/html/wio_2timer_8h.html#af59eea3668b93db9c545030f6f56d008). Your program will be blocked during the whole process and the WISP will be put into low power mode.

To clear the timeout on the timer, use [`wio_clear_timeout()`](https://lqf96.github.io/wisp-ert/client/html/wio_2timer_8h.html#a8a882b09b154841f72eca6b83992a631):

```c
WIO_TRY(wio_clear_timeout(&timer))
```

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

To get the pointer to the beginning and the end of the queue, use [`WIO_QUEUE_BEGIN()`](https://lqf96.github.io/wisp-ert/client/html/queue_8h.html#ae8232a8e0759d46dd8e2563e83db7b35) and [`WIO_QUEUE_END()`](https://lqf96.github.io/wisp-ert/client/html/queue_8h.html#ac0dcf3fc103a6c8f3818135833916cbf) marco. To get the pointer to a queue item by index, use [`WIO_QUEUE_AT()`](https://lqf96.github.io/wisp-ert/client/html/queue_8h.html#aa8726b3d5f5abca8b33a3895af571ef5). Do remember to ensure the queue isn't empty and check the validity of the queue item, or you will get unexpected results.

A typical example of these marcos is the code to traverse through a WIO queue:

```c
//Index of current item
uint8_t index = num_queue->end;

for (uint8_t i=0;i<fragments_queue->size;i++) {
    //Get pointer to queue item
    uint16_t* _num = WIO_QUEUE_AT(num_queue, uint16_t, index);

    //Do whatever you want with the item
    //...

    //Move to next item
    index++;
    if (index>=num_queue->capacity)
        index = 0;
}
```

Finally, don't forget to release the memory of the queue after using it:

```c
//Release queue memory
WIO_TRY(wio_queue_fini(num_queue))
```
