// This file is free software; C 2024 Rainer Müller

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

/* SPI interface instruction set */
#define INSTRUCTION_WRITE		0x02
#define INSTRUCTION_READ		0x03
#define INSTRUCTION_BIT_MODIFY	0x05
#define INSTRUCTION_RTS(n)	(0x80 | ((n) & 0x07))

/* MPC251x registers */
#define BFPCTRL		0x0c
#define CANSTAT		0x0e
#define CANCTRL		0x0f
#define CNF3		0x28
#define CANINTF		0x2c
#define TXBCTRL(n)	(((n) * 0x10) + 0x30)
#define RXBCTRL(n)	(((n) * 0x10) + 0x60)

/* Port support routines */
uint8_t spiinit(short port);
uint8_t spistat(void);
void spitransfer(uint8_t *txbuf, uint8_t *rxbuf, int len);

uint8_t txbuf[16];
uint8_t rxbuf[16] = {0};
uint8_t txmsg[16];

short debug = 1;
short port = 0x378;				// LPT1=0x378, LPT2=0x278
uint8_t opmode = 0 << 5;		// 0=normal, 3=listen_only
char *txfifo = "/tmp/cantx";	// default trigger

static uint8_t mcp251x_read_reg(uint8_t reg)
{
	txbuf[0] = INSTRUCTION_READ;
	txbuf[1] = reg;
	spitransfer(txbuf, rxbuf, 4);	// read two bytes
	return rxbuf[2];				// return the first
}

static void mcp251x_write_reg(uint8_t reg, uint8_t val)
{
	txbuf[0] = INSTRUCTION_WRITE;
	txbuf[1] = reg;
	txbuf[2] = val;
	spitransfer(txbuf, NULL, 3);
}

static void mcp251x_write_bits(uint8_t reg, uint8_t mask, uint8_t val)
{
	txbuf[0] = INSTRUCTION_BIT_MODIFY;
	txbuf[1] = reg;
	txbuf[2] = mask;
	txbuf[3] = val;
	spitransfer(txbuf, NULL, 4);
}

static void decodeMsg(uint8_t * msg, char info)
{
	unsigned int dlc, hid, rtr;
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	printf("%02d:%02d:%02d.%03d  %c ",
			tm->tm_hour, tm->tm_min, tm->tm_sec, (int)tv.tv_usec / 1000, info);

	dlc = msg[4] & 0x0F;
	hid = ((unsigned int)msg[0] << 8) | (msg[1] & 0xE0);
	if (msg[1] & 8) {			// extended ID
		rtr = msg[4] & 0x40;
		hid = (hid >> 3) | (msg[1] & 3);
		printf("0x%04X%02X%02X  [%u]", hid, msg[2], msg[3], dlc);
	} else {					// standard ID
		rtr = msg[1] & 0x10;
		printf("<S>  0x%03X  [%u]", hid >> 5, dlc);
	}
	if (info == 'T') rtr = msg[4] & 0x40;
	if (rtr) printf(" <RTR>");
	else for (int i=5; i<(dlc + 5); i++) printf(" %02X", msg[i]);
	printf("\n");
}

static void handleRX(int chan)
{
	txbuf[0] = INSTRUCTION_READ;
	txbuf[1] = RXBCTRL(chan) + 1;
	spitransfer(txbuf, rxbuf, 15);
	decodeMsg(&rxbuf[2], debug ? chan + 0x30 : ' ');
	mcp251x_write_bits(CANINTF, 1 << chan, 0);
}

static int parseTXcmd(char *line)
{
	int dlc = 0;
	char dby[4];
	char *sep = strchr(line, '#');
	if (sep == NULL) return 0;
	*sep = 0;				// separate
	int lv = sep - line;
	if (lv == 0) {			// display MCP251x register
		long reg = strtol(sep + 1, NULL, 16) & 0xFF;
		printf("**** register %02lX contains %02X\n", reg, mcp251x_read_reg(reg));
		return 0;
	}
	long id = strtol(line, NULL, 16);
	if ((lv > 3) || (id > 0x7FF)){
		if (debug) printf("extended id %08lx ", id);
		txmsg[2] = (id >> 21) & 0xFF;
		txmsg[3] = ((id >> 13) & 0xE0) | ((id >> 16) & 3) | 8;
		txmsg[4] = (id >> 8) & 0xFF;
		txmsg[5] = id & 0xFF;
	} else {
		if (debug) printf("standard id %03lx ", id);
		txmsg[2] = (id >> 3) & 0xFF;
		txmsg[3] = (id << 5) & 0xE0;
	}
	if ((sep[1] == 'r') || (sep[1] == 'R')) {
		if ((sep[2] >= '0') && (sep[2] <= '8')) dlc = sep[2] & 0xF;
		if (debug) printf("RTR %d\n", dlc);
		txmsg[6] = dlc | 0x40;
		return 7;
	}
	while (sep[1] && sep[2] && (dlc < 8)) {
		sprintf(dby, "%c%c\n", sep[1], sep[2]);
		long val = strtol(dby, NULL, 16);
		txmsg[7 + dlc++] = val & 0xFF;
		sep += 2;
	}
	if (debug) printf("with DLC %d\n", dlc);
	txmsg[6] = dlc;
	return (dlc + 7);
}

int main()
{
	uint8_t rdval, status;
	char *line = NULL;
	size_t llen;
	ssize_t nread = -1;

	umask(0);
	mkfifo(txfifo, 0666);
	FILE *infifo = fdopen(open(txfifo, O_RDONLY | O_NONBLOCK), "r");
	if (infifo == NULL) printf("could not open input fifo, no transmission\n");

	if (ioperm(port, 3L, 1)) {
		printf("error ioperm failed, permission insufficient\n");
		return 8;
	}
	status = spiinit(port);
	if (debug) printf("port status after reset is %X\n", status);

	usleep(5000);
	rdval = mcp251x_read_reg(CANCTRL);
	if (rdval != 0xE7) {
		printf("HW not working correctly, CANCTRL after Reset: %2X\n", rdval);
		return 8;
	}

	// Initialize CAN Timing  250 Kbps @ 16MHz
	txbuf[0] = INSTRUCTION_WRITE;
	txbuf[1] = CNF3;
	txbuf[2] = 0x05;	//0000 0101  //PS2  6TQ
	txbuf[3] = 0xB8;	//1011 1000  //SEG2PHTS 1  sampled once  PS1=8TQ  Prop 1TQ
	txbuf[4] = 0x80 + (16000000 / 250000 / 32) - 1;
	txbuf[5] = 0xFF;	// enable all interrupt sources
	txbuf[6] = 0x00;	// reset all pending flags
	spitransfer(txbuf, NULL, 7);

	mcp251x_write_reg(RXBCTRL(0), 0x64);	// any message, rollover enabled
	mcp251x_write_reg(RXBCTRL(1), 0x60);	// any message
	mcp251x_write_reg(BFPCTRL, 0x0F);		// activate both RXBF pins

	// Initialization completed, finalize config mode
	mcp251x_write_reg(CANCTRL, opmode);
	usleep(10000);
	rdval = mcp251x_read_reg(CANSTAT);
	if ((rdval & 0xE0) != opmode) {
		printf("Operation mode not switched correctly, CANSTAT: %2X\n", rdval);
		return 8;
	}

	// continious CAN handling
	while (1) {
		status = spistat();
		if ((status & 0x20) == 0) handleRX(0);
		if ((status & 0x10) == 0) handleRX(1);
		else if ((status & 0x70) == 0x30) {			// ints other than RXBF
			rdval = mcp251x_read_reg(CANINTF);
			if (debug)
				printf("interrupt status is %02X, error flags %02X\n", rdval, rxbuf[3]);
			if (rdval & 0x80) printf("**** CAN message error\n");
			if (rdval & 0x20) printf("**** CAN controller error: %02X\n", rxbuf[3]);
			if (rdval & 0x1C) decodeMsg(&txmsg[2], 'T');
			mcp251x_write_bits(CANINTF, rdval & 0xFC, 0);	// reset indications
		}
		else if (infifo) {
			clearerr(infifo);
			nread = getline(&line, &llen, infifo);	// check if tx requested
			if (nread > 0) {
				int tlen = parseTXcmd(line);
				if (tlen) {
					txmsg[0] = INSTRUCTION_WRITE;
					txmsg[1] = TXBCTRL(0) + 1;		// send via TX buffer 0
					spitransfer(txmsg, NULL, tlen);
					txmsg[0] = INSTRUCTION_RTS(1);	// trigger transmission 0
					spitransfer(txmsg, NULL, 1);
				}
			}
		}
	}
}
