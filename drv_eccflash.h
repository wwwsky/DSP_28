 
#ifndef SOURCE_DRIVER_DRV_ECCFLASH_H_
#define SOURCE_DRIVER_DRV_ECCFLASH_H_
 
#ifndef     DRV_ECCFLASH_GOLBAL_
#define     DRV_ECCFLASH_EXT
#else
#define     DRV_ECCFLASH_EXT        extern
#endif
 
 
 
 
 
 
#define PUMPREQUEST *(unsigned long*)(0x00050024)
 
 
 
#define BOne_SectorAB_start        0xFE000
#define BOne_SectorAB_End          0xFFFFF//1FFF
 
#define BOne_SectorAA_start        0xFC000
#define BOne_SectorAA_End          0xFDFFF//1FFF
 
#define BOne_SectorZ_start         0xFA000
#define BOne_SectorZ_End           0xFBFFF//1FFF
 
#define BOne_SectorY_start         0xF8000
#define BOne_SectorY_End           0xF9FFF//1FFF
 
#define BOne_SectorX_start         0xF0000
#define BOne_SectorX_End           0xF7FFF//7FFF
 
#define BOne_SectorW_start         0xE8000
#define BOne_SectorW_End           0xEFFFF//7FFF
 
#define BOne_SectorV_start         0xE0000
#define BOne_SectorV_End           0xE7FFF//7FFF ??
 
#define BOne_SectorU_start         0xD8000
#define BOne_SectorU_End           0xDFFFF//7FFF
 
#define BOne_SectorT_start         0xD0000
#define BOne_SectorT_End           0xD7FFF//7FFF ??
 
#define BOne_SectorS_start         0xC8000
#define BOne_SectorS_End           0xCFFFF//7FFF
 
#define BOne_SectorR_start         0xC6000
#define BOne_SectorR_End           0xC7FFF//1FFF
 
#define BOne_SectorQ_start         0xC4000
#define BOne_SectorQ_End           0xC5FFF//1FFF
 
#define BOne_SectorP_start         0xC2000
#define BOne_SectorP_End           0xC3FFF//1FFF
 
#define BOne_SectorO_start         0xC0000 //8KiB
#define BOne_SectorO_End           0xC1FFF//1FFF
 
// bank 1  =0x40000 =256KiB
 
#define Bzero_SectorN_start        0xBE000
#define Bzero_SectorN_End          0xBFFFF//1FFF
 
#define Bzero_SectorM_start        0xBC000
#define Bzero_SectorM_End          0xBDFFF//1FFF
 
#define Bzero_SectorL_start        0xBA000
#define Bzero_SectorL_End          0xBBFFF//1FFF
 
#define Bzero_SectorK_start        0xB8000
#define Bzero_SectorK_End          0xB9FFF//1FFF
 
#define Bzero_SectorJ_start        0xB0000
#define Bzero_SectorJ_End          0xB7FFF//7FFF
 
#define Bzero_SectorI_start        0xA8000
#define Bzero_SectorI_End          0xAFFFF//7FFF
 
#define Bzero_SectorH_start        0xA0000
#define Bzero_SectorH_End          0xA7FFF//7FFF
 
#define Bzero_SectorG_start        0x98000
#define Bzero_SectorG_End          0x9FFFF//7FFF
 
#define Bzero_SectorF_start        0x90000
#define Bzero_SectorF_End          0x97FFF//7FFF
 
#define Bzero_SectorE_start        0x88000
#define Bzero_SectorE_End          0x8FFFF//7FFF
 
#define Bzero_SectorD_start        0x86000
#define Bzero_SectorD_End          0x87FFF//1FFF
 
#define Bzero_SectorC_start        0x84000
#define Bzero_SectorC_End          0x85FFF//1FFF
 
#define Bzero_SectorB_start        0x82000
#define Bzero_SectorB_End          0x83FFF//1FFF
 
#define Bzero_SectorA_start        0x80000
#define Bzero_SectorA_End          0x81FFF //1FFF
 
 
#define Bzero_16KSector_u32length  0x1000
#define Bzero_64KSector_u32length  0x4000
 
#define BOne_16KSector_u32length   0x1000
#define BOne_64KSector_u32length   0x4000
 
 
 
extern int eccflash_write_16bit(uint16_t *dat ,uint32_t len);
 
#endif /* SOURCE_DRIVER_DRV_ECCFLASH_H_ */