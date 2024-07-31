// This file is free software; C 2024 Rainer Müller

#include <stdio.h>
#include <stdint.h>
#include <sys/io.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* SPI interface instruction set */
#define INSTRUCTION_WRITE		0x02
#define INSTRUCTION_READ		0x03
#define INSTRUCTION_BIT_MODIFY	0x05

/* MPC251x registers */
#define BFPCTRL		0x0c
#define CANSTAT		0x0e
#define CANCTRL		0x0f
#define CNF3		0x28
#define CANINTF		0x2c
#define RXBCTRL(n)	(((n) * 0x10) + 0x60)

/* Port support routines */
uint8_t spiinit(short port);
uint8_t spistat(void);
void spitransfer(uint8_t *txbuf, uint8_t *rxbuf, int len);

uint8_t txbuf[16];
uint8_t rxbuf[16] = {0};

short debug = 1;
short port = 0x378;				// LPT1=0x378, LPT2=0x278
uint8_t opmode = 0 << 5;		// 0=normal, 3=listen_only

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

static void printall()
{
	for (int i=0; i<128; i++) {
		if ((i % 16) == 0) printf("\n%2X:", i);
		printf(" %02X", mcp251x_read_reg(i));
	}
	printf("\n");
}

int main()
{
	uint8_t rdval, status;

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
	if (debug) printall();

	// continious CAN handling
	while (1) {
		status = spistat();
		if ((status & 0x20) == 0) handleRX(0);
		if ((status & 0x10) == 0) handleRX(1);
		else if ((status & 0x70) == 0x30) {			// ints other than RXBF
			rdval = mcp251x_read_reg(CANINTF);
			printf("interrupt status is %02X, error flags %02X\n", rdval, rxbuf[3]);
			mcp251x_write_bits(CANINTF, rdval & 0xFC, 0);	// reset indications
		}
	}
}
