# WISP Extended Runtime: Design
This article talks about the design of the WISP extended runtime (ERT).

## Server API
### ERT Service
For the WISP ERT server API, a group of related functions and constants are packed into a service. A WISP ERT service should inherit from the [`Service`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1runtime_1_1_service.html) class and implement [`Service.functions`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1runtime_1_1_service.html#a0513370028fabcfe9ec6c31fc881c3c5) and [`Service.constants`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1runtime_1_1_service.html#a49b7c86414101e196d74a1b9a53cc7ee) property. The former function should return a mapping from function name to function, and the latter should return an iterable of tuples containing the constant and its u-RPC type.

When a service is added to the WISP ERT server, all of its functions are added to the u-RPC endpoint, and all of its constants are synchronized to WISP when a new client connects to the server. Synchronizing constants ensure that the WISP's constants always match the constants of the server, regardless of the platform of the server.

### File System Demo Service
The WISP ERT server-side package currently contains a file system demo service called "fs", which is represented by the [`LocalFS`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1fs_1_1_local_f_s.html) class. It is used to demostrate the ability for WISP to remotely manipulate files.

The file system demo service instance is constructed with the root path WISPs can access and it contains a few file operation functions. The [`LocalFS.open()`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1fs_1_1_local_f_s.html#aecfe57d28b75b42a599db78f2a25de6c) method opens a file on behalf of the WISP, and returns a virtual file descriptor only valid for the service. The [`LocalFS.close()`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1fs_1_1_local_f_s.html#a4b28219ee9fb58d20cc6bb81706637f9) method closes and removes the virtual file descriptor. Other functions like [`LocalFS.read()`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1fs_1_1_local_f_s.html#a6419bddde896ac369d20d4873ceb40b3), [`LocalFS.write()`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1fs_1_1_local_f_s.html#a01c828e1abd28c5e53bf373c42dec2b5) and [`LocalFS.lseek()`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1fs_1_1_local_f_s.html#af24a3200a2724b4e209cf4a13f85c055) are thin wrappers around respective system call functions and provides the WISP with the ability to read, write and seek. All functions returns non-negative result if succeeded, and negative error number on failure, except [`LocalFS.read()`](https://lqf96.github.io/wisp-ert/server/html/classwisp__ert_1_1fs_1_1_local_f_s.html#a6419bddde896ac369d20d4873ceb40b3) also returns the data read on success or an empty byte string on failure.

The file system demo service also contains common error numbers, open parameters and seek parameters. This allows the WISP to work with servers on different POSIX platforms.

## Client API
### Constants Loading
Once the WISP connects to the WISP ERT server using WTP, it starts synchronizing constants from the server. This is done by doing an u-RPC remote function call to [`ert_func_srv_consts`](https://lqf96.github.io/wisp-ert/client/html/rpc_8h.html#a1338589078ff47411b66ee89a6146376) on the computer. The function accepts the name of the service and returns all constants of the server in a packed byte array. By defining a structure that matches the order and the size of the constants declared on the server, we can get the value of a particular constant.

### Context Switching and Stackful Coroutine
Because most of the WTP, u-RPC and WISP ERT operations are asynchronous, we can end up writing a lot of callback functions, making the code ugly and less readable:

```c
WIO_CALLBACK(test_then_2) {
    //More synchronous logic
    //...

    return WIO_OK;
}

WIO_CALLBACK(test_then_1) {
    //More synchronous logic
    //...

    return another_async_op(..., NULL, test_then_2);
}

wio_status_t test() {
    //Synchronous logic
    //...

    return async_op(..., NULL, test_then_1);
}
```

To solve the problem, a stackful coroutine can be used instead. A stackful coroutine has a piece of memory as its stack when being executed. When we are going to execute an asynchronous function, we use `ERT_AWAIT` marco to hang up the coroutine until the asynchronous function invokes the callback it receives. The above code can now be rewritten this way:

```c
//Operation status
ert_status_t status;
//Placeholder variable
uint16_t _;

//Asynchronous operation 1
ERT_AWAIT(status, uint16_t, _, async_op, ...)
//Asynchronous operation 2
ERT_AWAIT(status, uint16_t, _, another_async_op, ...)
```

This approach also provides another benefit. Since the stack of the coroutine is on the FRAM rather than the memory, when WISP loses power the stack won't lose, which makes preserving the state of the WISP easier than before.
