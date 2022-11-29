//---
//	gint:mpu:pfc - Pin Function Controller
//
//	The Pin Function Controller has a simple register interface, the main
//	difficulty is still understanding the role of its pins.
//---

#ifndef GINT_MPU_PFC
#define GINT_MPU_PFC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

//---
//	SH7705 Pin Function Controller. Refer to:
//	  "Renesas SH7705 Group Hardware Manual"
//	  Section 19: "Pin Function Controller"
//---

typedef volatile struct
{
	/* Control registers */
	uint16_t PACR;
	uint16_t PBCR;
	uint16_t PCCR;
	uint16_t PDCR;
	uint16_t PECR;
	uint16_t PFCR;
	uint16_t PGCR;
	uint16_t PHCR;
	uint16_t PJCR;
	uint16_t PKCR;
	uint16_t PLCR;
	uint16_t SCPCR;		/* Port SC control register */
	uint16_t PMCR;
	uint16_t PNCR;

	pad(4);

	/* Data registers */
	uint8_t PADR;
	pad(1);
	uint8_t PBDR;
	pad(1);
	uint8_t PCDR;
	pad(1);
	uint8_t PDDR;
	pad(1);
	uint8_t PEDR;
	pad(1);
	uint8_t PFDR;
	pad(1);
	uint8_t PGDR;
	pad(1);
	uint8_t PHDR;
	pad(1);
	uint8_t PJDR;
	pad(1);
	uint8_t PKDR;
	pad(1);
	uint8_t PLDR;
	pad(1);
	uint8_t SCPDR;		/* Port SC data register */
	pad(1);
	uint8_t PMDR;
	pad(1);
	uint8_t PNDR;
	pad(1);

} GPACKED(4) sh7705_pfc_t;

#define SH7705_PFC (*((sh7705_pfc_t *)0xa4000100))

//---
//	TODO: Document the SH7305 Pin Function Controller
//---

typedef volatile word_union( sh7305_Port_Control_Register_t,
	uint16_t    P7MD :2;
	uint16_t    P6MD :2;
	uint16_t    P5MD :2;
	uint16_t    P4MD :2;
	uint16_t    P3MD :2;
	uint16_t    P2MD :2;
	uint16_t    P1MD :2;
	uint16_t    P0MD :2;
);

typedef volatile byte_union( sh7305_port_data_register_t,
	uint8_t    P7DT :1;
	uint8_t    P6DT :1;
	uint8_t    P5DT :1;
	uint8_t    P4DT :1;
	uint8_t    P3DT :1;
	uint8_t    P2DT :1;
	uint8_t    P1DT :1;
	uint8_t    P0DT :1;
);

typedef volatile word_union( sh7305_pin_select_register_t,
	uint16_t    PS15 :1;
	uint16_t    PS14 :1;
	uint16_t    PS13 :1;
	uint16_t    PS12 :1;
	uint16_t    PS11 :1;
	uint16_t    PS10 :1;
	uint16_t    PS9 :1;
	uint16_t    PS8 :1;
	uint16_t    PS7 :1;
	uint16_t    PS6 :1;
	uint16_t    PS5 :1;
	uint16_t    PS4 :1;
	uint16_t    PS3 :1;
	uint16_t    PS2 :1;
	uint16_t    PS1 :1;
	uint16_t    PS0 :1;
);

typedef volatile word_union( sh7305_IO_buffer_hiz_control_register_t,
	uint16_t    HIZ15 :1;
	uint16_t    HIZ14 :1;
	uint16_t    HIZ13 :1;
	uint16_t    HIZ12 :1;
	uint16_t    HIZ11 :1;
	uint16_t    HIZ10 :1;
	uint16_t    HIZ9 :1;
	uint16_t    HIZ8 :1;
	uint16_t    HIZ7 :1;
	uint16_t    HIZ6 :1;
	uint16_t    HIZ5 :1;
	uint16_t    HIZ4 :1;
	uint16_t    HIZ3 :1;
	uint16_t    HIZ2 :1;
	uint16_t    HIZ1 :1;
	uint16_t    HIZ0 :1;
);

typedef volatile word_union( sh7305_module_function_select_register_t,
	uint16_t    MSEL15 :1;
	uint16_t    MSEL14 :1;
	uint16_t    MSEL13 :1;
	uint16_t    MSEL12 :1;
	uint16_t    MSEL11 :1;
	uint16_t    MSEL10 :1;
	uint16_t    MSEL9 :1;
	uint16_t    MSEL8 :1;
	uint16_t    MSEL7 :1;
	uint16_t    MSEL6 :1;
	uint16_t    MSEL5 :1;
	uint16_t    MSEL4 :1;
	uint16_t    MSEL3 :1;
	uint16_t    MSEL2 :1;
	uint16_t    MSEL1 :1;
	uint16_t    MSEL0 :1;
);

typedef volatile word_union( sh7305_IO_buffer_drive_control_register_t,
	uint16_t    DRV15 :1;
	uint16_t    DRV14 :1;
	uint16_t    DRV13 :1;
	uint16_t    DRV12 :1;
	uint16_t    DRV11 :1;
	uint16_t    DRV10 :1;
	uint16_t    DRV9 :1;
	uint16_t    DRV8 :1;
	uint16_t    DRV7 :1;
	uint16_t    DRV6 :1;
	uint16_t    DRV5 :1;
	uint16_t    DRV4 :1;
	uint16_t    DRV3 :1;
	uint16_t    DRV2 :1;
	uint16_t    DRV1 :1;
	uint16_t    DRV0 :1;
);


typedef volatile struct
{
	// List of all PORT CONTROL REGISTERS (PCRs)
	sh7305_Port_Control_Register_t PACR;
	sh7305_Port_Control_Register_t PBCR;
	sh7305_Port_Control_Register_t PCCR;
	sh7305_Port_Control_Register_t PDCR;
	sh7305_Port_Control_Register_t PECR;
	sh7305_Port_Control_Register_t PFCR;
	sh7305_Port_Control_Register_t PGCR;
	sh7305_Port_Control_Register_t PHCR;
	sh7305_Port_Control_Register_t PJCR;
	sh7305_Port_Control_Register_t PKCR;
	sh7305_Port_Control_Register_t PLCR;
	sh7305_Port_Control_Register_t PMCR;
	sh7305_Port_Control_Register_t PNCR;
	sh7305_Port_Control_Register_t PQCR;
	sh7305_Port_Control_Register_t PRCR;
	sh7305_Port_Control_Register_t PSCR;

	// List of all PORT DATA REGISTERS (PDRs)
	sh7305_port_data_register_t PADR;
	sh7305_port_data_register_t PBDR;
	sh7305_port_data_register_t PCDR;
	sh7305_port_data_register_t PDDR;
	sh7305_port_data_register_t PEDR;
	sh7305_port_data_register_t PFDR;
	sh7305_port_data_register_t PGDR;
	sh7305_port_data_register_t PHDR;
	sh7305_port_data_register_t PJDR;
	sh7305_port_data_register_t PKDR;
	sh7305_port_data_register_t PLDR;
	sh7305_port_data_register_t PMDR;
	sh7305_port_data_register_t PNDR;
	sh7305_port_data_register_t PQDR;
	sh7305_port_data_register_t PRDR;
	sh7305_port_data_register_t PSDR;

	sh7305_Port_Control_Register_t PTCR;
	sh7305_Port_Control_Register_t PUCR;
	sh7305_Port_Control_Register_t PVCR;

	//Missing ports ?
	//sh7305_Port_Control_Register_t PWCR;
	//sh7305_Port_Control_Register_t PXCR;
	//sh7305_Port_Control_Register_t PYCR;
	//sh7305_Port_Control_Register_t PZCR;
	pad( 0x08 );

	// List of all PIN SELECT REGISTERS (PSELs)
	sh7305_pin_select_register_t PSELA;
	sh7305_pin_select_register_t PSELB;
	sh7305_pin_select_register_t PSELC;
	sh7305_pin_select_register_t PSELD;
	sh7305_pin_select_register_t PSELE;

	// List of all IO BUFFER HI-Z CONTROL REGISTERS (HIZCRs)
	sh7305_IO_buffer_hiz_control_register_t HIZCRA;
	sh7305_IO_buffer_hiz_control_register_t HIZCRB;
	sh7305_IO_buffer_hiz_control_register_t HIZCRC;

	// the next one is not clearly listed, can be aither PSELF or HIZCRD so we skip it with pad
	//sh7305_pin_select_register_t PSELF;
	//sh7305_IO_buffer_hiz_control_register_t HIZCRD;
	pad( 0x02 );

	sh7305_port_data_register_t PTDR;
	sh7305_port_data_register_t PUDR;
	sh7305_port_data_register_t PVDR;

	//sh7305_port_data_register_t PWDR;
	//sh7305_port_data_register_t PXDR;
	//sh7305_port_data_register_t PYDR;
	//sh7305_port_data_register_t PZDR;
	pad( 0x08 );


	pad( 20 ); // jump from address 0xa405016c to 0xa4050180


	// List of all MODULE FUNCTION SELECT REGISTERS (MSELs)
	sh7305_module_function_select_register_t MSELCRA;
	sh7305_module_function_select_register_t MSELCRB;

/*	
	// PULL-UP CONTROL REGISTER (PULCR)
	word_union( PULCR,
		uint16_t PUL15 :1;
		uint16_t reserved : 15;
	);

	// List of all IO BUFFER DRIVE CONTROL REGISTERS (DRVCRs)
	sh7305_IO_buffer_drive_control_register_t DRVCRA;
	sh7305_IO_buffer_drive_control_register_t DRVCRB;
	sh7305_IO_buffer_drive_control_register_t DRVCRC;

*/
} sh7305_pfc_t;

#define SH7305_PFC (*((sh7305_pfc_t *)0xa4050100))





#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_PFC */
