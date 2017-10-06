# Home
Welcome to the WISP ERT wiki home!

## Outline
* Home
* WISP Firmware
  - [Introduction](wiki/WISP-Firmware:-Introduction)
  - [WIO API](wiki/WISP-Firmware:-WIO-API)
* WTP
  - [Getting Started](wiki/WTP:-Getting-Started)
  - [Design](wiki/WTP:-Design)
  - [Protocol Format](wiki/WTP:-Protocol-Format)
* [u-RPC](https://github.com/lqf96/u-rpc/wiki)
  - [Getting Started](https://github.com/lqf96/u-rpc/wiki/Getting-Started)
  - [Protocol Design](https://github.com/lqf96/u-rpc/wiki/Protocol-Design)
* WISP ERT
  - [Design](wiki/WISP-Extended-Runtime:-Design)
  - [Demo](wiki/WISP-Extended-Runtime:-Demo)
* [Issues](wiki/Issues)
* [Future Works](wiki/Future-Works)

## Project Structure
* `client`: Client-side C code for the WISP
  - `run-once`: The WISP5 run once project that is used to initialize the random number table used by the WISP5 firmware.
  - `wisp-base`: Modified WISP5 firmware.
  - `wisp-ert`: WISP Extended Runtime client-side code.
  - `wisp-ert-demo`: A simple file operation demo of the WISP Extended Runtime.
  - `wtp`: WTP client-side code.
* `deps`
  - `sllurp`: A custom [`sllurp`](https://github.com/lqf96/sllurp) fork used by the project.
  - `urpc`: The (u-RPC)[https://github.com/lqf96/u-rpc] remote procedure call framework.
* `misc`: Miscellaneous utilties for updating the wiki and the API documents.
* `server`: Server-side Python code for the computer.
  - `sllurp`: See `deps/sllurp`.
  - `urpc`: See `deps/urpc`.
  - `wisp-ert`: WISP Extended Runtime server-side code.
  - `wtp`: WTP server-side code.
* `wiki`: WISP ERT wiki documents.

## API Documentation
The API descriptions are written in Javadoc-like format and the documentation is generated with Doxygen.
* [C API](https://lqf96.github.io/wisp-ert/client/html/index.html)
* [Python API](https://lqf96.github.io/wisp-ert/server/html/index.html)

## License
The server-side WTP and WISP ERT code uses GPLv3-licensed [`sllurp`](https://github.com/lqf96/sllurp) and is therefore also licensed under [GNU GPLv3](https://github.com/lqf96/wisp-ert/blob/master/LICENSE-GPLv3). If one day these two projects no longer depend on [`sllurp`](https://github.com/lqf96/sllurp) or the underlying LLRP library is abstracted out, we will consider relicensing these two projects under more permissive open source license.

All other code in this project is licensed under [BSD 2-Clause License](https://github.com/lqf96/wisp-ert/blob/master/LICENSE-BSD-2-Clause).
