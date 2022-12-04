//---
// gint:mpu:bsc - Bus State Controller
//---

#ifndef GINT_MPU_BSC
#define GINT_MPU_BSC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

//---
// SH7705 But State Controller. Refer to:
//   Renesas SH7705 Group Hardware Manual
//   Section 7: Bus State Controller (BSC)
//---

typedef volatile lword_union(sh7705_bsc_CSnBCR_t,
	uint32_t	:2;
	uint32_t IWW	:2;
	uint32_t	:1;
	uint32_t IWRWD	:2;
	uint32_t	:1;
	uint32_t IWRWS	:2;
	uint32_t	:1;
	uint32_t IWRRD	:2;
	uint32_t	:1;
	uint32_t IWRRS	:2;
	uint32_t	:1;
	uint32_t TYPE	:3;
	uint32_t	:1;
	uint32_t BSZ	:2;
	uint32_t	:9;
);

typedef volatile lword_union(sh7705_bsc_CSnWCR_t,
	uint32_t	:13;
	uint32_t WW :3;
	uint32_t 	:3;
	uint32_t SW	:2;
	uint32_t WR	:4;
	uint32_t WM	:1;
	uint32_t	:4;
	uint32_t HW	:2;
);


typedef volatile struct
{
	lword_union(CMNCR,
		uint32_t :24;	/* reserved */
		uint32_t DMAIW : 2; /* Wait State*/
		uint32_t DMAIWA : 1; /* Method of Inserting Wait state bewteen cycles */ 
		uint32_t :1; /* reserved */
		uint32_t ENDIAN : 1; /* Endian Flag*/
		uint32_t :1; /* reserved */
		uint32_t HIZMEM : 1; /* High Z memory Control*/
		uint32_t HIZCNT : 1; /* High Z Control*/
	);

	sh7705_bsc_CSnBCR_t CS0BCR;
	sh7705_bsc_CSnBCR_t CS2BCR;
	sh7705_bsc_CSnBCR_t CS3BCR;
	sh7705_bsc_CSnBCR_t CS4BCR;
	sh7705_bsc_CSnBCR_t CS5ABCR;
	sh7705_bsc_CSnBCR_t CS5BBCR;
	sh7705_bsc_CSnBCR_t CS6ABCR;
	sh7705_bsc_CSnBCR_t CS6BBCR;


	sh7705_bsc_CSnWCR_t CS0WCR;
	sh7705_bsc_CSnWCR_t CS2WCR;
	sh7705_bsc_CSnWCR_t CS3WCR;
	sh7705_bsc_CSnWCR_t CS4WCR;
	sh7705_bsc_CSnWCR_t CS5AWCR;
	sh7705_bsc_CSnWCR_t CS5BWCR;
	sh7705_bsc_CSnWCR_t CS6AWCR;	
	sh7705_bsc_CSnWCR_t CS6BWCR;

	/* TODO : there are several other registers not described yet
	as they are not used for overclocking the SH7705 processor.
	To be done later on when needed.*/


} GPACKED(4) sh7705_bsc_t;

#define SH7705_BSC (*(sh7705_bsc_t *)0xa4fd0000)


//---
// SH7305 But State Controller. Refer to:
//   Renesas SH7730 Group Hardware Manual
//   Section 11: Bus State Controller (BSC)
//---

typedef volatile lword_union(sh7305_bsc_CSnBCR_t,
	uint32_t	:1;
	uint32_t IWW	:3;
	uint32_t IWRWD	:3;
	uint32_t IWRWS	:3;
	uint32_t IWRRD	:3;
	uint32_t IWRRS	:3;
	uint32_t TYPE	:4;
	uint32_t	:1;
	uint32_t BSZ	:2;
	uint32_t	:9;
);

typedef volatile lword_union(sh7305_bsc_CSnWCR_06A6B_t,
	uint32_t	:11;
	uint32_t BAS	:1;
	uint32_t	:1;
	uint32_t WW	:3;
	uint32_t ADRSFIX:1;
	uint32_t	:2;
	uint32_t SW	:2;
	uint32_t WR	:4;
	uint32_t WM	:1;
	uint32_t	:4;
	uint32_t HW	:2;
);

typedef volatile lword_union(sh7305_bsc_CSnWCR_45A5B_t,
	uint32_t	:11;
	uint32_t BAS	:1;
	uint32_t	:1;
	uint32_t WW	:3;
	uint32_t	:3;
	uint32_t SW	:2;
	uint32_t WR	:4;
	uint32_t WM	:1;
	uint32_t	:4;
	uint32_t HW	:2;
);

typedef volatile struct
{
	lword_union(CMNCR,
		uint32_t	:6;
		uint32_t CKOSTP	:1;
		uint32_t CKODRV	:1;
		uint32_t	:7;
		uint32_t DMSTP	:1;
		uint32_t	:1;
		uint32_t BSD	:1;
		uint32_t MAP	:2;
		uint32_t BLOCK	:1;
		uint32_t	:7;
		uint32_t ENDIAN	:1;
		uint32_t	:1;
		uint32_t HIZMEM	:1;
		uint32_t HIZCNT	:1;
	);

	sh7305_bsc_CSnBCR_t CS0BCR;
	sh7305_bsc_CSnBCR_t CS2BCR;
	sh7305_bsc_CSnBCR_t CS3BCR;
	sh7305_bsc_CSnBCR_t CS4BCR;
	sh7305_bsc_CSnBCR_t CS5ABCR;
	sh7305_bsc_CSnBCR_t CS5BBCR;
	sh7305_bsc_CSnBCR_t CS6ABCR;
	sh7305_bsc_CSnBCR_t CS6BBCR;

	sh7305_bsc_CSnWCR_06A6B_t  CS0WCR;
	lword_union(CS2WCR,
		uint32_t	:8;
		uint32_t BW	:2;
		uint32_t PMD	:1;
		uint32_t BAS	:1;
		uint32_t	:1;
		uint32_t WW	:3;
		uint32_t	:3;
		uint32_t SW	:2;
		uint32_t WR	:4;
		uint32_t WM	:1;
		uint32_t	:4;
		uint32_t HW	:2;
	);
	lword_union(CS3WCR,
		uint32_t	:17;
		uint32_t TRP	:2;
		uint32_t	:1;
		uint32_t TRCD	:2;
		uint32_t	:1;
		uint32_t A3CL	:2;
		uint32_t	:2;
		uint32_t TRWL	:2;
		uint32_t	:1;
		uint32_t TRC	:2;
	);
	sh7305_bsc_CSnWCR_45A5B_t  CS4WCR;
	sh7305_bsc_CSnWCR_45A5B_t  CS5AWCR;
	sh7305_bsc_CSnWCR_45A5B_t  CS5BWCR;
	sh7305_bsc_CSnWCR_06A6B_t  CS6AWCR;
	sh7305_bsc_CSnWCR_06A6B_t  CS6BWCR;

	lword_union(SDCR,
		uint32_t	:11;
		uint32_t A2ROW	:2;
		uint32_t	:1;
		uint32_t A2COL	:2;
		uint32_t	:4;
		uint32_t RFSH 	:1;
		uint32_t RMODE	:1;
		uint32_t PDOWN	:1;
		uint32_t BACTV	:1;
		uint32_t	:3;
		uint32_t A3ROW	:2;
		uint32_t	:1;
		uint32_t A3COL	:2;
	);

	uint32_t RTCSR;
	uint32_t RTCNT;
	uint32_t RTCOR;

} GPACKED(4) sh7305_bsc_t;

#define SH7305_BSC (*(sh7305_bsc_t *)0xfec10000)

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_BSC */
