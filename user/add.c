#include "kernel/types.h"
#include "user/user.h"

#define BUF_SIZE      64
#define INT_MAX_VAL   2147483647
#define INT_MIN_VAL  (-2147483648)

#define RN_EMPTY      1
#define RN_TOOLONG    2
#define RN_BADNUM     3
#define RN_OVERFLOW   4
#define RN_READERR    5
#define RN_NEGATIVE   6

static void
print_error(int code)
{
    switch (code) {
    case RN_EMPTY:
        printf("Error: empty input\n");
        break;
    case RN_TOOLONG:
        printf("Error: input too long (max %d bytes)\n", BUF_SIZE - 1);
        break;
    case RN_BADNUM:
        printf("Error: invalid number\n");
        break;
    case RN_OVERFLOW:
        printf("Error: integer overflow\n");
        break;
    case RN_READERR:
        printf("Error: read error\n");
        break;
    case RN_NEGATIVE:
        printf("Error: negative number\n");
        break;
    default:
        printf("Error: unknown\n");
        break;
    }
}

static int
read_line(char *buf, int max)
{
    int bytes = 0;
    int n;

    int allowMinus = 1;

    while (bytes < BUF_SIZE) {
        n = read(0, buf + bytes, 1);
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            break;
        }
        if (buf[bytes] == '\n') {
            break;
        }

        if (buf[bytes] - '0' < 0 || buf[bytes] - '0' > 9) {
            if (buf[bytes] == '-' && allowMinus == 1) {
                return -4;
            }

            return -3;
        }

        if (buf[bytes] != '-') {
            allowMinus = 0;
        }

        bytes++;
    }

    if (bytes == BUF_SIZE && buf[BUF_SIZE-1] != '\n') {
        return -2;
    }

    if (bytes == 0) {
        return -4;
    }

    buf[bytes] = '\0';
    return bytes;
}

int main(int argc, char *argv[])
{
    char buffer[BUF_SIZE];
    int len, v1, v2;

    len = read_line(buffer, BUF_SIZE);
    if (len == -1) {
        print_error(RN_READERR);
        exit(1);
    }
    if (len == -2) {
        print_error(RN_TOOLONG);
        exit(1);
    }
    if (len == -3) {
        print_error(RN_BADNUM);
        exit(1);
    }
    if (len == -4) {
        print_error(RN_EMPTY);
        exit(1);
    }
    v1 = atoi(buffer);

    len = read_line(buffer, BUF_SIZE);
    if (len == -1) {
        print_error(RN_READERR);
        exit(1);
    }
    if (len == -2) {
        print_error(RN_TOOLONG);
        exit(1);
    }
    if (len == -3) {
        print_error(RN_BADNUM);
        exit(1);
    }
    if (len == -4) {
        print_error(RN_EMPTY);
        exit(1);
    }
    v2 = atoi(buffer);

    int overflow = 0;
    if (v2 > 0 && v1 > INT_MAX_VAL - v2)
        overflow = 1;
    else if (v2 < 0 && v1 < INT_MIN_VAL - v2)
        overflow = 1;

    if (overflow) {
        print_error(RN_OVERFLOW);
        exit(1);
    }

    int result = add(v1, v2);
    if (result < 0) {
        print_error(RN_NEGATIVE);
        exit(1);
    }

    printf("%u\n", result);
    exit(0);
}