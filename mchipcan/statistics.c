// This file is free software; C 2025 Rainer Müller

#include <stdio.h>
#include <sys/time.h>

#define BLOCK 10
#define SPEED 250

struct timeval tvstart, tvdiff;
unsigned long totstd, totext, totdat;
unsigned int astd, aext, adat;
int n = -1;

void statistics(struct timeval tv, int extmsg, int dlen)
{
	if (n++ < 0) tvstart = tv;
	else {
		if (extmsg) aext++;
		else astd++;
		adat += dlen;
	}
	if (n == BLOCK) {
		totstd += astd;
		totext += aext;
		totdat += adat;
		timersub(&tv, &tvstart, &tvdiff);
		unsigned int tms = tvdiff.tv_sec * 1000 + tvdiff.tv_usec / 1000;
		unsigned int nbits = 47 * BLOCK + 20 * aext + 8 * adat;
		unsigned int load = (nbits * 110) / (SPEED * tms);		// 110 -> 100% + stuffing
		printf("total S:%lu / X:%lu / D:%lu, last S:%u / X:%u / D:%u in %ums, load %u%%\n",
			totstd, totext, totdat, astd, aext, adat, tms, load);			
		n = 0;
		astd = 0;
		aext = 0;
		adat = 0;
		tvstart = tv;
	}
}
