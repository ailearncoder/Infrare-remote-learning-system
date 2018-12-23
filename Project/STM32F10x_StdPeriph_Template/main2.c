
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32_eval.h"
#include <stdio.h>

#define DEBUG_NAME "main"
#define DEBUG_LEVEL 3
#include "debug.h"

#define MAGIC_WORD 0x89

extern void uart_init(void);
extern void pwm_main(void);
extern void pwm_run(uint32_t * time,uint8_t num);
extern int input_main(void);
int flash_data(uint8_t index,uint32_t * data,uint16_t length);

typedef struct
{
    uint8_t index;
    uint8_t resv;
    uint16_t size;
    uint16_t head_time[6];
    uint8_t head_size;
    uint8_t cmd[24];
    uint8_t cmd_size;
    uint32_t data[400];
}__attribute__((aligned (4))) infrared_remote_data;

infrared_remote_data remote_data;

uint16_t exti_index = 0;

uint32_t * const exti_time = remote_data.data;
uint16_t * const yindao_time = remote_data.head_time;
uint8_t * const result_data = remote_data.cmd;

uint8_t exti_data[400] = {0};

void delay_us2(u32 nTimer)
{
	u32 i=0;
	for(i=0;i< nTimer;i++){
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
	}
}

void delay_ms2(u32 nTimer)
{
	u32 i=1000*nTimer;
	delay_us2(i);
}

//引导码 1bit
//用户码 * 2 + 数据码 + 数据反码 (小字节序) 32bit
//停止位 1bit
//4字节 总共 (1 + 32 + 1) * 2 = 68  边沿
//6字节 总共 (1 + 48 + 1) * 2 = 100 边沿

volatile uint8_t is_begin = 0;

void print_remote(infrared_remote_data * remote_data)
{
    log_info_raw("Index:%d\r\n",remote_data->index);
    log_info_raw("Size:%d\r\n",remote_data->size);
    log_info_raw("Head Time:\r\n");
    for(int i=0; i<remote_data->head_size;i++)
    {
        log_info_raw("%d,",remote_data->head_time[i]);
    }
    log_info_raw("\r\nCmd:\r\n");
    for(int i=0; i<remote_data->cmd_size;i++)
    {
        log_info_raw("0x%02X,",remote_data->cmd[i]);
    }
    log_info_raw("\r\nData:\r\n");
    log_info_raw("uint8_t time[]={\r\n");
    for(int i=0; i<remote_data->size;i++)
    {
        log_info_raw("%d,",remote_data->data[i]);
        if(i % 50 == 49)
            log_info_raw("\r\n");
    }
    log_info_raw("};\r\n");
    log_info_raw("\r\n");
}

void analyze(uint8_t scale);

void send_temp_cmd(const uint8_t * temp_cmd)
{
    uint8_t index = 0;
    uint8_t size = 2;
    
    while(size--)
    {
        exti_time[index++] = 4500;
        exti_time[index++] = 4500;
        for(int r = 0;r < 6;r++)
        {
            for(int i=0;i<8;i++)
            {
                uint8_t bit = temp_cmd[r]&(1<<i);
                if(bit)
                {
                    //1
                    exti_time[index++] = 560;
                    exti_time[index++] = 1690;
                }
                else
                {
                    //0
                    exti_time[index++] = 560;
                    exti_time[index++] = 565;
                }
            }
        }
        exti_time[index++] = 560;
        exti_time[index++] = 5000;//结束
    }

#if 0    
    log_info_raw("uint8_t time[]={\r\n");
    for(int i=0;i<index-1;i++)
    {
        log_info_raw("%d,",exti_time[i]);
        if(i%25 == 24)
        {
            log_info_raw("\r\n");
        }
    }
    log_info_raw("%d",565);
    log_info_raw("};\r\n");
#endif
    
    pwm_run(exti_time,index);
}


void time_deal()
{
    log_info_raw("uint8_t time[]={\r\n");
    for(int i=0;i<exti_index - 1;i++)
    {
        exti_time[i] = (exti_time[i] - exti_time[i+1])/9;
        log_info_raw("%d,",exti_time[i]);
        if(i % 50 == 49)
            log_info_raw("\r\n");
    }
    exti_time[exti_index - 1] = 5500;
    log_info_raw("%d",5500);
    log_info_raw("};\r\n");
}

void analyze(uint8_t scale)
{
    int r;
    uint32_t temp;
    uint8_t data;
    uint8_t result_index = 0;
    uint8_t head_size = 2;
    uint8_t exti_offset = 0;
    
    //引导码
    if(exti_time[0 + exti_offset] == 0)
        exti_time[0+ exti_offset] = 0xFFFFFF;
    yindao_time[0] = (exti_time[0 + exti_offset] - exti_time[1 + exti_offset])/scale;
    yindao_time[1] = (exti_time[1 + exti_offset] - exti_time[2 + exti_offset])/scale;
    
    result_index = 0;
    exti_offset = 0;
    while(1)
    {
        data = 0;
        for(r = 1; r < 9;r++)
        {
            temp = (exti_time[r * 2 + exti_offset] - exti_time[r * 2 + 2 + exti_offset])/scale;
            //0 1.125ms
            //1 2.25ms
            //(1125 + 2250)/2=1678
            
            //bit 0
            if(562 < temp && temp < 1678)
            {
                data>>=1;
            }
            //bit 1
            else if(1678 <= temp && temp < 3375)
            {
                data>>=1;
                data|=0x80;
            }
            else
            {
                log_error("%d time error:%d\r\n",r, temp);
            }
        }
        
        //2 4
        //4 6
        //6 8
        //8 10
        //10 12
        //12 14
        //14 16
        //16 18
        
        //18 20
        result_data[result_index++] = data;
        if(20 + exti_offset > exti_index - 1)
        {
            break;//over
        }
        
        temp = (exti_time[18 + exti_offset] - exti_time[20 + exti_offset])/scale;
        if(temp > 3375)
        {
            //本次结束 又一次引导码
            yindao_time[0 + head_size] = (exti_time[0 + exti_offset + 20] - exti_time[1 + exti_offset + 20])/scale;
            yindao_time[1 + head_size] = (exti_time[1 + exti_offset + 20] - exti_time[2 + exti_offset + 20])/scale;
            head_size += 2;
            exti_offset += 20;
            continue;
        }
        
        exti_offset += 16;//1 byte 8 bit
    }
    
    time_deal();
    
    remote_data.head_size = head_size;
    remote_data.cmd_size = result_index;
    remote_data.size = exti_index;
    
    for(r = 0;r< head_size; r+=2)
    {
        log_info_raw("time1: %d us\r\n",yindao_time[r]);
        log_info_raw("time2: %d us\r\n",yindao_time[r+1]);
    }
    
    for(r = 0;r< result_index; r++)
    {
        log_info_raw("%02X,",result_data[r]);
    }
    log_info_raw("\n");
}


static int get_key()
{
    #if DEBUG_USE_RTT == 0
    int r = -1;
    r = USART_GetFlagStatus(EVAL_COM1, USART_FLAG_RXNE);
    if(r == 0)
        return -1;
    r = USART_ReceiveData(EVAL_COM1);
    USART_ClearFlag(EVAL_COM1,USART_FLAG_RXNE);
    return r;
    #else
    return SEGGER_RTT_GetKey();
    #endif
}

static int wait_key()
{
    int r;
    while((r = get_key()) == -1)
    {
    
    }
    return r;
}

static int learn_model()
{
    log_info_raw("Waiting signal...\r\n");
    log_info_raw("Input q to exit.\r\n");
    int r;
    while (is_begin == 0)
    {
        r = get_key();
        if(r == 'q')
        {
            return 0;
        }
    }
    log_info_raw("Signal have been detected.\r\n");
    delay_ms2(500);
    log_info_raw("Signal Num = %d\r\n",exti_index);
    #if 0
    for(r = 0;r< exti_index; r++)
    {
        log_info_raw("%d,",exti_time[r]);
        if(r % 16 == 15)
        {
            log_info_raw("\n");
        }
    }
    log_info_raw("\n");
    
    for(r = 0;r< exti_index; r++)
    {
        log_info_raw("%d,",exti_data[r]);
        if(r % 16 == 15)
        {
            log_info_raw("\n");
        }
    }
    #endif
    log_info_raw("\n");
    analyze(9);     
    SysTick->CTRL=0;
    SysTick->VAL=0;
    is_begin = 0;
    log_info_raw("Save to flash?(y/N)\r\n");
    r = wait_key();
    if(r=='y'||r=='Y')
    {
        log_info_raw("Input a index num to save\r\n");
        int num = 0;
        while(1)
        {
            r = wait_key();
            if(r == '\r' || r == '\n')
            {
                break;
            }
            else
            if('0'<= r&&r <='9')
            {
                num = num * 10 + (r - '0');
                log_info_raw("%c",r);
            }
            else
            if(r == '\b')
            {
                num = num/10;
                log_info_raw("\b");
            }
            else
            {
                log_info_raw("\a");
            }
        }
        log_info_raw("\r\n");
        remote_data.index = num;
        remote_data.resv = MAGIC_WORD;
        if(flash_data(num,(uint32_t *)(&remote_data),sizeof(remote_data)) == 0)
        {
            log_info_raw("Save %d OK!\r\n",num);
        }
        else
        {
            log_info_raw("Save %d Failed!\r\n",num);
        }
    }
    if(r=='n'||r=='N')
    {
        
    }
    return 1;
}

extern __IO uint32_t* flash_read(uint8_t index);
int read_model()
{
    log_info_raw("Please input index to read\r\n");
    log_info_raw("Input q to exit.\r\n");
    int r;
    int num = 0;
    while(1)
    {
        r = get_key();
        if(r == '\r' || r == '\n')
        {
            break;
        }
        else
        if('0'<= r&&r <='9')
        {
            num = num * 10 + (r - '0');
            log_info_raw("%c",r);
        }
        else
        if(r == '\b')
        {
            num = num/10;
            log_info_raw("\b");
        }
        else if(r == 'q')
        {
            return 0;
        }
    }
    log_info_raw("\r\n");
    infrared_remote_data * remote_date = (infrared_remote_data *)flash_read(num);
    if(remote_date)
    {
        if(remote_date->resv != MAGIC_WORD)
        {
            log_info_raw("Read %d Not Exist!\r\n",num);
        }
        else
        {
            print_remote(remote_date);
            log_info_raw("Read %d OK!\r\n",num);
        }
    }
    else
    {
        log_info_raw("Read %d Failed!\r\n",num);
    }
    return 1;
}

int work_model()
{
    log_info_raw("Please input index to send\r\n");
    log_info_raw("Input q to exit.\r\n");
    int r;
    int num = 0;
    while(1)
    {
        r = get_key();
        if(r == '\r' || r == '\n')
        {
            break;
        }
        else
        if('0'<= r&&r <='9')
        {
            num = num * 10 + (r - '0');
            log_info_raw("%c",r);
        }
        else
        if(r == '\b')
        {
            num = num/10;
            log_info_raw("\b");
        }
        else if(r == 'q')
        {
            return 0;
        }
    }
    log_info_raw("\r\n");
    infrared_remote_data * remote_date = (infrared_remote_data *)flash_read(num);
    if(remote_date)
    {
        if(remote_date->resv != MAGIC_WORD)
        {
            log_info_raw("Send %d Not Exist!\r\n",num);
        }
        else
        {   
            pwm_run(remote_date->data,remote_date->size);
            log_info_raw("Send %d OK!\r\n",num);
        }
    }
    else
    {
        log_info_raw("Send %d Failed!\r\n",num);
    }
    return 1;
}

int main(void)
{
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f10x_xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f10x.c file
     */     

  /* Initialize LEDs, Key Button, LCD and COM port(USART) available on
     STM3210X-EVAL board ******************************************************/

    uart_init();
    log_init();
    log_info_raw("Welcome\r\n");
    STM_EVAL_LEDInit(LED1);
    //STM_EVAL_LEDInit(LED2);

    //STM_EVAL_LEDOn(LED1);
    //STM_EVAL_LEDOn(LED2);

    /* Add your application code here
     */
    
    pwm_main();
    input_main();
    
    //4D,B2,FD,02,34,CB,4D,B2,FD,02,34,CB//20度
    //const uint8_t temp_cmd[]={0x4D,0xB2,0xFD,0x02,0x34,0xCB};
    //send_temp_cmd(temp_cmd);
    
    /* Infinite loop */
    while (1)
    {
        log_info_raw("---------------------------\r\n");
        log_info_raw("\tPlease select model\r\n");
        log_info_raw("\t\t1. Learn\r\n");
        log_info_raw("\t\t2. Work\r\n");
        log_info_raw("\t\t3. Read\r\n");
        log_info_raw("---------------------------\r\n");
        int r = wait_key();
        switch(r)
        {
            case '1':
                while(learn_model())
                {
                
                }
                break;
            case '2':
                while(work_model())
                {
                
                }
                break;
           case '3':
                while(read_model())
                {
                
                }
                break;
        }
    }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
      
  }
}
#endif

uint8_t read_pin()
{
    uint8_t i=0;
    uint8_t num = 0;
    for(i = 0;i < 7;i++)
    {
        if((GPIOA->IDR) & GPIO_Pin_0);
            num++;
    }
    return (num > 3);
}

/**
  * @}
  */

/**
  * @brief  This function handles External line 0 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void)
{
  if (EXTI_GetITStatus(EXTI_Line0) != RESET)
  {
    if(is_begin == 0)
    {
        is_begin = 1;
        SysTick->LOAD=0xFFFFFF;
        SysTick->CTRL=0X01;
        SysTick->VAL=0;
        exti_index = 0;
    }
    exti_time[exti_index++] = SysTick->VAL;
    exti_data[exti_index] = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0);
    /* Clear the  EXTI line 0 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line0);
  }
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
