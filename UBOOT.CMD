 
 
 
 
MEMORY
{
PAGE 0 :  /* Program Memory */
          /* Memory (RAM/FLASH) blocks can be moved to PAGE1 for data allocation */
          /* BEGIN is used for the "boot to Flash" bootloader mode   */
   BEGIN           	: origin = 0x080000, length = 0x000002
   RAMD0           	: origin = 0x00B000, length = 0x000800
   RAMLS0123        : origin = 0x008000, length = 0x002000	
   RAMLS4      		: origin = 0x00A000, length = 0x000800
   RESET           	: origin = 0x3FFFC0, length = 0x000002
 
   /* Flash sectors */
   FLASHA           : origin = 0x080002, length = 0x001FFE	/* on-chip Flash */
   FLASHB           : origin = 0x082000, length = 0x002000	/* on-chip Flash */
   FLASHC           : origin = 0x084000, length = 0x002000	/* on-chip Flash */
   FLASHD           : origin = 0x086000, length = 0x002000	/* on-chip Flash */
   FLASHE           : origin = 0x088000, length = 0x008000	/* on-chip Flash */
   FLASHF           : origin = 0x090000, length = 0x008000	/* on-chip Flash */
   FLASHG           : origin = 0x098000, length = 0x008000	/* on-chip Flash */
   FLASHH           : origin = 0x0A0000, length = 0x008000	/* on-chip Flash */
   FLASHI           : origin = 0x0A8000, length = 0x008000	/* on-chip Flash */
   FLASHJ           : origin = 0x0B0000, length = 0x008000	/* on-chip Flash */
   FLASHK           : origin = 0x0B8000, length = 0x002000	/* on-chip Flash */
   FLASHL           : origin = 0x0BA000, length = 0x002000	/* on-chip Flash */
   FLASHM           : origin = 0x0BC000, length = 0x002000	/* on-chip Flash */
   FLASHN           : origin = 0x0BE000, length = 0x002000	/* on-chip Flash */
 
PAGE 1 : /* Data Memory */
         /* Memory (RAM/FLASH) blocks can be moved to PAGE0 for program allocation */
 
   BOOT_RSVD       : origin = 0x000002, length = 0x000120     /* Part of M0, BOOT rom will use this for stack */
   RAMM01          : origin = 0x000122, length = 0x0006DE     
   RAMD1           : origin = 0x00B800, length = 0x000800
 
   RAMLS5      : origin = 0x00A800, length = 0x000800
 
   RAMGS0      : origin = 0x00C000, length = 0x001000
   RAMGS1      : origin = 0x00D000, length = 0x001000
   RAMGS2      : origin = 0x00E000, length = 0x001000
   RAMGS3      : origin = 0x00F000, length = 0x001000
   RAMGS4      : origin = 0x010000, length = 0x001000
   RAMGS5      : origin = 0x011000, length = 0x001000
   RAMGS6to10  : origin = 0x012000, length = 0x005000
   RAMGS11     : origin = 0x017000, length = 0x001000
}
 
 
SECTIONS
{
   /* Allocate program areas: */
   .cinit              : > FLASHA      PAGE = 0, ALIGN(4)
   .pinit              : > FLASHA,     PAGE = 0, ALIGN(4)
   .text               : >> FLASHA|FLASHB | FLASHC | FLASHD | FLASHE|FLASHF      PAGE = 0, ALIGN(4)
   codestart           : > BEGIN       PAGE = 0, ALIGN(4)
 
   /* Allocate uninitalized data sections: */
   .stack              : > RAMM01       PAGE = 1						
   .ebss               : >> RAMLS5 | RAMGS0 | RAMGS1 | RAMGS2|RAMGS3|RAMGS4|RAMGS5|RAMGS6to10| RAMGS11   PAGE = 1	
   .esysmem            : > RAMLS5       PAGE = 1
 
   /* Initalized sections go in Flash */
   .econst             : >> FLASHG | FLASHH | FLASHI      PAGE = 0, ALIGN(4)
   .switch             : > FLASHJ      PAGE = 0, ALIGN(4)
 
   .reset              : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */
 
#ifdef __TI_COMPILER_VERSION__
   #if __TI_COMPILER_VERSION__ >= 15009000
        GROUP
    {
        .TI.ramfunc
        {-l F021_API_F2837xS_FPU32.lib} //注意：该库文件 SRAM  中
 
    } LOAD = FLASHK ,
                         RUN = RAMLS0123 ,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_SIZE(_RamfuncsLoadSize),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         RUN_SIZE(_RamfuncsRunSize),
                         RUN_END(_RamfuncsRunEnd),
                         PAGE = 0, ALIGN(4)
   #else
   ramfuncs            : LOAD = FLASHK,
                         RUN = RAMLS0123,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_SIZE(_RamfuncsLoadSize),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         RUN_SIZE(_RamfuncsRunSize),
                         RUN_END(_RamfuncsRunEnd),
                         PAGE = 0, ALIGN(4)
   #endif
#endif
 
   ramgnet          : > RAMGS6to10, PAGE = 1
   .cio             : >> RAMGS2,    PAGE = 1
 
   /* The following section definitions are for SDFM examples */
   Filter1_RegsFile : > RAMGS1,	PAGE = 1, fill=0x1111
   Filter2_RegsFile : > RAMGS2,	PAGE = 1, fill=0x2222
   Filter3_RegsFile : > RAMGS3,	PAGE = 1, fill=0x3333
   Filter4_RegsFile : > RAMGS4,	PAGE = 1, fill=0x4444
   Difference_RegsFile : >RAMGS5, 	PAGE = 1, fill=0x3333
}
 
/*
//===========================================================================
// End of file.
//===========================================================================
*/