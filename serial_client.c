#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

int uart_filestream;
volatile sig_atomic_t exit_flag = 0;
struct termios options, oldopts;

int uart_init(speed_t BAUD, const char *stream);
void exit_program(int sig);

void
exit_program(int sig)
{
    exit_flag = 1;
}

/* I think I found this printb fuc on stackoverflow */

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define for_endian(size) for (int i = 0; i < size; ++i)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define for_endian(size) for (int i = size - 1; i >= 0; --i)
#else
#error "Endianness not detected"
#endif

#define printb(value)                                   \
({                                                      \
        typeof(value) _v = value;                       \
        __printb((typeof(_v) *) &_v, sizeof(_v));       \
})

void __printb(void *value, size_t size)
{
        uint8_t byte;
        size_t blen = sizeof(byte) * 8;
        uint8_t bits[blen + 1];

        bits[blen] = '\0';
        for_endian(size) {
                byte = ((uint8_t *) value)[i];
                memset(bits, '0', blen);
                for (int j = 0; byte && j < blen; ++j) {
                        if (byte & 0x80)
                                bits[j] = '1';
                        byte <<= 1;
                }
                printf("%s ", bits);
        }
        printf("\n");
}

int
uart_init(speed_t BAUD, const char *stream)
{
    /* Open in non blocking read/write mode */
    uart_filestream = open(stream, O_RDWR | O_NDELAY);
    if (uart_filestream < 0) {
        perror("open() error");
        return -1;
    }

    switch (BAUD) {
        case 9600   : BAUD = B9600;   break;
        case 19200  : BAUD = B19200;  break;
        case 38400  : BAUD = B38400;  break;
        case 57600  : BAUD = B57600;  break;
        case 115200 : BAUD = B115200; break;

        default : {
            fprintf(stderr, "Unknown baud: %d", BAUD);
            return -3;
        }
    }

    /* Save current term settings */
    tcgetattr(uart_filestream, &oldopts);
    /* all options in man 3 tcsetattr */
    /* Set baud, 8 bit mode, ignore modem control lines, enable receiver */
    options.c_cflag = BAUD | CS8 | CLOCAL | CREAD;
    /* Ignore parity error */
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    /* I am not sure about this because we are in canonical mode, see manual */
    options.c_cc[VMIN]  = 1;
    options.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tcflush(uart_filestream, TCIFLUSH);
    /* Set the values */
    if (tcsetattr(uart_filestream, TCSANOW, &options) != 0) {
        perror("tcsetattr() error");
        return -2;
    }

    return 0;
}

int
main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "You need to specify the UART stream\n");
        exit(EXIT_FAILURE);
    }

    if (uart_init(9600, argv[1]))
        exit(EXIT_FAILURE);

    /* Install signal for ^C */
    struct sigaction sa_exit;

    sa_exit.sa_handler = exit_program;
    sa_exit.sa_flags = 0;
    sigemptyset(&sa_exit.sa_mask);

    if (sigaction(SIGINT, &sa_exit, NULL) < 0) {
        perror("sigaction() error");
        exit(EXIT_FAILURE);
    }
    else
        printf("Press ^C to exit\n");

    /* Initialize poll structure for waiting UART data */
    struct pollfd pfd;

    pfd.fd = uart_filestream;
    /* We care about only the incoming data. See man 2 poll */
    pfd.events = POLLIN;

    /* Main loop. This do the magic */
    while (!exit_flag) {
        /* Waiting for incoming data */
        int ret = poll(&pfd, 1, -1);

        if (ret < 0) {
            /* ^C gives you an interrupted system call error */
            if (errno != EINTR)
                perror("poll() error");
        }
        /* If POLLIN we have new incming data and read it */
        if (pfd.revents & POLLIN) {
            uint8_t data;
            ssize_t ret = read(pfd.fd, &data, sizeof(data));

            if(ret < 0)
                perror("read() error");
            else
                /* Print data in binary form 7...0 */
                printb(data);
        }
    }

    /* Restore term settings for uart_filestream and exit */
    tcsetattr(uart_filestream, TCSANOW, &oldopts);
    printf("bye!\n");

    return 0;
}
