#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
civil_from_days(uint64 z, int *year, int *month, int *day)
{
	z += 719468;
	uint64 era = z / 146097;
	uint64 doe = z - era * 146097;
	uint64 yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
	uint64 y = yoe + era * 400;
	uint64 doy = doe - (365*yoe + yoe/4 - yoe/100);
	uint64 mp = (5*doy + 2)/153;
	uint64 d = doy - (153*mp+2)/5 + 1;
	uint64 m = mp + (mp < 10 ? 3 : -9);

	*year  = (int)(y + (m <= 2));
	*month = (int)m;
	*day   = (int)d;
}

static void
print2(int v)
{
	if (v < 10) {
		printf("0");
    }
	printf("%d", v);
}

void
print_date() {
	uint64 ns;

	if (rtc_read(&ns) < 0) {
		printf("date: rtc_read failed\n");
		exit(1);
	}

	uint64 secs = ns / 1000000000ULL;
	uint64 nsec = ns % 1000000000ULL;

	uint64 days = secs / 86400ULL;
	uint64 secs_of_day = secs % 86400ULL;

	uint64 hours   = secs_of_day / 3600;
	uint64 minutes = (secs_of_day % 3600) / 60;
	uint64 seconds = secs_of_day % 60;

	int year, month, day;
	civil_from_days(days, &year, &month, &day);

	printf("%d-", year);
    print2(month);
	printf("-");
    print2(day);
	printf(" ");
    print2((int)hours);
	printf(":");
    print2((int)minutes);
	printf(":");
    print2((int)seconds);
    int secd = (int)nsec / 10000000;
	printf(".");
	print2(secd);
	printf("\n");
}

int
main(int argc, char *argv[])
{
    print_date();
	exit(0);
}