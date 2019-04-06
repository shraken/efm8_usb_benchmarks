/*  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define READ_BUFF_LENGTH 4095

// One of these must be defined, usually via the Makefile
//#define MACOSX
//#define LINUX
//#define WINDOWS

#if defined(MACOSX) || defined(LINUX)
#include <termios.h>
#include <sys/select.h>
#define PORTTYPE int
#define BAUD B115200
#if defined(LINUX)
#include <sys/ioctl.h>
#include <linux/serial.h>
#endif
#elif defined(WINDOWS)
#include <windows.h>
#define PORTTYPE HANDLE
#define BAUD 115200
#else
#error "You must define the operating system\n"
#endif

// function prototypes
PORTTYPE open_port_and_set_baud_or_die(const char *name, long baud);
int receive_bytes(PORTTYPE port, char *data, int len);
void close_port(PORTTYPE port);
void delay(double sec);
void die(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

int main(int argc, char **argv)
{
	PORTTYPE port;
	uint8_t read_buffer[READ_BUFF_LENGTH];
	struct timeval begin, end;
	int n, received;
	double elapsed, speed;

	if (argc < 2) {
		die("Usage: send_test <comport>\n");
	} else {
		port = open_port_and_set_baud_or_die(argv[1], BAUD);
		printf("port %s opened\n", argv[1]);
	}

	gettimeofday(&begin, NULL);
	received = 0;

	while (1) {
		n = receive_bytes(port, (char *) read_buffer, READ_BUFF_LENGTH);
		received += n;

		if (n == -1) {
			die("errors receiveing data\n");
		}

		gettimeofday(&end, NULL);

		elapsed = (double)(end.tv_sec - begin.tv_sec);
		elapsed += (double)(end.tv_usec - begin.tv_usec) / 1000000.0;

		if (elapsed >= 10.0f) {
			break;
		}
	}

	gettimeofday(&end, NULL);
	elapsed = (double)(end.tv_sec - begin.tv_sec);
	elapsed += (double)(end.tv_usec - begin.tv_usec) / 1000000.0;

	close_port(port);
	speed = received / elapsed;
	printf("Received %d bytes in %.01f seconds\n", received, elapsed);
	printf("Average bytes per second = %.0lf\n", speed);
	return 0;
}

/**********************************/
/*  Serial Port Functions         */
/**********************************/

PORTTYPE open_port_and_set_baud_or_die(const char *name, long baud)
{
	PORTTYPE fd;
#if defined(MACOSX)
	struct termios tinfo;
	fd = open(name, O_RDWR | O_NONBLOCK);
	if (fd < 0) die("unable to open port %s\n", name);
	if (tcgetattr(fd, &tinfo) < 0) die("unable to get serial parms\n");
	cfmakeraw(&tinfo);
	if (cfsetspeed(&tinfo, baud) < 0) die("error in cfsetspeed\n");
	tinfo.c_cflag |= CLOCAL;
	if (tcsetattr(fd, TCSANOW, &tinfo) < 0) die("unable to set baud rate\n");
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
#elif defined(LINUX)
	struct termios tinfo;
	struct serial_struct kernel_serial_settings;
	int r;
	fd = open(name, O_RDWR);
	if (fd < 0) die("unable to open port %s\n", name);
	if (tcgetattr(fd, &tinfo) < 0) die("unable to get serial parms\n");
	cfmakeraw(&tinfo);
	if (cfsetspeed(&tinfo, baud) < 0) die("error in cfsetspeed\n");
	if (tcsetattr(fd, TCSANOW, &tinfo) < 0) die("unable to set baud rate\n");
	r = ioctl(fd, TIOCGSERIAL, &kernel_serial_settings);
	if (r >= 0) {
		kernel_serial_settings.flags |= ASYNC_LOW_LATENCY;
		r = ioctl(fd, TIOCSSERIAL, &kernel_serial_settings);
		if (r >= 0) printf("set linux low latency mode\n");
	}
#elif defined(WINDOWS)
	COMMCONFIG cfg;
	COMMTIMEOUTS timeout;
	DWORD n;
	char portname[256];
	int num;
	if (sscanf(name, "COM%d", &num) == 1) {
		sprintf(portname, "\\\\.\\COM%d", num); // Microsoft KB115831
	} else {
		strncpy(portname, name, sizeof(portname)-1);
		portname[n-1] = 0;
	}
	fd = CreateFile(portname, GENERIC_READ | GENERIC_WRITE,
		0, 0, OPEN_EXISTING, 0, NULL);
	if (fd == INVALID_HANDLE_VALUE) die("unable to open port %s\n", name);
	GetCommConfig(fd, &cfg, &n);
	//cfg.dcb.BaudRate = baud;
	cfg.dcb.BaudRate = 115200;
	cfg.dcb.fBinary = TRUE;
	cfg.dcb.fParity = FALSE;
	cfg.dcb.fOutxCtsFlow = FALSE;
	cfg.dcb.fOutxDsrFlow = FALSE;
	cfg.dcb.fOutX = FALSE;
	cfg.dcb.fInX = FALSE;
	cfg.dcb.fErrorChar = FALSE;
	cfg.dcb.fNull = FALSE;
	cfg.dcb.fRtsControl = RTS_CONTROL_ENABLE;
	cfg.dcb.fAbortOnError = FALSE;
	cfg.dcb.ByteSize = 8;
	cfg.dcb.Parity = NOPARITY;
	cfg.dcb.StopBits = ONESTOPBIT;
	cfg.dcb.fDtrControl = DTR_CONTROL_ENABLE;
	SetCommConfig(fd, &cfg, n);
	GetCommTimeouts(fd, &timeout);
	timeout.ReadIntervalTimeout = 0;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.ReadTotalTimeoutConstant = 1;
	timeout.WriteTotalTimeoutConstant = 0;
	timeout.WriteTotalTimeoutMultiplier = 0;
	SetCommTimeouts(fd, &timeout);
#endif
	return fd;

}

int receive_bytes(PORTTYPE port, char *data, int len)
{
#if defined(MACOSX) || defined(LINUX)
	return read(port, data, len);
#elif defined(WINDOWS)
	DWORD n;
	BOOL r;
	r = ReadFile(port, data, len, &n, NULL);

	if (!r) {
		return -1;
	} else {
		return n;
	}
#endif
}

void close_port(PORTTYPE port)
{
#if defined(MACOSX) || defined(LINUX)
	close(port);
#elif defined(WINDOWS)
	CloseHandle(port);
#endif
}


/**********************************/
/*  Misc. Functions               */
/**********************************/

void delay(double sec)
{
#if defined(MACOSX) || defined(LINUX)
	usleep(sec * 1000000);
#elif defined(WINDOWS)
	Sleep(sec * 1000);
#endif
}


void die(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	exit(1);
}

