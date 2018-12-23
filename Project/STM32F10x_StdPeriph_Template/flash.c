#include "flash.h"
#include "stm32f10x.h"

#define DEBUG_NAME "flash"
#define DEBUG_LEVEL 3
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;

/* Private define ------------------------------------------------------------*/
/* Define the STM32F10x FLASH Page Size depending on the used STM32 device */
#if defined (STM32F10X_HD) || defined (STM32F10X_HD_VL) || defined (STM32F10X_CL) || defined (STM32F10X_XL)
  #define FLASH_PAGE_SIZE    ((uint16_t)0x800)
#else
  #define FLASH_PAGE_SIZE    ((uint16_t)0x400)
#endif

#define BANK1_WRITE_START_ADDR  ((uint32_t)0x0804E000)
#define BANK1_WRITE_END_ADDR    ((uint32_t)0x08080000)

#ifdef STM32F10X_XL
 #define BANK2_WRITE_START_ADDR   ((uint32_t)0x08088000)
 #define BANK2_WRITE_END_ADDR     ((uint32_t)0x0808C000)
#endif /* STM32F10X_XL */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/  
uint32_t EraseCounter = 0x00, Address = 0x00;
uint32_t Data = 0x3210ABCD;
uint32_t NbrOfPage = 0x00;
volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
volatile TestStatus MemoryProgramStatus = PASSED;

#ifdef STM32F10X_XL
volatile TestStatus MemoryProgramStatus2 = PASSED;
#endif /* STM32F10X_XL */

/* Private function prototypes -----------------------------------------------*/   
/* Private functions ---------------------------------------------------------*/

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
 
__IO uint32_t* flash_read(uint8_t index)
{
    /* Define the number of page to be erased */
  NbrOfPage = (BANK1_WRITE_END_ADDR - BANK1_WRITE_START_ADDR) / FLASH_PAGE_SIZE;
  if (index >= NbrOfPage)
  {
      log_error("index too large:%d max:%d\r\n",index,NbrOfPage);
      return NULL;
  }
  
  EraseCounter = index;
  /* Check the correctness of written data */
  Address = BANK1_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter);
  return (__IO uint32_t*) Address;
}

int flash_data(uint8_t index,uint32_t * data,uint16_t length)
{
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f10x_xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f10x.c file
     */     

/* Porgram FLASH Bank1 ********************************************************/       
  /* Unlock the Flash Bank1 Program Erase controller */
  FLASH_UnlockBank1();

  /* Define the number of page to be erased */
  NbrOfPage = (BANK1_WRITE_END_ADDR - BANK1_WRITE_START_ADDR) / FLASH_PAGE_SIZE;

  /* Clear All pending flags */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	
    
  if (index >= NbrOfPage)
  {
      log_error("index too large:%d max:%d\r\n",index,NbrOfPage);
      return -1;
  }
  
  EraseCounter = index;
  
  /* Erase the FLASH pages */
  FLASHStatus = FLASH_ErasePage(BANK1_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter));

  if(FLASHStatus != FLASH_COMPLETE)
  {
      log_error("Page Index %d Erase Failed! error:%d",index,FLASHStatus);
      return -1;
  }
  
  /* Program Flash Bank1 */
  Address = BANK1_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter);

  for(uint16_t i=0;i<length && i<500;i++)
  {
    FLASHStatus = FLASH_ProgramWord(Address, data[i]);
    Address = Address + 4;
    if(FLASHStatus != FLASH_COMPLETE)
    {
      log_error("Page Index %d Progrem Failed! error:%d",index,FLASHStatus);
      return -1;
    }
  }

  FLASH_LockBank1();
  
  /* Check the correctness of written data */
  Address = BANK1_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter);
  
  for(uint16_t i=0;i<length && i<500;i++)
  {
    if((*(__IO uint32_t*) Address) != data[i])
    {
      MemoryProgramStatus = FAILED;
      log_error("Page Index %d Check Failed! data:%08X error:%08X",index,data[i],(*(__IO uint32_t*) Address));
      return -1;
    }
    Address += 4;
  }
 
  log_info("EraseCounter:%d\r\n",EraseCounter);
  log_info("Address:0x%08X\r\n",Address);
  log_info("NbrOfPage:%d\r\n",NbrOfPage);
  log_info("MemoryProgramStatus:%d\r\n",MemoryProgramStatus);
  log_info("FLASHStatus:%d\r\n",FLASHStatus);
  
  return 0;
}
