#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static char hexdigit[] = "0123456789ABCDEF";

static void
put_hex_byte(char b)
{
	char out[2];
	out[0] = hexdigit[(b >> 4) & 0xf];
	out[1] = hexdigit[b & 0xf];
	write(1, out, 2);
}

static void
put_ascii(char b)
{
	if(b >= 0x20 && b < 0x7f)
		write(1, &b, 1);
	else
		write(1, ".", 1);
}

int
main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("usage: hexdump <count> <file>\n");
		exit(1);
	}

	int count = atoi(argv[1]);
	if (count < 0) {
		printf("hexdump: bad count\n");
		exit(1);
	}

	int fd = open(argv[2], O_RDONLY);
	if (fd < 0) {
		printf("hexdump: cannot open %s\n", argv[2]);
		exit(1);
	}

	char buf[16];
	int total = 0;

	while (total < count) {
		int want = count - total;
		if (want > (int)sizeof(buf)) {
			want = sizeof(buf);
        }

		int n = read(fd, buf, want);
		if (n < 0) {
			printf("hexdump: read error\n");
			close(fd);
			exit(1);
		}

		if (n == 0) {
			break;
        }

		printf("%d: ", total);

		for (int i = 0; i < n; i++) {
			put_hex_byte(buf[i]);

			if (i + 1 < n) {
				write(1, " ", 1);
            }
		}

		for (int i = n; i < (int)sizeof(buf); i++) {
			write(1, "   ", 3);
        }

		write(1, "  ", 2);
		for (int i = 0; i < n; i++) {
			put_ascii(buf[i]);
        }

		write(1, "\n", 1);

		total += n;
	}

	close(fd);
	exit(0);
}