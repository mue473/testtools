// diganal.cpp : Hauptprogramm für die Konsolenanwendung zur Analyse des Digitalsignals.
// C 2005 - 2021 Rainer Müller
// Das Programm unterliegt den Bedingungen der GNU General Public License 3 (GPL3).

#include <string.h>
#include <stdio.h>
#include "analyse.h"

int main(int argc, char* argv[])
{
	unsigned char c = 0;
	unsigned long p;
    char * typ;
	int q, divi;
	int wav = 0;
    int trc = 0;
	int level = 0;
	int dauer = 0;
	int uschwell = 0x80 - 4;
	int oschwell = 0x80 + 4;

	if (argc < 2) {
		printf("Aufruf: diganal filename [-d|-x]\n");
		return 8;	}

	FILE *eingabe = fopen(argv[1], "rb");
	if (eingabe == NULL) {
		printf("Zu analysierende Datei nicht gefunden ! \n");
		return 16; }

	typ = strrchr(argv[1], '.');
    if (typ) c = typ[1];
	if ((c == 'w') || (c == 'W')) {
		wav = 1;
		fseek(eingabe, 24L, SEEK_SET);
		int rate = getw(eingabe);
		printf("*** Abtastrate %d Hz\n", rate);
		switch (rate) {
			case 44100:	divi = 441;	break;
			case 88200:	divi = 882;	break;
			default:	printf("Nicht unterstuetzte Abtastrate ! \n");
						return	12;
		}
		fseek(eingabe, 44L, SEEK_SET);
	}
	else if ((c >= '0') && (c <= '7')) {	// logic file extracted from sigrok archive
        trc = 1 << (c & 7);					// file extention gives the channel to analyze
        divi = 1000;						// 1 MSample/s assumed
		printf("*** Abtastrate %d kHz, Tracemaske %x\n", divi, trc);
    }
	else divi = 882;
	if ((argc > 2) && (argv[2][0] == '-')) {
		if (argv[2][1] == 'd') detail = true;
		if (argv[2][1] == 'x') mfxdetail = true;	}

	// handle wave file
	if (wav) for (p=0; !feof(eingabe); p++) {
		c = fgetc(eingabe);
		dauer++;
		if (divi == 441) dauer++;
		if ((dauer == 30) && (oschwell > uschwell)) {
			if ( level) { oschwell=0x7d; uschwell=0x7c;	}
			if (!level) { oschwell=0x85; uschwell=0x84;	}
		}
		if ((level && (c <= uschwell)) || (!level && (c >= oschwell))) {
			analysiere((p*10)/divi, dauer);
			level = 1 - level;
			dauer = 0;
		}
	}
	// handle trace file
	else if (trc) for (p=0; !feof(eingabe); p++) {
		c = fgetc(eingabe);
		dauer++;
		if ((level && (c & trc)) || (!level && !(c & trc))) {
			analysiere((p/divi), (dauer*88)/divi);
			level = 1 - level;
			dauer = 0;
		}
	}
	// handle fang file
	else for (p=0; !feof(eingabe); p++) {
		c = fgetc(eingabe);
		for (int b=0; b<8; b++) {
			q = 128 >> b;
			dauer++;
			if ((level && (c & q)) || (!level && !(c & q))) {
				analysiere((p*80/divi), dauer);
				level = 1 - level;
				dauer = 0;
			}
		}
	}

	fclose(eingabe);
	printf("\n");
	return 0;
}
