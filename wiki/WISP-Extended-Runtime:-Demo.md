# WISP Extended Runtime: Demo
This article introduces the steps to run the WISP ERT demo.

## Install WISP Extended Runtime
* Clone the repository to your computer and fetch the submodules of the project.

```sh
# Clone repository
git clone https://github.com/lqf96/wisp-ert
# Fetch submodules
cd wisp-ert
git submodules update
```

* Install the server-side components of WISP ERT in the following order. You may use `./setup.py develop` instead of `./setup.py install` if you want to develop or debug the WISP ERT code.

```sh
# Make a Python virtual environment
virtualenv vnev
# Enter virtual environment
. venv/bin/activate

# Sllurp (Custom fork)
(cd server/sllurp; ./setup.py install)
# u-RPC
(cd server/urpc; ./setup.py install)
# WTP
(cd server/wtp; ./setup.py install)
# WISP ERT
(cd server/wisp-ert; ./setup.py install)
```

* Import the client side projects into Code Composer Studio by choosing `Project > Import CCS Projects` and load all five projects inside the `client` folder.

## Run WISP Extended Runtime
* If you haven't initialized your WISP5 before, double click on the `run-once` project to select it, then run it to initialize the random number table on the WISP. When the LED light shines, it means the initialization work is completed.
* Enter the virtual environment you just created, and start the `wisp-ert` command line program with the IP of the RFID reader:

```sh
# Enter virtual environment
. venv/bin/activate
# Start WISP ERT server
wisp-ert -s [Reader IP]
```

* Double click on the `wisp-ert-demo` project to select it, place the WISP at an appropriate position in front of the reader, and then start the program.  
* The server-side command line program will print logs about the WTP connection and the file operations carried out on behalf of the WISP. When you see "Proxying close system call" on the screen, check if there is a file called `test.txt` under your home directory. That is the file created on behalf of the WISP, and its content should be "12345".
