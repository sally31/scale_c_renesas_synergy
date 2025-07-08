/* HAL-only entry function */
#include "hal_data.h"
#include <stdio.h>
extern void initialise_monitor_handles(void); // for printf()

#define sck IOPORT_PORT_06_PIN_08
#define DOUT IOPORT_PORT_01_PIN_12

#define scale  103.1f

ioport_level_t pin_value;


static long get_data(void)
{
    SSP_CRITICAL_SECTION_DEFINE;  // use interrupt mask
    SSP_CRITICAL_SECTION_ENTER; // disable interrupt

    long data = 0;
    while(1)
    {
        g_ioport.p_api->pinRead(DOUT, &pin_value);
        if(pin_value == IOPORT_LEVEL_LOW) break;
    }

    for (int i=0;i<24;i++)
    {
        g_ioport.p_api->pinWrite(sck, IOPORT_LEVEL_HIGH);
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);



        g_ioport.p_api->pinRead(DOUT, &pin_value);
        if(pin_value == IOPORT_LEVEL_LOW)
        {
            data = (data<<1) | 0;
        }
        else
        {
            data = (data<<1) | 1;
        }

        g_ioport.p_api->pinWrite(sck, IOPORT_LEVEL_LOW);
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);

    }

        g_ioport.p_api->pinWrite(sck, IOPORT_LEVEL_HIGH);
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);


        g_ioport.p_api->pinWrite(sck, IOPORT_LEVEL_LOW);
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);

        //data = data^0x800000;

        SSP_CRITICAL_SECTION_EXIT;  // enable interrupt
        return data;
}


void hal_entry(void)
{
    /* TODO: add your own code here */
    initialise_monitor_handles(); // for printf()
    ssp_err_t   status;
    int         msg_len = 0;
    long offset_sum = 0;
    char msg[200];
    status = g_uart0.p_api->open(g_uart0.p_ctrl, g_uart0.p_cfg);
    if (status != SSP_SUCCESS)
    {
        // 送信エラー
        __BKPT(0);  // ブレークして原因を調査
    }
    for (int i = 0; i < 10; i++)
    {
        offset_sum += get_data();
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
    long offset = offset_sum / 10;

    while(1)
    {
        long sum = 0;
        for (int i = 0; i < 10; i++)
        {
            sum += get_data();
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        }
        long data = sum / 10;

        float weight = (float)(data - offset) / scale;

        msg_len = snprintf(msg, sizeof(msg), "%d.%02d\n", (int)weight, (int)(weight * 100) % 100);
        if(msg_len > 0)
        {
            status = g_uart0.p_api->write(g_uart0.p_ctrl, (uint8_t*)msg, (uint32_t)msg_len);
        }
        //printf("%d.%02d\n", (int)weight, (int)(weight * 100) % 100);
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }


}

