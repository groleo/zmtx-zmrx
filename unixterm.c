/*
 * This file implement these functions for UNIX:
 *
 * 1. Terminal setup and tear down
 * 2. Flushing the transmit and receive channels.
 * 3. "Raw" transmit and receive on stdout and stdin.
 * 4. Timeout handling when receiving.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "zmdm.h"
#include "zmodem.h"

extern int last_sent;
int AUX_DEV_CMD_LINE=-1;

/*
 * routines to make the io channel raw and restore it
 * to its normal state.
 */

struct termios tty,oldtty;

/*
 * mode(n)
 *  ZM_MODE_RAW_FLOW: save old tty stat AND set raw mode with flow control
 *  ZM_MODE_XONXOFF: set XON/XOFF for sb/sz with ZMODEM
 *  ZM_MODE_RAW: save old tty stat AND set raw mode
 *  ZM_MODE_RESTORE: restore original tty mode
 * Returns the output baudrate, or zero on failure
 */
int fd_init(int fd, int mode)
{
    static int did0 = FALSE;
    fprintf(stderr, "mode:%d\n", mode);

    switch(mode) {

        case ZM_MODE_XONXOFF:		/* Un-raw mode used by sz, sb when -g detected */
            if(!did0) {
                did0 = TRUE;
                tcgetattr(fd,&oldtty);
            }
            tty = oldtty;
            tty.c_iflag = BRKINT|IXON;
            tty.c_oflag = 0;	/* Transparent output */
            tty.c_cflag &= ~PARENB;	/* Disable parity */
            tty.c_cflag |= CS8;	/* Set character size = 8 */
            tty.c_lflag =  0;
            tty.c_cc[VINTR] = -1;	/* Interrupt char */
#ifdef _POSIX_VDISABLE
            if (((int) _POSIX_VDISABLE)!=(-1)) {
                tty.c_cc[VQUIT] = _POSIX_VDISABLE;		/* Quit char */
            } else {
                tty.c_cc[VQUIT] = -1;			/* Quit char */
            }
#else
            tty.c_cc[VQUIT] = -1;			/* Quit char */
#endif
            tty.c_cc[VMIN] = 1;
            tty.c_cc[VTIME] = 1;	/* or in this many tenths of seconds */

            tcsetattr(fd,TCSADRAIN,&tty);

            return 1;
        case ZM_MODE_RAW:
        case ZM_MODE_RAW_FLOW:
            if(!did0) {
                did0 = TRUE;
                tcgetattr(fd,&oldtty);
            }
            tty = oldtty;

            tty.c_iflag = IGNBRK;
            /* with flow control */
            if (mode==ZM_MODE_RAW_FLOW)
                tty.c_iflag |= IXOFF;

            /* No echo, crlf mapping, INTR, QUIT, delays, no erase/kill */
            tty.c_lflag &= ~(ECHO | ICANON | ISIG);
            tty.c_oflag = 0;	/* Transparent output */

            tty.c_cflag &= ~(PARENB);	/* Same baud rate, disable parity */
            /* Set character size = 8 */
            tty.c_cflag &= ~(CSIZE);
            tty.c_cflag |= CS8;
            tty.c_cc[VMIN] = 1; /* This many chars satisfies reads */
            tty.c_cc[VTIME] = 1;	/* or in this many tenths of seconds */
            tcsetattr(fd,TCSADRAIN,&tty);
            return 1;
        case ZM_MODE_RESTORE:
            if(!did0)
                return 0;
            tcdrain (fd); /* wait until everything is sent */
            tcflush (fd,TCIOFLUSH); /* flush input queue */
            tcsetattr (fd,TCSADRAIN,&oldtty);
            tcflow (fd,TCOON); /* restart output */

            return 1;
        default:
            return 0;
    }
}

void fd_exit(int fd) {
    fd_init(fd, ZM_MODE_RESTORE);
}

/*
 * read bytes as long as rdchk indicates that
 * more data is available.
 */

void rx_purge(void) {
    struct timeval t;
    fd_set f;
    unsigned char c;

    t.tv_sec = 0;
    t.tv_usec = 0;

    FD_ZERO(&f);
    FD_SET(0, &f);

    while (select(1, &f, NULL, NULL, &t)) {
        int rv = read(0, &c, 1);
        if (rv < 0) break;
    }
}

/*
 * send the bytes accumulated in the output buffer.
 */

void tx_flush(void) { fflush(stdout); }

/*
 * transmit a character.
 * this is the raw modem interface
 */

void tx_raw(int c) {
#ifdef DEBUG
    //fprintf(stderr, "%02x ", c);
#endif

    last_sent = c & 0x7f;

    putchar(c);
}

/*
 * receive any style header within timeout milliseconds
 */

void alrm(int a) { signal(SIGALRM, SIG_IGN); }

int rx_poll(void) {
    struct timeval t;
    fd_set f;

    t.tv_sec = 0;
    t.tv_usec = 0;

    FD_ZERO(&f);
    FD_SET(0, &f);

    if (select(1, &f, NULL, NULL, &t)) {
        return 1;
    }

    return 0;
}

unsigned char inputbuffer[1024];
size_t n_in_inputbuffer = 0;
int inputbuffer_index;

/*
 * rx_raw ; receive a single byte from the line.
 * reads as many are available and then processes them one at a time
 * check the data stream for 5 consecutive CAN characters;
 * and if you see them abort. this saves a lot of clutter in
 * the rest of the code; even though it is a very strange place
 * for an exit. (but that was wat session abort was all about.)
 */

/* inline */
int rx_raw(int timeout_ms) {
    unsigned char c;
    static int n_cans = 0;

    if (n_in_inputbuffer == 0) {
        /*
         * change the timeout_ms into seconds; minimum is 1
         */

        timeout_ms /= 1000;
        if (timeout_ms == 0) {
            timeout_ms++;
        }

        /*
         * setup an alarm in case io takes too long
         */

        signal(SIGALRM, alrm);

        timeout_ms /= 1000;

        if (timeout_ms == 0) {
            timeout_ms = 2;
        }

        alarm(timeout_ms);

        n_in_inputbuffer = read(0, inputbuffer, 1024);

        if (n_in_inputbuffer <= 0) {
            n_in_inputbuffer = 0;
        }

        /*
         * cancel the alarm in case it did not go off yet
         */

        signal(SIGALRM, SIG_IGN);

        if (n_in_inputbuffer < 0 && (errno != 0 && errno != EINTR)) {
            fprintf(stderr, "zmdm : fatal error reading device\n");
            exit(1);
        }

        if (n_in_inputbuffer == 0) {
            return TIMEOUT;
        }

        inputbuffer_index = 0;
    }

    c = inputbuffer[inputbuffer_index++];
    n_in_inputbuffer--;

    if (c == CAN) {
        n_cans++;
        if (n_cans == 5) {
            /*
             * the other side is serious about this. just shut up;
             * clean up and exit.
             */
            cleanup();

            exit(CAN);
        }
    } else {
        n_cans = 0;
    }

    return c;
}

int check_user_abort(void)
{
    return 0;
}
