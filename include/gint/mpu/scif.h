//---
//	gint:mpu:scif - Serial Communication Interface with FIFO (SCIF)
//---

#ifndef GINT_MPU_SCIF
#define GINT_MPU_SCIF

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>


typedef volatile struct
{
    // Serial Mode
    word_union(SCSMR,
        uint16_t        :8; //Reserved
        uint16_t CA     :1; //Communication Mode.
        uint16_t CHR    :1; //Character Length. 
        uint16_t PE     :1; //Parity Enable
        uint16_t OE     :1; //Parity Mode
        uint16_t STOP   :1; //Bit Stop Length   
        uint16_t        :1; //Reserved
        uint16_t CKS    :2; //Clock Select
    );
    pad(0x2);

    // Serial Bit Rate
    uint8_t SCBRD; //set the bit rate of serial transmission/reception in relation to the operating clock of the baud rate generator
    pad(0x3);

    // Serial Control
    word_union(SCSCR,
        uint16_t        :8; //Reserved
        uint16_t TIE    :1; //Transmit Interrupt Enable
        uint16_t RIE    :1; //Receive Interrupt Enable 
        uint16_t TE     :1; //Transmit Enable
        uint16_t RE     :1; //Receive Mode
        uint16_t REIE   :1; //Receive Error Interrupt Enable   
        uint16_t        :1; //Reserved
        uint16_t CKE    :2; //Clock Enable
    );
    pad(0x2);

    // Serial Transmit data
    uint8_t SCFTD; // FIFO for serial transmit data
    pad(0x3);

    // Serial Status
    word_union(SCFSR,
        uint16_t PERC   :4; //Number of Parity Errors
        uint16_t FERC   :4; //Number of Framing Errors
        uint16_t ER     :1; //Receive Error 
        uint16_t TEND   :1; //Transmit End
        uint16_t TDFE   :1; //Transmit FIFO Data Empty
        uint16_t BRK    :1; //Break Detection  
        uint16_t FER    :1; //Framing Error Indication
        uint16_t PER    :1; //Parity Error Indication
        uint16_t RDF    :1; //Receive FIFO Data Full
        uint16_t DR     :1; //Received data Ready
    );
    pad(0x2);

    // Serial Receive data
    uint8_t SCFRD; // FIFO for serial received data
    pad(0x3);

    // Serial FIFO Control
    word_union(SCFCR,
        uint16_t        :8; //Reserved
        uint16_t RTRG   :2; //Receive FIFO Data Trigger
        uint16_t TTRG   :2; //Transmit FIFO Data Trigger
        uint16_t        :1; //Reserved
        uint16_t TFRST  :1; //Transmit FIFO Data Register Reset
        uint16_t RFRST  :1; //Receive FIFO Data Register Reset 
        uint16_t LOOP   :1; //Loopback Test
    );
    pad(0x2);

    // Serial FIFO Count
    word_union(SCFDR,
        uint16_t        :3; //Reserved
        uint16_t TFDC   :5; //Number of Data Bytes in Transmit FIFO
        uint16_t        :3; //Reserved
        uint16_t RFDC   :5; //Number of Data Bytes in Receive FIFO
    );
    pad(0x2);

    // Serial Line Status
    word_union(SCLSR,
        uint16_t        :15; //Reserved
        uint16_t ORER   :1;  //Overrun Error 
    );
    pad(0x2);

} GPACKED(4) sh7305_scif_t;

#define SH7305_SCIF (*((sh7305_scif_t *)0x0xa4410000))


#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_SCIF */