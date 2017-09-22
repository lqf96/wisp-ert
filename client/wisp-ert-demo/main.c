#include <fcntl.h>
#include <ert/fs.h>
#include <ert/runtime.h>

/**
 * {@inheritDoc}
 */
void ert_main() {
    //Status
    ert_status_t status;

    //File descriptor
    int16_t fd;
    //Placeholder variable
    int16_t _;
    //Read data
    uint8_t* read_data;

    //Open a file
    //ERT_AWAIT(status, int16_t, fd, ert_open, "./test.txt", O_RDONLY, 0644)
    ERT_AWAIT(status, int16_t, fd, ert_open, "./test.txt", O_CREAT|O_RDWR, 0644)
    if (status!=0)
        goto demo_end;

    //Read file
    /*
    ERT_AWAIT(status, uint8_t*, read_data, ert_read, fd, 5)
    if (status!=0)
        goto demo_end;
    */
    //Write to file
    ERT_AWAIT(status, int16_t, _, ert_write, fd, "12345", 5)
    if (status!=0)
        goto demo_end;

    //Close file
    ERT_AWAIT(status, int16_t, _, ert_close, fd)
demo_end:
    //Suspend user context
    ert_user_suspend(NULL, 0, NULL);
}
