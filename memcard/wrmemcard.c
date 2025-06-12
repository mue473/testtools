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

void printbuffer(BYTE *buffer, BYTE length) {
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

unsigned int cardinfo(BYTE *pbRecvBuffer)
{
	switch (pbRecvBuffer[0] >> 4) {
		case 8:	printf("I2C protocol memory card\n");
				break;
		case 9:	printf("3-wire bus protocol memory card\n");
				break;
		case 10:printf("2-wire bus protocol memory card\n");
				break;
		default:printf("unsupported card\n");
				return 0;
	}
	unsigned int memsize = 1 << (((pbRecvBuffer[1] & 0x78) >> 3) + ((pbRecvBuffer[1] & 7)) + 3);
	printf("Memsize: %u Bytes\n", memsize);

	return memsize;
}

int main(int argc, char** argv)
{
	FILE *fp;
	LONG rv;

	SCARDCONTEXT hContext;
	LPTSTR mszReaders;
	SCARDHANDLE hCard;
	DWORD dwReaders, dwActiveProtocol, dwRecvLength;

	SCARD_IO_REQUEST pioSendPci;
	BYTE pbRecvBuffer[32];

	BYTE cmdSELECT[] = { 0xFF, 0xA4, 0x00, 0x00, 0x01, 0x02 };	// long address
	BYTE cmdRESET[]  = { 0x20, 0x11, 0x01, 0x01, 0x00, 0x00 };
	BYTE cmdWRITE[CHUNK+5] = { 0xFF, 0xD0, 0x00, 0x00, CHUNK };	// write memory card

	unsigned int n, i = 0;

	// open file to copy
	if (argc < 2) {
		fprintf(stderr, "error: no input file name given\n");
		return 8;
	}
	fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		fprintf(stderr, "error: open file %s failed\n", argv[1]);
		return 8;
	}

	// prepare card reader
	rv = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);
	CHECK("SCardEstablishContext", rv)
	if (rv == SCARD_S_SUCCESS) {
		rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
		CHECK("SCardListReaders", rv)
		mszReaders = calloc(dwReaders, sizeof(char));
		rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
		CHECK("SCardListReaders", rv)
		printf("reader name: %s\n", mszReaders);
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
			cmdWRITE[1] = 0xD6;
			if ((dwRecvLength == 2) && (pbRecvBuffer[0] != 0x90)) {
				fprintf(stderr, "error: no memory card detected.\n");
				rv = SCARD_W_UNRESPONSIVE_CARD;
			}
			else if (dwRecvLength > 5) cardinfo(pbRecvBuffer);
		}
	}

	// copy the input file to the card
	if (rv == SCARD_S_SUCCESS) do {
		n = fread((void *)(cmdWRITE+5), 1, CHUNK, fp);
		if (n > 0) {
			dwRecvLength = sizeof(pbRecvBuffer);
			cmdWRITE[2] = i >> 8;
			cmdWRITE[3] = i & 255;
			cmdWRITE[4] = n;
			rv = SCardTransmit(hCard, &pioSendPci, cmdWRITE, n + 5, NULL, pbRecvBuffer, &dwRecvLength);
			CHECK("SCardTransmit", rv);
			printf("response for write to %0X: ", i);
			printbuffer(pbRecvBuffer, dwRecvLength);
			if ((dwRecvLength == 2) && (pbRecvBuffer[0] != 0x90)) {
				fprintf(stderr, "Error when writing to card.\n");
				rv = SCARD_E_NOT_TRANSACTED;
			}
			i += CHUNK;
		}
	} while ((n == CHUNK) && (rv == SCARD_S_SUCCESS));

	// cleanup
	free(mszReaders);
	rv = SCardDisconnect(hCard, SCARD_UNPOWER_CARD);
	CHECK("SCardDisconnect", rv)

	rv = SCardReleaseContext(hContext);
	CHECK("SCardReleaseContext", rv)

	fclose(fp);
	return 0;
}
