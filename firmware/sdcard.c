/*
 * GPS_Bamf!  (Datalogger for Bicycle + Strava)
 *
 * Copyright 2018, Mark Fassler
 * Licensed under the GPLv3
 *
 * I'm using an ATmega644 (as SPI master) connected to an Adafruit MicroSD card
 * breakout board:  https://www.adafruit.com/product/254
 *
 * 4-wire SPI (SS, MOSI, MISO, CLK) is connected in the most obvious, typical way.
 *
 */

#include <avr/io.h>

#include "usart.h"
#include "sdcard_csd.h"


// TODO:  don't use delays, use intelligent interrupts!
#include <util/delay.h>

//extern volatile uint32_t jiffies;

volatile uint8_t sd_card_is_ready = 0;
volatile uint32_t _sdcard_block_number = 1;
uint32_t _sdcard_number_of_blocks = 0;

// We will use an ad-hoc, hard-coded custom filesystem.
// Before writing to the card, we will check for a unique
// string:

// only use cards marked with this:
const char _CARD_ID[] = "GPS-Bamf!  https://github.com/mfassler/gps-bamf";
#define _CARD_ID_LENGTH 47


// Some of this is from:  https://github.com/greiman/Fat16.git

//------------------------------------------------------------------------------
// error codes
/** Card did not go into SPI mode */
uint8_t const SD_ERROR_CMD0              = 0x1;
/** Card did not go ready */
uint8_t const SD_ERROR_ACMD41            = 0x2;
/** Write command not accepted */
uint8_t const SD_ERROR_CMD24             = 0x3;
/** Read command not accepted */
uint8_t const SD_ERROR_CMD17             = 0x4;
/** timeout waiting for read data */
uint8_t const SD_ERROR_READ_TIMEOUT      = 0x5;
/** write error occurred */
uint8_t const SD_ERROR_WRITE_RESPONSE    = 0x6;
/** timeout waiting for write status */
uint8_t const SD_ERROR_WRITE_TIMEOUT     = 0x7;
/** attempt to write block zero */
uint8_t const SD_ERROR_BLOCK_ZERO_WRITE  = 0x8;
/** card returned an error to a CMD13 status check after a write */
uint8_t const SD_ERROR_WRITE_PROGRAMMING = 0x9;
/** card fialed to initialize with CMD1*/
uint8_t const SD_ERROR_CMD1              = 0xA;
//------------------------------------------------------------------------------

// SD operation timeouts
/** init timeout ms */
uint16_t const SD_INIT_TIMEOUT = 2000;
/** erase timeout ms */
uint16_t const SD_ERASE_TIMEOUT = 10000;
/** read timeout ms */
//uint16_t const SD_READ_TIMEOUT = 300;
// see below...

/** write time out ms */
#define SD_WRITE_TIMEOUT 600


//------------------------------------------------------------------------------
/** GO_IDLE_STATE - init card in spi mode if CS low */
#define CMD0_GO_IDLE 0

/** SEND_IF_COND - verify SD Memory Card interface operating condition.*/
#define CMD8_SEND_IF_COND 8

/** SEND_CSD - read the Card Specific Data (CSD register) */
uint8_t const CMD9 = 0X09;
/** SEND_CID - read the card identification information (CID register) */
uint8_t const CMD10 = 0X0A;
/** STOP_TRANSMISSION - end multiple block read sequence */
uint8_t const CMD12 = 0X0C;
/** SEND_STATUS - read the card status register */
uint8_t const CMD13 = 0X0D;
/** READ_SINGLE_BLOCK - read a single data block from the card */
uint8_t const CMD17 = 0X11;
/** READ_MULTIPLE_BLOCK - read a multiple data blocks from the card */
uint8_t const CMD18 = 0X12;
/** WRITE_BLOCK - write a single data block to the card */
uint8_t const CMD24 = 0X18;
/** WRITE_MULTIPLE_BLOCK - write blocks of data until a STOP_TRANSMISSION */
uint8_t const CMD25 = 0X19;
/** ERASE_WR_BLK_START - sets the address of the first block to be erased */
uint8_t const CMD32 = 0X20;
/** ERASE_WR_BLK_END - sets the address of the last block of the continuous
    range to be erased*/
uint8_t const CMD33 = 0X21;
/** ERASE - erase all previously selected blocks */
uint8_t const CMD38 = 0X26;
/** APP_CMD - escape for application specific command */
uint8_t const CMD55 = 0X37;
/** READ_OCR - read the OCR register of a card */
uint8_t const CMD58 = 0X3A;
/** CRC_ON_OFF - enable or disable CRC checking */
uint8_t const CMD59 = 0X3B;
/** SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be
     pre-erased before writing */
uint8_t const ACMD23 = 0X17;
/** SD_SEND_OP_COMD - Sends host capacity support information and
    activates the card's initialization process */
//uint8_t const ACMD41 = 0X29;
#define ACMD41_SEND_OP_COND 41






void _spi_ss_high() {
	PORTB |= (1<<PB4);
}

void _spi_ss_low() {
	PORTB &= ~(1<<PB4);
}

void _spi_send(char cData) {
	SPDR = cData;
	while (!(SPSR & (1 << SPIF)));
}

uint8_t _spi_rec(void) {
	SPDR = 0xff;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}


int _spi_waitForToken(uint8_t token, uint16_t timeoutMillis) {

	uint16_t i;
	uint8_t rx_byte;

	USART0_printf(" * _spi_waitForToken(%d, %d)\n", token, timeoutMillis);

	for (i=0; i<timeoutMillis; ++i) {
		rx_byte = _spi_rec();
		//USART0_printf("rx: %d\n", rx_byte);

		if (rx_byte == token) {
			return 0;
		}

		_delay_ms(1);
	}

	USART0_printf(" * _spi_waitForToken->timed out :-(\n");

	return -1;

/*
	int count = 0;
	while (_spi_rec() != token) {
		count++;
		if (count > timeoutMillis) {
			return -1;
		}
		_delay_ms(1);
	}

	return 0;
*/
}

#define DATA_START_BLOCK 0xfe
#define SD_READ_TIMEOUT 300

int _spi_read_transfer(char *buf, uint16_t length) {
	uint16_t i;

	USART0_printf(" * _spi_read_transfer()\n");

	// wait for start of data
	if (_spi_waitForToken(DATA_START_BLOCK, SD_READ_TIMEOUT) < 0) {
		return -2;
	}

	// start first SPI transfer
	SPDR = 0xff;
	for (i=0; i<length; ++i) {
		while(!(SPSR & (1 << SPIF)));
		buf[i] = SPDR;
		SPDR = 0xff;
	}

	// wait for first CRC bytes
	while(!(SPSR & (1 << SPIF)));
	_spi_rec(); // second CRC bytes

	//_spi_ss_high();

	return 0;
}


int sdcard_cardCommand(uint8_t cmd, uint32_t arg) {
	// All sdcard SPI commands are 6 bytes
	// MSB:
	//      0bx0000000 - must be 0 for start bit
	//      0b0x000000 - must be 1 for transmission bit
	//      0b00xxxxxx - command
	//  4 bytes of args
	// LSB:  CRC7 plus an end bit of 1

	uint8_t r1;
	//int8_t s;
	int i;
	uint16_t retry;
	uint8_t cmd_bytes[6];

	USART0_printf(" * sdcard_cardCommand(%d, %d)\n", cmd, arg);

	cmd_bytes[0] = 0x40 | cmd;
	cmd_bytes[1] = (uint8_t) (arg >> 24);
	cmd_bytes[2] = (uint8_t) (arg >> 16);
	cmd_bytes[3] = (uint8_t) (arg >> 8);
	cmd_bytes[4] = (uint8_t) arg;
	// crc7:
	cmd_bytes[5] = (cmd == 0) ? 0x95 : 0xfb;  // <--- TODO, FIXME:  this is just a hack...


	// wait if busy
	_spi_ss_low();
	_spi_waitForToken(0xff, SD_WRITE_TIMEOUT);

	// send command
	USART0_printf("sending: 0x");
	for (i=0; i<6; i++) {
		USART0_printf(" %02x", cmd_bytes[i]);
	}
	USART0_printf("\n");

	for (i=0; i<6; i++) {
		_spi_send(cmd_bytes[i]);
	}

	// wait for not busy
	for (retry = 0; retry < 0xfff; retry++) {
		r1 = _spi_rec();
		//USART0_printf("r1: %d\n", r1);
		if (!(0x80 & r1)) {
			//_spi_ss_high();
			//_spi_rec();  // clock out 8 bits
			return (int)r1;
		}
		//_delay_ms(1);
	}
	USART0_printf("timeout\n");
	_spi_ss_high();
	_spi_rec();  // clock out 8 bits

	return -1;
}


int _spi_read_register(uint8_t cmd, char *buf) {
	int retval;

	retval = sdcard_cardCommand(cmd, 0);

	if (retval < 0) {
		return retval;
	}

	return _spi_read_transfer(buf, 16);
}

/*
 * We will use the print_buffer as defined in usart.c to send the
 * data (we don't want to do a memcpy).  But it's a circular buffer
 * so we'll need to know the structure...
 */
#define DATA_RES_MASK 0x1f
#define DATA_RES_ACCEPTED 0x05

//extern int sdcard_write_block(uint32_t, char *);
int sdcard_write_block(uint32_t blockNumber, char *data) {
	//uint32_t address = blockNumber << 9;
	uint16_t i;
	uint8_t r1;

	int retval;

	retval = sdcard_cardCommand(CMD24, blockNumber);
	if (retval) {
		USART0_printf("write failed: %d\n", retval);
		return -4;
	}

	SPDR = DATA_START_BLOCK;

	for (i=0; i<512; i++) {
		// TODO: use proper interrupts!
		while (!(SPSR & (1 << SPIF)));
		SPDR = data[i];
	}

	while (!(SPSR & (1 << SPIF)));  // wait for last data byte
	_spi_send(0xff);  // dummy crc
	_spi_send(0xff);  // dummy crc

	r1 = _spi_rec();


	if ((r1 & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
		USART0_printf("write not accepted\n");
		return -3;
	}


	if (_spi_waitForToken(0xff, SD_WRITE_TIMEOUT) < 0) {
		USART0_printf("write timeout\n");
		return -2;
	}

	return 0;
}


int sdcard_write_metadata(uint32_t ending_block_number) {
	uint16_t i;
	uint8_t r1;

	int retval;

	retval = sdcard_cardCommand(CMD24, 1);
	if (retval) {
		USART0_printf("write failed: %d\n", retval);
		return -4;
	}

	SPDR = DATA_START_BLOCK;

	while (!(SPSR & (1 << SPIF)));
	SPDR = (uint8_t) (ending_block_number >> 24);
	while (!(SPSR & (1 << SPIF)));
	SPDR = (uint8_t) (ending_block_number >> 16);
	while (!(SPSR & (1 << SPIF)));
	SPDR = (uint8_t) (ending_block_number >> 8);
	while (!(SPSR & (1 << SPIF)));
	SPDR = (uint8_t) ending_block_number;

	for (i=4; i<512; i++) {
		// TODO: use proper interrupts!
		while (!(SPSR & (1 << SPIF)));
		SPDR = 0x00;
	}

	while (!(SPSR & (1 << SPIF)));  // wait for last data byte
	_spi_send(0xff);  // dummy crc
	_spi_send(0xff);  // dummy crc

	r1 = _spi_rec();

	if ((r1 & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
		USART0_printf("write not accepted\n");
		return -3;
	}


	if (_spi_waitForToken(0xff, SD_WRITE_TIMEOUT) < 0) {
		USART0_printf("write timeout\n");
		return -2;
	}

	return 0;
}


volatile uint16_t sdcard_consumer_idx = 0;
volatile uint8_t lock = 0;

int sdcard_write_from_circ_buffer(volatile char* buffer, uint16_t stop, uint16_t buf_size) {
	uint16_t i;
	uint8_t r1;
	int retval;

	if (!sd_card_is_ready) {
		return -1;
	}

	if (lock) {
		return -1;
	}
	lock = 1;

	// wrap-around math for circular buffer
	if (sdcard_consumer_idx > stop) {
		stop += buf_size;
	}

	// must have at least 512 bytes...
	if ((stop - sdcard_consumer_idx) < 512) {
		lock = 0;
		return -1;
	}


	retval = sdcard_cardCommand(CMD24, _sdcard_block_number);
	if (retval) {
		USART0_printf("write failed: %d\n", retval);
		lock = 0;
		return -4;
	}

	SPDR = DATA_START_BLOCK;

	for (i=0; i<512; i++) {
		// TODO: use proper interrupts!
		while (!(SPSR & (1 << SPIF)));
		SPDR = buffer[(i+sdcard_consumer_idx) % buf_size];
	}
	sdcard_consumer_idx += 512;
	sdcard_consumer_idx = sdcard_consumer_idx % buf_size;


	while (!(SPSR & (1 << SPIF)));  // wait for last data byte
	_spi_send(0xff);  // dummy crc
	_spi_send(0xff);  // dummy crc

	r1 = _spi_rec();


	if ((r1 & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
		USART0_printf("write not accepted\n");
		lock = 0;
		return -3;
	}


	if (_spi_waitForToken(0xff, SD_WRITE_TIMEOUT) < 0) {
		USART0_printf("write timeout\n");
		lock = 0;
		return -2;
	}
	lock = 0;

	_sdcard_block_number++;
	if (_sdcard_block_number >= _sdcard_number_of_blocks) {
		_sdcard_block_number = 2;
	}

	// Don't update the block number too much, because we don't
	// want to burn out too many write cycles on the flash chip
	if ((_sdcard_block_number % 100) == 0) {
		sdcard_write_metadata(_sdcard_block_number);
	}

	lock = 0;
	return 0;
}







int SPI_Init() {
	int retval;
	int i;
	//unsigned char rx_byte;

	uint32_t c_size;
	char read_buffer[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	csd2_t *csd;
	char inData[512];

	//DDR_SPI = (1<<DD_MOSI) | (1<<DD_SCK) | (1<<DD_SS);
	DDRB |= (1<<PB5) | (1<<PB7) | (1<<PB4);

	// From page 168 of the ATmega644 spec sheet:
	// -----------------------------------------
	//   SPIE: SPI Interrupt Enable
	//   SPE:  SPI Enable
	//   DORD:  Data Order  (0 means MSb first)
	//   MSTR:  Master mode (not slave mode)
	//   CPOL
	//   CPHA
	//   SPR1, SPR0  (clock speed.  0b11 ->  f_osc / 128)
	//SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<SPR0);  // == f_osc / 128
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1);  // == f_osc / 64
	//SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);  // == f_osc / 16

	_spi_ss_high();
	// The Adafruit code says that CS should be high for 74 clocks?...
	// At 8MHz/16 this is:  0.148 ms
	//  (the simplified sdcard docs don't say anything about this...)
	//_delay_ms(1);
	for (i=0; i<10; i++) {
		_spi_send(0xff);
	}

	for (i=0; i<5; i++) {
		USART0_printf("go idle...\n");
		retval = sdcard_cardCommand(CMD0_GO_IDLE, 0);
		USART0_printf("... retval: 0x%02x\n", retval);
		if (retval==1) {
			break;
		}
	}

	if (retval != 1) {
		USART0_printf("go idle failed: %d\n", retval);
		return -1;
	}

	// R1_IDLE_STATE == 1
	if (retval == 1) {
		USART0_printf("SD card is idle.\n");
	}

	_delay_ms(2); // this is only so my debug messages can keep up

	// SEND_IF_COND
	// 31:12 reserved (set zero)
	// 11:8 supply voltage  (page 74)  0b0001 for 3.3 volts
	// 7:0 test pattern to echo
	USART0_printf("cmd8...\n");
	retval = sdcard_cardCommand(CMD8_SEND_IF_COND, 0x00000169);
	if (retval != 1) {
		USART0_printf("cmd8 failed\n");
		return -1;
	}

	USART0_printf("r1: %02x", retval);
	read_buffer[0] = _spi_rec();
	read_buffer[1] = _spi_rec();
	read_buffer[2] = _spi_rec();
	read_buffer[3] = _spi_rec();

	for (i=0; i<4; i++) {
		USART0_printf(" %02x", read_buffer[i]);
	}

	USART0_printf("\n");

	_delay_ms(2); // this is only so my debug messages can keep up

	if (read_buffer[2] != 0x01) {
		USART0_printf("sdcard bad voltage\n");
		return -1;
	}

	if (read_buffer[3] != 0x69) {
		USART0_printf("sdcard cmd8 test pattern failed\n");
		return -1;
	}

	//_delay_ms(50);

	for (i=0; i<300; i++) {
		// Must issue a CMD55 before issuing any ACMD's
		USART0_printf("cmd55...\n");
		retval = sdcard_cardCommand(CMD55, 0);
		USART0_printf("retval: %d\n", retval);

		USART0_printf("acmd41...\n");

		// HCS bit (declaring that we support SDHC):
		retval = sdcard_cardCommand(ACMD41_SEND_OP_COND, 0x40000000);
		//retval = sdcard_cardCommand(ACMD41_SEND_OP_COND, 0);
		USART0_printf("retval: %d\n", retval);

		if ((retval & 1) == 0) {
			break;
		}
		_delay_ms(1);
	}


	_spi_ss_high();
	_delay_ms(1);

	USART0_printf("read register...\n");
	retval = _spi_read_register(CMD9, read_buffer);

	USART0_printf("retval: %d\n", retval);
	if (retval < 0) {
		return -1;
	}

/*
	USART0_printf("read_buffer: 0x");
	for (i=0; i<16; ++i) {
		USART0_printf("%2x ", read_buffer[i]);
	}
	USART0_printf("\n\n");
*/

	USART0_printf("\n\n\n    ------------ CSD -----------  \n\n\n");

	csd = (csd2_t *) read_buffer;

/*
	USART0_printf("Version: %d\n", csd->csd_ver);
	USART0_printf("high: %d\n", csd->c_size_high);
	USART0_printf("mid: %d\n", csd->c_size_mid);
	USART0_printf("low: %d\n", csd->c_size_low);
*/

	c_size = csd->c_size_high;
	c_size <<= 16;
	c_size |=  (csd->c_size_mid) << 8 | csd->c_size_low;

	USART0_printf("c_size: %d\n", c_size);

	// For a version 2 card, the total size is:
	//   (c_size + 1) * 512 * 1024 bytes
	//
	// Total unique sectors (blocks) is:
	//   (c_size + 1) * 1024 blocks
	_sdcard_number_of_blocks = c_size + 1;
	_sdcard_number_of_blocks <<= 10;   // *1024


	USART0_printf("**ABOUT TO READ***\n");

	// ****** Block #0 must begin with the magic string, or we will not use it
	retval = sdcard_cardCommand(CMD17, 0);
	_spi_read_transfer(inData, 512);

/*
	USART0_printf("data: \n");
	for (i=0; i<512; i++) {
		if (i%16 == 0) {
			USART0_printf("\n");
			_delay_ms(40);  // serial port printf can't keep up with us...
		}
		USART0_printf("%02x ", inData[i]);
	}
	USART0_printf("\n");
*/

	for (i=0; i<_CARD_ID_LENGTH; i++) {
		if (inData[i] != _CARD_ID[i]) {
			USART0_printf("sdcard not bamf\n");
			return -2;
		}
	}

	// ****** Block #1 contains our ending block
	retval = sdcard_cardCommand(CMD17, 1);
	_spi_read_transfer(inData, 512);
	//_sdcard_block_number = (uint32_t) *(&inData[64]);

	// This will be Big-Engian (ie: natural reading order / network order):
	_sdcard_block_number = inData[0];
	_sdcard_block_number <<= 8;
	_sdcard_block_number |= inData[1];
	_sdcard_block_number <<= 8;
	_sdcard_block_number |= inData[2];
	_sdcard_block_number <<= 8;
	_sdcard_block_number |= inData[3];

	USART0_printf("\n ** Ending block number: 0x%lx\n\n", _sdcard_block_number);

	if (_sdcard_block_number <= 1) {
		_sdcard_block_number = 2;
	} else if (_sdcard_block_number >= _sdcard_number_of_blocks) {
		_sdcard_block_number = 2;
	}
	_delay_ms(40); // in case serial port is slow...
	USART0_printf("\n ** Ending block number: 0x%lx\n\n", _sdcard_block_number);

	//sdcard_write_metadata(0xdeadbeef);
	sdcard_write_metadata(_sdcard_block_number);

	//sdcard_write_block(3, inData);

	sd_card_is_ready = 1;

	return 0;
}


