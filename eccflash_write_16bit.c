 
#define     DRV_ECCFLASH_GOLBAL_
 
#include "Types.h"
#include "drv.h"
#include "drv_eccflash.h"
#include "app.h"
#include "f021.h"
#include "Constants.h "
#include "F021_F2837xS_C28x.h"
#include "F28x_Project.h"
 
#pragma CODE_SECTION(eccflash_write_16bit, ".TI.ramfunc");//该函数必须在SRAM中执行
 
// return 1:fail ,0:ok
int eccflash_write_16bit(uint16_t *dat ,uint32_t len)
{
    volatile unsigned long u32Index = 0;
    volatile unsigned long i = 0;
    Fapi_StatusType oReturnCheck;
    volatile Fapi_FlashStatusType oFlashStatus;
 
    uint32_t Get_sector[14]= //BANK 1
    {
        BOne_SectorO_start,
        BOne_SectorP_start,
        BOne_SectorQ_start,
        BOne_SectorR_start,
        BOne_SectorS_start,
        BOne_SectorT_start,
        BOne_SectorU_start,
        BOne_SectorV_start,
        BOne_SectorW_start,
        BOne_SectorX_start,
        BOne_SectorY_start,
        BOne_SectorZ_start,
        BOne_SectorAA_start,
        BOne_SectorAB_start,
    };
//step 1
    EALLOW;// Bank0 Erase Program
    PUMPREQUEST = 0x5A5A0001; // Give pump ownership to FMC1
    oReturnCheck = Fapi_initializeAPI(F021_CPU0_W1_BASE_ADDRESS, 150); //设置flash 状态机 ，设置频率150MHz
    if(oReturnCheck != Fapi_Status_Success)goto end_prg;
//step 2
    oReturnCheck = Fapi_setActiveFlashBank(Fapi_FlashBank1);
    if(oReturnCheck != Fapi_Status_Success)goto end_prg;
//step 3
    for(i=0;i<14;i++) // 擦除bank1 14个扇区
    {
     oReturnCheck = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector,(uint32 *)Get_sector[i]);// Erase Sector x
     while(Fapi_checkFsmForReady() != Fapi_Status_FsmReady){}// Wait until FSM is done with erase sector operation
    }
//step 4
    for(i=0, u32Index = BOne_SectorO_start;(u32Index < (BOne_SectorO_start + len))&&(oReturnCheck == Fapi_Status_Success);)// 连续写入flash 扇区
    {
     oReturnCheck = Fapi_issueProgrammingCommand((uint32 *)u32Index,&dat[i],8,0,0,Fapi_AutoEccGeneration);
     while(Fapi_checkFsmForReady() == Fapi_Status_FsmBusy);
     if(oReturnCheck != Fapi_Status_Success) goto end_prg;
     oFlashStatus = Fapi_getFsmStatus();
     i+= 8; u32Index+= 8;
    }
//step 5
    oReturnCheck = Fapi_issueAsyncCommand(Fapi_ClearMore);
    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady){}// Wait until the Flash program operation is over
end_prg:
     PUMPREQUEST = 0x5A5A0000;// Give pump ownership back to FMC0
     EDIS;
     return oReturnCheck;
}