/**
 * Copyright (C) 2024 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
 /**
 *  Amended to support Raspbian - Aug 2025 - kmca
 *  By replacing the COINS functions with their Raspbian equivalents
 *  I2C is supported, but SPI is not implemented. 
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

#include "bme69x.h"
/*! #include "coines.h"  */
#include "rpi-common.h"

uint8_t rpi_device; 
uint8_t rslt;
/*! uint32_t time_ms); */

/******************************************************************************/
/*!                 Macro definitions                                         */
/*!                 BME69X */
#define TEMP_OFFSET 0.0f;
#define BME69X_USE_FPU
/*! 
 *               Static variable definition
 */
static uint8_t dev_addr;

/*!
 *               Raspbian I2C read function definition  
 *               COINS replacement  
 */
int8_t rpi_i2c_read(uint8_t regAddr, uint8_t *regData, uint32_t len, void *intf_ptr)
{
    rslt = BME69X_OK;
    if (write(*((uint8_t *)intf_ptr), &regAddr, 1) != 1)
    {
        perror("rpi_read register");
        rslt = -1;
    }
    if (read(*((uint8_t *)intf_ptr), regData, len) != len)
    {
        perror("rpi_read data");
        rslt = -1;
    }

    return rslt;
}

/*
 *          Raspbian i2c write function defintion
 *          COINES replacement
 */
int8_t rpi_i2c_write(uint8_t regAddr, const uint8_t *regData, uint32_t len, void *intf_ptr)
{
    rslt = BME69X_OK;
    uint8_t reg[len + 1];
    reg[0] = regAddr;

    for (uint32_t i = 1; i < len + 1; i++)
        reg[i] = regData[i - 1];

    if (write(*((uint8_t *)intf_ptr), reg, len + 1) != len + 1)
    {
        perror("rpi_write");
        rslt = -1;
    }

    return rslt;
}



/*!
 * Delay function to replace COINES platform
 */
void rpi_delay_us(uint32_t duration_us, void *intf_ptr)
{
    struct timespec ts;
    ts.tv_sec = duration_us / 1000000;
    ts.tv_nsec = (duration_us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}
/*!
* Timestamp functions
*/
int64_t rpi_timestamp_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);    
}

uint32_t rpi_timestamp_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
}   

uint32_t rpi_timestamp_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

void bme69x_check_rslt(const char api_name[], int8_t rslt)
{
    switch (rslt)
    {
        case BME69X_OK:

            /* Do nothing */
            break;
        case BME69X_E_NULL_PTR:
            printf("API name [%s]  Error [%d] : Null pointer\r\n", api_name, rslt);
            break;
        case BME69X_E_COM_FAIL:
            printf("API name [%s]  Error [%d] : Communication failure\r\n", api_name, rslt);
            break;
        case BME69X_E_INVALID_LENGTH:
            printf("API name [%s]  Error [%d] : Incorrect length parameter\r\n", api_name, rslt);
            break;
        case BME69X_E_DEV_NOT_FOUND:
            printf("API name [%s]  Error [%d] : Device not found\r\n", api_name, rslt);
            break;
        case BME69X_E_SELF_TEST:
            printf("API name [%s]  Error [%d] : Self test error\r\n", api_name, rslt);
            break;
        case BME69X_W_NO_NEW_DATA:
            printf("API name [%s]  Warning [%d] : No new data found\r\n", api_name, rslt);
            break;
        default:
            printf("API name [%s]  Error [%d] : Unknown error code\r\n", api_name, rslt);
            break;
    }
}

int8_t bme69x_interface_init(struct bme69x_dev *bme, uint8_t intf)
{
    int8_t rslt = BME69X_OK;

    if (bme != NULL)
    {
        /* Bus configuration : I2C */
        if (intf == BME69X_I2C_INTF)
        {
            printf("I2C Interface\n");
            dev_addr = BME69X_I2C_ADDR_LOW;
            bme->read = rpi_i2c_read;
            bme->write = rpi_i2c_write;
            bme->intf = BME69X_I2C_INTF;

            /* Open I2C device and assign it to bme->intf_ptr */
            rpi_device = open("/dev/i2c-1", O_RDWR);
            if (rpi_device < 0)
            {
                perror("Failed to open I2C device");
                return BME69X_E_COM_FAIL;
            }
            bme->intf_ptr = &rpi_device;
            if (ioctl(*(uint8_t *) bme->intf_ptr, I2C_SLAVE, dev_addr) < 0)
            {
                perror("Failed to set I2C address");
                return BME69X_E_COM_FAIL;
            }
            bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
            bme->read = rpi_i2c_read;
            bme->write = rpi_i2c_write;
            bme->delay_us = rpi_delay_us;


        }
        /* Bus configuration : SPI */
        else if (intf == BME69X_SPI_INTF)
        {
            printf("SPI Interface not supported\n");
            exit(1);
        }


        rpi_delay_us(100, bme->intf_ptr);

        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
    }
    else
    {
        rslt = BME69X_E_NULL_PTR;
    }

    return rslt;
}

void bme69x_I2C_deinit(void)
{
    /* No COINS to close/flush */
    printf("Exiting....");
}
