#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static int
hexval(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static int
parse_hex(const char *s, char *out, int max)
{
	int n = 0;
	int firsthalf = -1;

	for (int i = 0; s[i] != 0; i++) {
		char c = s[i];

		int v = hexval(c);
		if (v < 0) {
			printf("hexwrite: invalid hex character '%c'\n", c);
			return -1;
		}

		if (firsthalf < 0) {
			firsthalf = v;
		} else {
			if (n >= max) {
				printf("hexwrite: input too long\n");
				return -1;
			}
			out[n++] = (char)((firsthalf << 4) | v);
			firsthalf = -1;
		}
	}

	if (firsthalf >= 0) {
		printf("hexwrite: odd number of hex digits\n");
		return -1;
	}

	return n;
}

int
main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("usage: hexwrite <hex> <file>\n");
		exit(1);
	}

	char buf[512];
	int n = parse_hex(argv[1], buf, sizeof(buf));
	if (n < 0) {
		exit(1);
	}

	int fd = open(argv[2], O_WRONLY | O_CREATE);
	if (fd < 0) {
		printf("hexwrite: cannot open %s\n", argv[2]);
		exit(1);
	}

	int done = 0;
	while (done < n) {
		int w = write(fd, buf + done, n - done);
		if (w < 0) {
			printf("hexwrite: write error on %s\n", argv[2]);
			close(fd);
			exit(1);
		}
		if (w == 0) {
			printf("hexwrite: write returned 0 on %s\n", argv[2]);
			close(fd);
			exit(1);
		}
		done += w;
	}

	if (close(fd) < 0) {
		printf("hexwrite: close error on %s\n", argv[2]);
		exit(1);
	}

	exit(0);
}