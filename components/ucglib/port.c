#include "ucg.h"
#include "espressif/esp_common.h"
#include "esp/spi.h"
#include "esp/gpio.h"

#include "FreeRTOS.h"
#include "task.h"

#define SPI_BUS 1


int16_t ucg_com_cb_esp8266_spi_hw(ucg_t *ucg, int16_t msg, uint32_t arg, uint8_t *data)
{
  switch(msg)
  {
    case UCG_COM_MSG_POWER_UP:
        gpio_enable(ucg->pin_list[UCG_PIN_CD], GPIO_OUTPUT);
        gpio_enable(ucg->pin_list[UCG_PIN_CS], GPIO_OUTPUT);

        break;
    case UCG_COM_MSG_POWER_DOWN:
        break;
    case UCG_COM_MSG_DELAY:
        sdk_os_delay_us(arg);
        break;
    case UCG_COM_MSG_CHANGE_RESET_LINE:
        break;
    case UCG_COM_MSG_CHANGE_CS_LINE:
        gpio_write(ucg->pin_list[UCG_PIN_CS], arg);
        break;
    case UCG_COM_MSG_CHANGE_CD_LINE:
        gpio_write(ucg->pin_list[UCG_PIN_CD], arg);
        break;
    case UCG_COM_MSG_SEND_BYTE:
        spi_transfer_8(SPI_BUS, arg);
      break;
    case UCG_COM_MSG_REPEAT_1_BYTE:
        for(;arg > 0; arg--)
        {
            spi_transfer_8(SPI_BUS, data[0]);
        }
      break;
    case UCG_COM_MSG_REPEAT_2_BYTES:
        for(;arg > 0; arg--)
        {
            spi_transfer_16(SPI_BUS, (data[0] << 8) | data[1]);
        }
      break;
    case UCG_COM_MSG_REPEAT_3_BYTES:
        for(;arg > 0; arg--)
        {
            spi_transfer_16(SPI_BUS, (data[0] << 8) | data[1]);
            spi_transfer_8(SPI_BUS, data[2]);
        }
      break;
    case UCG_COM_MSG_SEND_STR:
        spi_transfer(SPI_BUS, data, NULL, arg, SPI_8BIT);
      break;
    case UCG_COM_MSG_SEND_CD_DATA_SEQUENCE:
        while(arg > 0)
        {
            if(*data != 0)
            {
                if(*data == 1)
                {
                    gpio_write(ucg->pin_list[UCG_PIN_CD], 0);
                } else {
                    gpio_write(ucg->pin_list[UCG_PIN_CD], 1);
                }
            }
            data++;
            spi_transfer_8(SPI_BUS, *data);
            data++;
            arg--;
        }
      break;
  }
  return 1;
}

