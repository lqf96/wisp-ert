# Home
Welcome to the WISP ERT wiki home!

## Outline
(TODO: Update links)
* Home
* WISP Firmware
  - [Introduction](wiki/WISP-Firmware:-Introduction)
  - [WIO API](wiki/WISP-Firmware:-WIO-API)
  - Run Once Project
* WTP
  - [Getting Started](wiki/WTP:-Getting-Started)
  - [EPC C1G2 Introduction](wiki/WTP:-EPC-C1G2-Introduction)
  - [Design](wiki/WTP:-Design)
  - [Protocol Format](wiki/WTP:-Protocol-Format)
* [u-RPC](https://github.com/lqf96/u-rpc/wiki)
  - [Getting Started](https://github.com/lqf96/u-rpc/wiki/Getting-Started)
  - [Protocol Design](https://github.com/lqf96/u-rpc/wiki/Protocol-Design)
* WISP ERT
* WISP ERT Demo
* [Issues](wiki/Issues)
* [Future Works](wiki/Future-Works)

## Project Structure
(TODO: Project structure description)
* `client`
  - `run-once`
  - `wisp-base`
  - `wisp-ert`
  - `wisp-ert-demo`
  - `wtp`
* `deps`
  - ``
* `misc`
* `server`
  - `sllurp`: See `deps/sllurp`.
  - `urpc`: See `deps/urpc`.
  - `wisp-ert`
  - `wtp`
* `wiki`: WISP ERT wiki documents.

## API Documentation
The API descriptions are written in Javadoc-like format and the documentation is generated with Doxygen.
* [C API](https://lqf96.github.io/wisp-ert/client/html/index.html)
* [Python API](https://lqf96.github.io/wisp-ert/server/html/index.html)

## License
The server-side WTP and WISP ERT code uses GPLv3-licensed [`sllurp`](https://github.com/lqf96/sllurp) and is therefore also licensed under [GNU GPLv3](https://github.com/lqf96/wisp-ert/blob/master/LICENSE-GPLv3). If one day these two projects no longer depend on [`sllurp`](https://github.com/lqf96/sllurp) or the underlying LLRP library is abstracted out, we will consider relicensing these two projects under more permissive open source license.

All other code in this project is licensed under [BSD 2-Clause License](https://github.com/lqf96/wisp-ert/blob/master/LICENSE-BSD-2-Clause).
