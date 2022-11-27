//---
//	gint:serial - Serial operation
//---

#include <gint/serial.h>

#define SCIF SH7305_SCIF


int gint_serial_read_one_byte(unsigned char *dest)
{
    
};

int gint_serial_write_one_byte(unsigned char byte)
{

};

int gint_serial_write_bytes(unsigned char *src, int size)
{

};

int gint_serial_clear_receive_buffer(void)
{

};

int gint_serial_clear_transmit_buffer(void)
{

};

int gint_serial_open(unsigned char *conf)
{
    SCIF.SCSCR.TE=0b0;
    SCIF.SCSCR.RE=0b0;
    SCIF.SCSCR.TIE=0b0;
    SCIF.SCSCR.RIE=0b0;

    SCIF.SCFCR.TFRST=0b1;
    SCIF.SCFCR.RFRST=0b1;

    SCIF.SCLCR.ORER=0b0;
    SCIF.SCFSR.ER=0b0;
    SCIF.SCFSR.DR=0b0;
    SCIF.SCFSR.BRK=0b0;

    SCIF.SCSCR.CKE=0b01;    //set to internal clock

    SCIF.SCSMR.CA = 0b1; // clock synchronous mode
    SCIF.SCSMR.CHR = 0b0; // 8bits length for data
    SCIF.SCSMR.PE = 0b0; // Parity bit not set
    SCIF.SCSMR.OE = 0b0; // just cleared, no effect as no parity is set
    SCIF.SCSMR.STOP = 0b0; // not effect in synchronous mode
    SCIF.SCSMR.CKS = 0b00; // 00=Pphi, 01=Pphi/4, 10=Pphi/16, 11=Pphi/64

    SCIF.SCFCR.RTRG = 0b11;
    SCIF.SCFCR.TTRG = 0b11;
    SCIF.SCFCR.RFRST = 0b0;
    SCIF.SCFCR.TFRST = 0b0;

    //PFC setting for external pins used
    //SCK, TXD, RXD

    SCIF.SCSCR.TE=0b1;
    SCIF.SCSCR.RE=0b1;
    SCIF.SCSCR.TIE=0b1;
    SCIF.SCSCR.RIE=0b1;
    SCIF.SCSCR.REIE=0b1;
    
};

int gint_serial_close(int mode)
{

};