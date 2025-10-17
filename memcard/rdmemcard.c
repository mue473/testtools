// This file is free software; C 2025 Rainer Müller

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#else
#include <winscard.h>
#endif

#define CHUNK	32

#ifdef WIN32
static char *pcsc_stringify_error(LONG rv) {
    static char out[20];
    sprintf_s(out, sizeof(out), "0x%08X", rv);

    return out;
}
#endif

static void printbuffer(const BYTE *buffer, BYTE length) {
    int i;
    for (i = 0; i < length; i++)
	printf("%02X ", buffer[i]);
    printf("\n");
}

#define CHECK(f, rv) \
 if (SCARD_S_SUCCESS != rv) \
 { \
  fprintf(stderr, f ": %s\n", pcsc_stringify_error(rv)); \
 }

char manufact[][32] = {	"Orga Kartensysteme", "Giesecke & Devrient",
						"Oldenbourg Datensysteme", "Gemplus Filderstadt",
						"Solaic", "Uniqa / Orga", "Gémenos Frankreich",
						"Schlumberger", "unknown" };

static void custcard(BYTE *dat)
{
	if ((dat[3] & 0xF0) == 0x50) {
		unsigned int strt = 0, rem = 0;
		printf("prepaid payphone card D-Telekom, ID: %1X%1X%02d%02X%02X%1X\n",
				dat[3] & 15, dat[4] >> 4, dat[5] & 15, dat[7], dat[6], dat[5] >> 4);
		switch (dat[4] & 15) {
			case 3:	strt = 150;		break;
			case 4:	strt = 600;		break;
			case 5:	strt = 1200;	break;
			case 7:	strt = 5000;	break;
		}
		for (int i = 8; i < 13; i++) {
			rem = (rem << 3) + __builtin_popcount(dat[i]);
		}
		unsigned int manf = dat[3] & 15;
		printf("Hergestellt %u/xxx%u von %u: %s\n",
				dat[5] & 15, dat[4] >> 4, manf, manf > 7 ? manufact[8] : manufact[manf]);
		const char *cu = dat[4] & 8 ? "€" : "DM";
		printf("Startguthaben %u,%02u %s, Rest %u,%02u %s\n\n",
				strt / 100, strt % 100, cu, rem / 100, rem % 100, cu);
	}
	else printf("unknown custom protocol memory card\n");
}

int main(int argc, char** argv)
{
	LONG rv;

	SCARDCONTEXT hContext;
	LPTSTR mszReaders;
	SCARDHANDLE hCard;
	DWORD dwReaders, dwActiveProtocol, dwRecvLength;

	SCARD_IO_REQUEST pioSendPci;
    BYTE pbRecvBuffer[258];

	BYTE cmdSELECT[] = { 0xFF, 0xA4, 0x00, 0x00, 0x01, 0x02 };	// long address
	BYTE cmdRESET[]  = { 0x20, 0x11, 0x01, 0x01, 0x00, 0x00 };
    BYTE cmdREAD[]   = { 0xFF, 0xB0, 0x00, 0x00, CHUNK };		// read memory card

	unsigned int i, memsize = 8192;
	const char *filename = (argc < 2) ? "lcimg.bin" : argv[1];

	// prepare card reader
	rv = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);
	CHECK("SCardEstablishContext", rv)
	if (rv == SCARD_S_SUCCESS) {
		rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
		CHECK("SCardListReaders", rv)
		mszReaders = calloc(dwReaders, sizeof(char));
		if (rv == SCARD_S_SUCCESS) {
			rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
			CHECK("SCardListReaders", rv)
			printf("reader name: %s\n", mszReaders);
		}
	}

	// get in touch with the card
	if (rv == SCARD_S_SUCCESS) {
		rv = SCardConnect(hContext, mszReaders, SCARD_SHARE_DIRECT, 0, &hCard, &dwActiveProtocol);
		CHECK("SCardConnect", rv)
	}

	if (rv == SCARD_S_SUCCESS) {		// try to select card type
		dwRecvLength = sizeof(pbRecvBuffer);
		rv = SCardTransmit(hCard, &pioSendPci, cmdSELECT, sizeof(cmdSELECT), NULL, pbRecvBuffer, &dwRecvLength);
		CHECK("SCardTransmit", rv)
		printf("response for select: ");
		printbuffer(pbRecvBuffer,dwRecvLength);

		if ((dwRecvLength == 2) && (pbRecvBuffer[0] == 0x6F)) {	// choose card type by reader
			dwRecvLength = sizeof(pbRecvBuffer);
			rv = SCardControl(hCard, 0, cmdRESET, sizeof(cmdRESET), pbRecvBuffer, 32, &dwRecvLength);
			CHECK("SCardControl", rv)
			printf("response for init: ");
			printbuffer(pbRecvBuffer,dwRecvLength);
			if ((dwRecvLength == 2) && (pbRecvBuffer[0] != 0x90)) {
				fprintf(stderr, "error: no memory card detected.\n");
				rv = SCARD_W_UNRESPONSIVE_CARD;
			}
			else if (dwRecvLength > 5) {
				switch (pbRecvBuffer[0] >> 4) {
				case 8:	printf("I2C protocol memory card\n");
						break;
				case 9:	printf("3-wire bus protocol memory card\n");
						break;
				case 10:printf("2-wire bus protocol memory card\n");
						break;
				default:dwRecvLength = sizeof(pbRecvBuffer);
					rv = SCardTransmit(hCard, &pioSendPci, cmdREAD, sizeof(cmdREAD), NULL, pbRecvBuffer, &dwRecvLength);
					CHECK("SCardTransmit", rv);
					printf("response: ");
					printbuffer(pbRecvBuffer, dwRecvLength);
					if ((dwRecvLength == 2) && (pbRecvBuffer[0] != 0x90)) {
						printf("Error when reading from card.\n");
						goto finalyze;
					}
					custcard(pbRecvBuffer);
					goto finalyze;
				}
			memsize = 1 << (((pbRecvBuffer[1] & 0x78) >> 3) + ((pbRecvBuffer[1] & 7)) + 3);
			}
		}
	}

	if (rv != SCARD_S_SUCCESS) goto finalyze;
	printf("Memsize: %u Bytes\n", memsize);

	/* read card and store to file */
	FILE* pFile = fopen(filename, "wb");
    if (pFile) {
    	dwRecvLength = sizeof(pbRecvBuffer);
    	for (i = 0; i < memsize; i += CHUNK) {
			cmdREAD[2] = i >> 8;
			cmdREAD[3] = i & 255;
			rv = SCardTransmit(hCard, &pioSendPci, cmdREAD, sizeof(cmdREAD), NULL, pbRecvBuffer, &dwRecvLength);
			CHECK("SCardTransmit", rv);
			printf("read response: ");
			printbuffer(pbRecvBuffer, dwRecvLength);
			if ((dwRecvLength == 2) && (pbRecvBuffer[0] != 0x90)) {
				printf("Error when reading from card.\n");
				goto finalyze;
			}
    		fwrite(pbRecvBuffer, CHUNK, 1, pFile);
    	}
    	fclose(pFile);
	}
    else
    	printf("Something wrong opening File.\n");

finalyze:
	// cleanup
	free(mszReaders);
	rv = SCardDisconnect(hCard, SCARD_UNPOWER_CARD);
	CHECK("SCardDisconnect", rv)

	rv = SCardReleaseContext(hContext);
	CHECK("SCardReleaseContext", rv)

	return 0;
}
