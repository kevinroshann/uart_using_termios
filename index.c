#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>

#define DEFAULT_BAUD B115200
#define RX_BUFFER_SIZE 256
#define SELECT_TIMEOUT_SEC 5

int configure_uart(int fd)
{
    struct termios tty;

    /* Get current serial port settings */
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return -1;
    }

    /*
      Set input and output baud rates
     */
    cfsetispeed(&tty, DEFAULT_BAUD);
    cfsetospeed(&tty, DEFAULT_BAUD);

    /*
      Control mode flags
     */

    /* Enable receiver and ignore modem control lines */
    tty.c_cflag |= (CLOCAL | CREAD);

    /* Set 8 data bits */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    /* Disable parity */
    tty.c_cflag &= ~PARENB;

    /* Set 1 stop bit */
    tty.c_cflag &= ~CSTOPB;

    /* Disable hardware flow control */
    tty.c_cflag &= ~CRTSCTS;

    /*
      Local mode flags
     */

    /* Disable canonical mode, echo, signals */
    tty.c_lflag = 0;

    /*
      Input mode flags
     */

    /* Disable software flow control */
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    /* Disable special handling of received bytes */
    tty.c_iflag = 0;

    /*
      Output mode flags
     */

    /* Raw output */
    tty.c_oflag = 0;

    /*
      Special control characters
     */

    /* Minimum number of characters for non-canonical read */
    tty.c_cc[VMIN] = 0;

    /* Timeout in deciseconds for read() */
    tty.c_cc[VTIME] = 10;

    /*
     Apply configuration immediately
     */
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int fd;
    const char *device;
    const char *tx_message = "Hello from Linux UART using termios!\n";

   //Validate command line arguments
    
    if (argc != 2) {
        fprintf(stderr,
                "Usage: %s <serial_device>\n"
                "Example: %s /dev/ttyUSB0\n",
                argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    device = argv[1];

    /*
      Open UART device
     
     O_RDWR   : Read/write access
     O_NOCTTY : Prevent this port from becoming controlling terminal
     O_SYNC   : Synchronous writes
     */
    fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0) {
        fprintf(stderr,
                "Failed to open serial device '%s': %s\n",
                device,
                strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Opened serial device: %s\n", device);

   //Configure UART settings

    if (configure_uart(fd) != 0) {
        close(fd);
        return EXIT_FAILURE;
    }

    printf("UART configured successfully.\n");

   //Transmit test message

    ssize_t bytes_written = write(fd,
                                  tx_message,
                                  strlen(tx_message));

    if (bytes_written < 0) {
        fprintf(stderr,
                "UART write failed: %s\n",
                strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Transmitted %zd bytes.\n", bytes_written);

  //Wait for incoming data using select()
   
    printf("Waiting for incoming data...\n");

    while (1) {

        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        timeout.tv_sec = SELECT_TIMEOUT_SEC;
        timeout.tv_usec = 0;

        //Monitor UART file descriptor for readability
       
        int ret = select(fd + 1,
                         &readfds,
                         NULL,
                         NULL,
                         &timeout);

        if (ret < 0) {
            fprintf(stderr,
                    "select() failed: %s\n",
                    strerror(errno));
            break;
        }

      //Timeout occurred
 
        if (ret == 0) {
            printf("Timeout: No data received within %d seconds.\n",
                   SELECT_TIMEOUT_SEC);
            continue;
        }

     // Data available to read

        if (FD_ISSET(fd, &readfds)) {

            char rx_buffer[RX_BUFFER_SIZE];

            ssize_t bytes_read = read(fd,
                                      rx_buffer,
                                      sizeof(rx_buffer) - 1);

            if (bytes_read < 0) {
                fprintf(stderr,
                        "UART read failed: %s\n",
                        strerror(errno));
                break;
            }

            if (bytes_read == 0) {
                printf("No data received.\n");
                continue;
            }

          // Null terminate received data
   
            rx_buffer[bytes_read] = '\0';

            printf("Received (%zd bytes): %s",
                   bytes_read,
                   rx_buffer);
        }
    }


    close(fd);

    printf("Serial port closed.\n");

    return EXIT_SUCCESS;
}
