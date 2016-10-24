#include<util/twi.h>


int bamf_twi_write(char SLA_W, char* writeData, int writeLen) {
	int i;

	// Send START condition
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

	// Wait for TWINT Flag set.  This indicates that the START condition
	// has been transmitted
	while(!(TWCR & (1<<TWINT)));

	// Check value of TWI Status Register.
	// Mask prescalar bits.  If status different from START, go to ERROR
	if ((TWSR & 0xf8) != TW_START) {
		// Transmit STOP condition
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
		return -1;
	}

	TWDR = SLA_W;  // load byte into hardware
	TWCR = (1<<TWINT) | (1<<TWEN);  // start transmission
	while(!(TWCR & (1<<TWINT)));  // wait for ack

	// Check status:
	if ((TWSR & 0xf8) != TW_MT_SLA_ACK) {
		// Error.
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); // send stop
		return -2;
	}

	for (i=0; i<writeLen; i++) {
		TWDR = writeData[i];  // load data onto hardware
		TWCR = (1<<TWINT) | (1<<TWEN);  // start transmission
		while(!(TWCR & (1<<TWINT)));  // wait for ack

		// Check status:
		if ((TWSR & 0xf8) != TW_MT_DATA_ACK) {
			// Error.
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); // send stop
			return -3;
		}
	}

	// Transmit STOP condition
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);

	return 0;
}


int bamf_twi_write_read(char SLA_W, char* writeData, int writeLen, char SLA_R, char* readBuffer, int readLen) {
	int i;

	// Send START condition
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

	// Wait for TWINT Flag set.  This indicates that the START condition
	// has been transmitted
	while(!(TWCR & (1<<TWINT)));

	// Check value of TWI Status Register.
	// Mask prescalar bits.  If status different from START, go to ERROR
	if ((TWSR & 0xf8) != TW_START) {
		// Transmit STOP condition
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
		return -1;
	}

	TWDR = SLA_W;  // load byte into hardware
	TWCR = (1<<TWINT) | (1<<TWEN);  // start transmission
	while(!(TWCR & (1<<TWINT)));  // wait for ack

	// Check status:
	if ((TWSR & 0xf8) != TW_MT_SLA_ACK) {
		// Error.
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); // send stop
		return -2;
	}

	for (i=0; i<writeLen; i++) {
		TWDR = writeData[i];  // load data onto hardware
		TWCR = (1<<TWINT) | (1<<TWEN);  // start transmission
		while(!(TWCR & (1<<TWINT)));  // wait for ack

		// Check status:
		if ((TWSR & 0xf8) != TW_MT_DATA_ACK) {
			// Error.
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); // send stop
			return -3;
		}
	}

	// Send RE-START condition (without sending stop)
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

	// Wait for TWINT Flag set.  This indicates that the START condition
	// has been transmitted
	while(!(TWCR & (1<<TWINT)));

	// Check value of TWI Status Register.
	// Mask prescalar bits.  If status different from START, go to ERROR
	if ((TWSR & 0xf8) != TW_REP_START) {
		// Transmit STOP condition
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
		return -4;
	}

	TWDR = SLA_R;  // load byte into hardware
	TWCR = (1<<TWINT) | (1<<TWEN);  // start transmission
	while(!(TWCR & (1<<TWINT)));  // wait for ack

	// Check status:
	if ((TWSR & 0xf8) != TW_MR_SLA_ACK) {
		// Error.
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); // send stop
		return -5;
	}

	for (i=0; i<readLen; i++) {
		if (i == (readLen-1)) {  // last byte
			TWCR = (1<<TWINT) | (1<<TWEN);  // get a databyte, return Not ack
		} else {
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);  // get a databyte, return ack
		}
		while(!(TWCR & (1<<TWINT)));  // wait for data
		readBuffer[i] = TWDR;  // load data from hardware
	}

	// Transmit STOP condition
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);

	return 0;
}

