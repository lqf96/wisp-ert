# Issues
This article lists the current issues of the WISP Extended Runtime.

## WISP Firmware
* Investigate if [`WISP_doRFID()`](https://lqf96.github.io/wisp-ert/client/html/globals_8h.html#a49df2cf7243a0c685a1be336b253cf7c) is corrupting the stack and if it is, fix it.
* To improve the efficiency of WIO functions, do not use `wio_status_t` for functions that does not throw any error. Return values directly instead of returning results through parameters.
* Refactor the WIO buffer API to make it more efficient. Remove the bounds check inside [`wio_read()`](https://lqf96.github.io/wisp-ert/client/html/buf_8h.html#adcdf707969bf279c2c15bf59979b87fa) and [`wio_write()`](https://lqf96.github.io/wisp-ert/client/html/buf_8h.html#a5b880b576e79955232894956d94cf154), and provide a spearate function for bounds check of the buffer. Inline as much WIO buffer functions as possible.
* The WIO timer API still has bugs and sometimes can't be used. The `current_time` variable is sometimes mysteriously modified outside the WIO API code, causing the software timer system to fail. Investigate the cause of the problem and fix it.

## WTP
* Refactor function signatures and usage of WIO functions to bring WTP on par with the WIO API.
* Currently function `wtp_after_do_rfid()` is directly inlined in the RFID loop, because the WTP code fails to work if the code is replaced by a function call. The problem might be related with potential stack corruption inside [`WISP_doRFID()`](https://lqf96.github.io/wisp-ert/client/html/globals_8h.html#a49df2cf7243a0c685a1be336b253cf7c). Before the problem is solved inside the firmware, see if we have any workarounds that solve the problem.
* Because of the timer issue inside the WISP firmware, retransmission for the uplink isn't enabled. Implements the uplink retransmission using WIO timers. The server-side retransmission code can be used as a reference.
* Support multiple OpSpec inside one AccessSpec. The current way that the WTP hook functions are called isn't compatible with that. May also requires optimization of the WTP client-side code.
* Acknowledgement, timeout and retransmission mechanism for control packets. Many types of control packets needs to be delivered reliably, and currently WTP has no such mechansim.

## WISP ERT
* Improves the efficiency of the u-RPC code and refactor its usage of WIO functions.
* Refactor the server-side WISP ERT code to allow custom services and configurations being applied for the `wisp-ert` command line utilities.
