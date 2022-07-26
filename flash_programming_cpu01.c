//flash_programming_cpu01.c
//#########################################################################
// FILE: flash_programming_cpu01.c
// TITLE: Flash Programming Example for F2837xD.
//
//! \addtogroup dual_example_list
//! <h1> Flash Programming </h1>
//!
//! This example demonstrates F021 Flash API usage.
//
//#########################################################################
// $TI Release: F2837xD Support Library v170 $
// $Release Date: Mon Sep 21 16:52:10 CDT 2015 $
// $Copyright: Copyright (C) 2013-2015 Texas Instruments Incorporated -
// http://www.ti.com/ ALL RIGHTS RESERVED $
//#########################################################################

#include "F28x_Project.h" // Device Headerfile and Examples Include	File
#include "F2837xD_Ipc_drivers.h"

#include <string.h>

//Include Flash API example header file
#include "flash_programming_c28.h"

#define ENTRYADDR 0x88000

extern void lightflash(void);

//*************************************************************************
// FILE Flash API include file
//*************************************************************************
#include "F021_F2837xD_C28x.h"

//Data/Program Buffer used for testing the flash API functions
#define WORDS_IN_FLASH_BUFFER 3608 // Programming data buffer, words

uint16 Buffer[WORDS_IN_FLASH_BUFFER + 1];
uint32 *Buffer32 = (uint32 *)Buffer;

#pragma DATA_SECTION(pbuffer , "PDATASAVE");
uint16 pbuffer[WORDS_IN_FLASH_BUFFER]= {
#include "P_DATA.h"
};

//*************************************************************************
// Prototype of the functions used in this example
//*************************************************************************
void Example_CallFlashAPI(void);

//*************************************************************************
// This is an example code demonstrating F021 Flash API usage.
// This code is in Flash
//*************************************************************************
void main(void)
{
// Step 1. Initialize System Control:
// Enable Peripheral Clocks
// This example function is found in the F2837xD_SysCtrl.c file.
InitSysCtrl();

IPCBootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH);

InitGpio();

InitPieCtrl();

IER = 0x0000;
IFR = 0x0000;

InitPieVectTable();

EINT; // Enable Global interrupt INTM

// Jump to RAM and call the Flash API functions
Example_CallFlashAPI();

DELAY_US(100000);

static void (*APPEntry)(void);

APPEntry = (void (*)(void))(ENTRYADDR);

ESTOP0;

(*APPEntry)();

while(1);

}

//*************************************************************************
// Example_CallFlashAPI
//
// This function will interface to the flash API.
// Flash API functions used in this function are executed from RAM
//*************************************************************************
#pragma CODE_SECTION(Example_CallFlashAPI , "ramfuncs");
void Example_CallFlashAPI(void)
{
 uint32 u32Index = 0;
 uint16 i = 0;

 Fapi_StatusType oReturnCheck;
 volatile Fapi_FlashStatusType oFlashStatus;

 // Gain pump semaphore
 SeizeFlashPump();

 EALLOW;
 Flash0EccRegs.ECC_ENABLE.bit.ENABLE = 0x0;
 EDIS;

 EALLOW;

 oReturnCheck = Fapi_initializeAPI(F021_CPU0_BASE_ADDRESS, 200);//for now keeping it out

 if(oReturnCheck != Fapi_Status_Success)
 {
 // Check Flash API documentation for possible errors
 while(1);
 }

 oReturnCheck = Fapi_setActiveFlashBank(Fapi_FlashBank0);
 if(oReturnCheck != Fapi_Status_Success)
 {
 // Check Flash API documentation for possible errors
 while(1);
 }

 oReturnCheck = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector,
 (uint32 *)ENTRYADDR);

 while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady){}

 // In this case just fill a buffer with data to program into the flash.
 for(i=0, u32Index = ENTRYADDR;

 (u32Index < (ENTRYADDR + WORDS_IN_FLASH_BUFFER))&& (oReturnCheck ==Fapi_Status_Success);

 i=i+1, u32Index = u32Index + 1)
 {
 oReturnCheck = Fapi_issueProgrammingCommand((uint32 *)u32Index,&pbuffer[i],
 1,
 0,
 0,
 Fapi_DataOnly);

 while(Fapi_checkFsmForReady() == Fapi_Status_FsmBusy);

 if(oReturnCheck != Fapi_Status_Success)
 {
 // Check Flash API documentation for possible errors
 while(1);
 }

 // Read FMSTAT register contents to know the status of FSM after
 // program command for any debug
 oFlashStatus = Fapi_getFsmStatus();
 }

 // Leave control over flash pump
 ReleaseFlashPump();
 }
	