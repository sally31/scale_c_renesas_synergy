/* HAL-only entry function */
#include "hal_data.h"
#include <stdio.h>
extern void initialise_monitor_handles(void); // for printf()

// HX711 pin configuration
#define sck  IOPORT_PORT_06_PIN_08   // Clock pin
#define DOUT IOPORT_PORT_01_PIN_12   // Data pin

// Scale adjustment
#define scale  107.4f
float scale_adj = -4.3f;  // Correction value (loaded from EEPROM)

// Error counter and communication buffer
int err_count = 0;
uint8_t i2c_buf[16] = {0,};
ssp_err_t   status;

// Button states
ioport_level_t  s4_current_value = IOPORT_LEVEL_HIGH;  // S4 button
ioport_level_t  s5_current_value = IOPORT_LEVEL_HIGH;  // S5 button
ioport_level_t pin_value;

//---------------- EEPROM data structure ----------------//
struct data_format{
    float   data1;  // Scale correction value
    int     data2;  // Reserved
};
union data_buf{
    struct data_format  form;
    uint8_t buf[16];   // Byte-level buffer for I2C transfer
};
union data_buf  eeprom_data;

//---------------- Function prototypes ----------------//
static long get_data(void);            // Read 24-bit data from HX711
void eeprom_write(uint16_t mem_addr);  // Write to EEPROM
void eeprom_read(uint16_t mem_addr);   // Read from EEPROM


//---------------------------------------------------------------------//
// Main entry function
//---------------------------------------------------------------------//
void hal_entry(void)
{
    initialise_monitor_handles(); // Enable printf() output

    int     msg_len = 0;
    long offset_sum = 0;
    char msg[50];
    uint8_t i2c_addr = 0x50;          // EEPROM I2C address
    uint16_t eeprom_mem_addr = 0x0000; // EEPROM memory start address

    //---------------- UART initialization ----------------//
    status = g_uart0.p_api->open(g_uart0.p_ctrl, g_uart0.p_cfg);
    if (status != SSP_SUCCESS)
    {
        __BKPT(0);  // Halt for debugging if UART init fails
    }

    //---------------- Calculate HX711 offset ----------------//
    for (int i = 0; i < 10; i++)
    {
        offset_sum += get_data();                   // Read data
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
    long offset = offset_sum / 10;                  // Average offset


    //---------------- I2C initialization ----------------//
    status = g_i2c0.p_api->open(g_i2c0.p_ctrl, g_i2c0.p_cfg);
    if(status != SSP_SUCCESS) err_count++;

    // Set the I2C slave address
    status = g_i2c0.p_api->slaveAddressSet(g_i2c0.p_ctrl, i2c_addr, I2C_ADDR_MODE_7BIT);
    if(status != SSP_SUCCESS) err_count++;

    //---------------- Read from EEPROM ----------------//
    eeprom_read(eeprom_mem_addr);               // Read 16 bytes from EEPROM
    scale_adj = eeprom_data.form.data1;         // Restore the correction value

    //---------------- Display EEPROM content via UART ----------------//
    for(uint32_t i=0; i<sizeof(i2c_buf); i++)
    {
        msg_len = snprintf(msg, sizeof(msg), " %02X", i2c_buf[i]);
        if(msg_len > 0)
        {
            status = g_uart0.p_api->write(g_uart0.p_ctrl, (uint8_t*)msg, (uint32_t)msg_len);
            if(status != SSP_SUCCESS) err_count++;
            R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
        }
    }
    msg_len = snprintf(msg, sizeof(msg), "\r\n");
    if(msg_len > 0)
    {
        status = g_uart0.p_api->write(g_uart0.p_ctrl, (uint8_t*)msg, (uint32_t)msg_len);
        if(status != SSP_SUCCESS) err_count++;
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }


    //---------------------------------------------------------------------//
    // Main loop
    //---------------------------------------------------------------------//
    while(1)
    {
        //---------------- Weight measurement ----------------//
        long sum = 0;
        for (int i = 0; i < 10; i++)
        {
            sum += get_data();
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        }
        long data = sum / 10;   // Average 10 samples

        // Convert HX711 output to weight (in grams)
        float weight = (float)(data - offset) / (scale + scale_adj);
        char *sign = (char*)" ";
        if(weight < 0)
        {
            sign = (char*)"-";
            weight = -weight;
        }

        // Send weight data via UART (e.g., " 123.45")
        msg_len = snprintf(msg, sizeof(msg), "%s%d.%02d\r\n", sign, (int)weight, (int)(weight * 100 + 0.5) % 100);
        if(msg_len > 0)
        {
            status = g_uart0.p_api->write(g_uart0.p_ctrl, (uint8_t*)msg, (uint32_t)msg_len);
            if(status != SSP_SUCCESS) err_count++;
            R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
        }


        //---------------- S4 button (increase adjustment) ----------------//
        g_ioport.p_api->pinRead(IOPORT_PORT_00_PIN_06, &pin_value);
        if((s4_current_value == IOPORT_LEVEL_HIGH) && (pin_value == IOPORT_LEVEL_LOW))
        {
            scale_adj += 0.05f; // Slightly increase the correction value

            // Display the new adjustment value
            sign = (scale_adj < 0) ? (char*)"-" : (char*)"";
            msg_len = snprintf(msg, sizeof(msg), "(%s%d.%02d)\r\n", sign, (int)fabsf(scale_adj), (int)(fabsf(scale_adj) * 100) % 100);
            if(msg_len > 0)
            {
                status = g_uart0.p_api->write(g_uart0.p_ctrl, (uint8_t*)msg, (uint32_t)msg_len);
                R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
            }

            // Save the updated value to EEPROM
            eeprom_data.form.data1 = scale_adj;
            eeprom_data.form.data2 = -1;
            eeprom_write(eeprom_mem_addr);

            // Recalculate offset after calibration
            offset_sum = 0;
            for (int i = 0; i < 10; i++)
            {
                offset_sum += get_data();
                R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            }
            offset = offset_sum / 10;
        }
        s4_current_value = pin_value;


        //---------------- S5 button (decrease adjustment) ----------------//
        g_ioport.p_api->pinRead(IOPORT_PORT_00_PIN_05, &pin_value);
        if((s5_current_value == IOPORT_LEVEL_HIGH) && (pin_value == IOPORT_LEVEL_LOW))
        {
            scale_adj -= 0.05f; // Slightly decrease the correction value

            // Display the new adjustment value
            sign = (scale_adj < 0) ? (char*)"-" : (char*)" ";
            msg_len = snprintf(msg, sizeof(msg), "(%s%d.%02d)\r\n", sign, (int)fabsf(scale_adj), (int)(fabsf(scale_adj) * 100) % 100);
            if(msg_len > 0)
            {
                status = g_uart0.p_api->write(g_uart0.p_ctrl, (uint8_t*)msg, (uint32_t)msg_len);
                R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
            }

            // Save the updated value to EEPROM
            eeprom_data.form.data1 = scale_adj;
            eeprom_data.form.data2 = -1;
            eeprom_write(eeprom_mem_addr);

            // Recalculate offset after calibration
            offset_sum = 0;
            for (int i = 0; i < 10; i++)
            {
                offset_sum += get_data();
                R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            }
            offset = offset_sum / 10;
        }
        s5_current_value = pin_value;
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
}


//---------------------------------------------------------------------//
// HX711: Read 24-bit ADC data
//---------------------------------------------------------------------//
static long get_data(void)
{
    SSP_CRITICAL_SECTION_DEFINE;  // Start interrupt protection
    SSP_CRITICAL_SECTION_ENTER;

    long data = 0;

    // Wait until DOUT goes LOW (conversion complete)
    while(1)
    {
        g_ioport.p_api->pinRead(DOUT, &pin_value);
        if(pin_value == IOPORT_LEVEL_LOW) break;
    }

    // Read 24-bit data (MSB first)
    for (int i=0;i<24;i++)
    {
        g_ioport.p_api->pinWrite(sck, IOPORT_LEVEL_HIGH);
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);

        g_ioport.p_api->pinRead(DOUT, &pin_value);
        data = (data << 1) | (pin_value == IOPORT_LEVEL_LOW ? 0 : 1);

        g_ioport.p_api->pinWrite(sck, IOPORT_LEVEL_LOW);
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
    }

    // Generate the 25th pulse to set channel/gain
    g_ioport.p_api->pinWrite(sck, IOPORT_LEVEL_HIGH);
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
    g_ioport.p_api->pinWrite(sck, IOPORT_LEVEL_LOW);
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);

    SSP_CRITICAL_SECTION_EXIT;  // End interrupt protection
    return data;
}


//---------------------------------------------------------------------//
// Write data to EEPROM
//---------------------------------------------------------------------//
void eeprom_write(uint16_t mem_addr)
{
    uint8_t send_size = 0;

    // Set 2-byte memory address at the start of the buffer
    i2c_buf[send_size++] = (uint8_t)(mem_addr >> 8);
    i2c_buf[send_size++] = (uint8_t)mem_addr;

    // Append data to write
    for(uint8_t i=0; (i < sizeof(eeprom_data.buf)) && (send_size < sizeof(i2c_buf)); i++, send_size++)
    {
        i2c_buf[send_size] = eeprom_data.buf[i];
    }

    // Send data to EEPROM over I2C
    status = g_i2c0.p_api->write(g_i2c0.p_ctrl, i2c_buf, send_size, false);
    if(status != SSP_SUCCESS) err_count++;
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // Wait for write cycle
}


//---------------------------------------------------------------------//
// Read data from EEPROM
//---------------------------------------------------------------------//
void eeprom_read(uint16_t mem_addr)
{
    uint8_t i2c_command[] = {0,0};
    i2c_command[0] = (uint8_t)(mem_addr >> 8);
    i2c_command[1] = (uint8_t)mem_addr;

    // Send the memory address to read from
    status = g_i2c0.p_api->write(g_i2c0.p_ctrl, i2c_command, sizeof(i2c_command), false);
    if(status != SSP_SUCCESS) err_count++;

    // Receive data from EEPROM
    status = g_i2c0.p_api->read(g_i2c0.p_ctrl, i2c_buf, sizeof(i2c_buf), false);
    if(status != SSP_SUCCESS) err_count++;
}
