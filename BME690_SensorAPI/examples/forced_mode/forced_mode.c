/**
 * Copyright (C) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bme69x.h"
#include "rpi-common.h"
#include "config_parser/parser.h"

/***********************************************************************/
/*                         Macros                                      */
/***********************************************************************/

/* Defaults used when config keys are missing */
#define DEFAULT_SAMPLE_COUNT  UINT16_C(3000)
#define DEFAULT_HEATR_TEMP    UINT16_C(300)
#define DEFAULT_HEATR_DUR     UINT16_C(100)
#define DEFAULT_SLEEP_SECS    UINT32_C(3)
#define DEFAULT_CONFIG_PATH   "./690.cfg"

struct forced_mode_params
{
    uint8_t i2c_bus;
    uint8_t i2c_port;
    uint8_t os_hum;
    uint8_t os_temp;
    uint8_t os_pres;
    uint8_t filter;
    uint8_t odr;
    uint16_t heatr_temp;
    uint16_t heatr_dur;
    uint16_t sample_count;
    uint32_t sample_sleep_secs;
    uint8_t infinite_mode;
};

static void init_default_params(struct forced_mode_params *params)
{
    params->i2c_bus = 1;
    params->i2c_port = BME69X_I2C_ADDR_LOW;
    params->os_hum = BME69X_OS_16X;
    params->os_temp = BME69X_OS_16X;
    params->os_pres = BME69X_OS_16X;
    params->filter = BME69X_FILTER_OFF;
    params->odr = BME69X_ODR_NONE;
    params->heatr_temp = DEFAULT_HEATR_TEMP;
    params->heatr_dur = DEFAULT_HEATR_DUR;
    params->sample_count = DEFAULT_SAMPLE_COUNT;
    params->sample_sleep_secs = DEFAULT_SLEEP_SECS;
    params->infinite_mode = 0;
}

static uint8_t map_os_value(unsigned long value)
{
    switch (value)
    {
        case 0: return BME69X_OS_NONE;
        case 1: return BME69X_OS_1X;
        case 2: return BME69X_OS_2X;
        case 4: return BME69X_OS_4X;
        case 8: return BME69X_OS_8X;
        case 16: return BME69X_OS_16X;
        default: return BME69X_OS_16X;
    }
}

static uint8_t map_filter_value(unsigned long value)
{
    switch (value)
    {
        case 0: return BME69X_FILTER_OFF;
        case 2: return BME69X_FILTER_SIZE_1;
        case 4: return BME69X_FILTER_SIZE_3;
        case 8: return BME69X_FILTER_SIZE_7;
        case 16: return BME69X_FILTER_SIZE_15;
        case 32: return BME69X_FILTER_SIZE_31;
        case 64: return BME69X_FILTER_SIZE_63;
        case 128: return BME69X_FILTER_SIZE_127;
        default: return BME69X_FILTER_OFF;
    }
}

static uint8_t map_odr_value(unsigned long value)
{
    switch (value)
    {
        case 0: return BME69X_ODR_0_59_MS;
        case 1: return BME69X_ODR_62_5_MS;
        case 2: return BME69X_ODR_125_MS;
        case 3: return BME69X_ODR_250_MS;
        case 4: return BME69X_ODR_500_MS;
        case 5: return BME69X_ODR_1000_MS;
        case 6: return BME69X_ODR_10_MS;
        case 7: return BME69X_ODR_20_MS;
        case 8: return BME69X_ODR_NONE;
        default: return BME69X_ODR_NONE;
    }
}

static void free_config_list(config_option_t co)
{
    while (co != NULL)
    {
        config_option_t prev = co->prev;
        free(co);
        co = prev;
    }
}

static int8_t load_forced_mode_params(const char *cfg_path, struct forced_mode_params *params)
{
    config_option_t co;
    config_option_t head;

    init_default_params(params);
    co = read_config_file((char *)cfg_path);
    if (co == NULL)
    {
        return -1;
    }

    head = co;

    while (co != NULL)
    {
        unsigned long value = strtoul(co->value, NULL, 0);

        if (strcmp(co->key, "osh") == 0)
        {
            params->os_hum = map_os_value(value);
        }
        else if (strcmp(co->key, "bus") == 0)
        {
            params->i2c_bus = (uint8_t)value;
        }
        else if (strcmp(co->key, "port") == 0)
        {
            params->i2c_port = (uint8_t)value;
        }
        else if (strcmp(co->key, "ost") == 0)
        {
            params->os_temp = map_os_value(value);
        }
        else if (strcmp(co->key, "osp") == 0)
        {
            params->os_pres = map_os_value(value);
        }
        else if (strcmp(co->key, "lpf") == 0)
        {
            params->filter = map_filter_value(value);
        }
        else if (strcmp(co->key, "slp") == 0)
        {
            params->odr = map_odr_value(value);
        }
        else if ((strcmp(co->key, "heatr_temp") == 0) || (strcmp(co->key, "htp") == 0))
        {
            params->heatr_temp = (uint16_t)value;
        }
        else if ((strcmp(co->key, "heatr_dur") == 0) || (strcmp(co->key, "htd") == 0))
        {
            params->heatr_dur = (uint16_t)value;
        }
        else if (strcmp(co->key, "samples") == 0)
        {
            if (strcasecmp(co->value, "NULL") == 0)
            {
                params->infinite_mode = 1;
                params->sample_count = UINT16_MAX;
            }
            else
            {
                params->sample_count = (uint16_t)value;
            }
        }
        else if (strcmp(co->key, "sleep_sec") == 0)
        {
            params->sample_sleep_secs = (uint32_t)value;
        }

        co = co->prev;
    }

    free_config_list(head);
    return BME69X_OK;
}
static void print_config_summary(struct forced_mode_params *params)
{
    if (params->infinite_mode)
    {
        printf("Config: I2C Bus=%u Addr=0x%02x, OSH=%u OST=%u OSP=%u Filter=%u ODR=%u, HeatrT=%uC HeatrD=%ums, samples=NULL sleep_sec=%u\n",
               params->i2c_bus,
               params->i2c_port,
               params->os_hum,
               params->os_temp,
               params->os_pres,
               params->filter,
               params->odr,
               params->heatr_temp,
               params->heatr_dur,
               params->sample_sleep_secs);
    }
    else
    {
        printf("Config: I2C Bus=%u Addr=0x%02x, OSH=%u OST=%u OSP=%u Filter=%u ODR=%u, HeatrT=%uC HeatrD=%ums, samples=%u sleep_sec=%u\n",
               params->i2c_bus,
               params->i2c_port,
               params->os_hum,
               params->os_temp,
               params->os_pres,
               params->filter,
               params->odr,
               params->heatr_temp,
               params->heatr_dur,
               params->sample_count,
               params->sample_sleep_secs);
    }
}
/***********************************************************************/
/*                         Test code                                   */
/***********************************************************************/

int main(int argc, char *argv[])
{
    struct bme69x_dev bme;
    int8_t rslt;
    struct bme69x_conf conf;
    struct bme69x_heatr_conf heatr_conf;
    struct forced_mode_params params;
    struct bme69x_data data;
    uint32_t del_period;
    uint32_t time_ms = 0;
    uint8_t n_fields;
    uint16_t sample_count = 1;
    const char *cfg_path = DEFAULT_CONFIG_PATH;
    /*! uint8_t linux_device = 0;  */

    if (argc > 1)
    {
        cfg_path = argv[1];
    }

    if (load_forced_mode_params(cfg_path, &params) != BME69X_OK)
    {
        init_default_params(&params);
        printf("Config load failed for '%s'. Using defaults.\n", cfg_path);
    }

    print_config_summary(&params);
    bme69x_set_i2c_bus_and_addr(params.i2c_bus, params.i2c_port);

    /* Interface preference is updated as a parameter
     * For I2C : BME69X_I2C_INTF
     * For SPI : BME69X_SPI_INTF (not yet supported here)
     */
    rslt = bme69x_interface_init(&bme, BME69X_I2C_INTF);
    bme69x_check_rslt("bme69x_interface_init", rslt);

    rslt = bme69x_init(&bme);
    bme69x_check_rslt("bme69x_init", rslt);

    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    conf.filter = params.filter;
    conf.odr = params.odr;
    conf.os_hum = params.os_hum;
    conf.os_pres = params.os_pres;
    conf.os_temp = params.os_temp;
    rslt = bme69x_set_conf(&conf, &bme);
    bme69x_check_rslt("bme69x_set_conf", rslt);

    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    heatr_conf.enable = BME69X_ENABLE;
    heatr_conf.heatr_temp = params.heatr_temp;
    heatr_conf.heatr_dur = params.heatr_dur;
    rslt = bme69x_set_heatr_conf(BME69X_FORCED_MODE, &heatr_conf, &bme);
    bme69x_check_rslt("bme69x_set_heatr_conf", rslt);

    printf("Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Gas Index, Status\n");

    while (params.infinite_mode || sample_count <= params.sample_count)
    {
        rslt = bme69x_set_op_mode(BME69X_FORCED_MODE, &bme);
        bme69x_check_rslt("bme69x_set_op_mode", rslt);

        /* Calculate delay period in microseconds */
        del_period = bme69x_get_meas_dur(BME69X_FORCED_MODE, &conf, &bme) + (heatr_conf.heatr_dur * 1000);
        bme.delay_us(del_period, bme.intf_ptr);

        time_ms = rpi_timestamp_ms();

        /* Check if rslt == BME69X_OK, report or handle if otherwise */
        rslt = bme69x_get_data(BME69X_FORCED_MODE, &data, &n_fields, &bme);
        bme69x_check_rslt("bme69x_get_data", rslt);

        if (n_fields)
        {
#ifdef BME69X_USE_FPU
            printf("%u, %lu, %.2f, %.2f, %.2f, %.2f, %u, 0x%x\n", 
                   sample_count,
                   (long unsigned int)time_ms,
                   data.temperature,
                   data.pressure,
                   data.humidity,
                   data.gas_resistance,
                   data.gas_index,
                   data.status);
#else
            printf("%u, %lu, %d, %ld, %ld, %ld, 0x%x\n",
                   sample_count,
                   (long unsigned int)time_ms,
                   data.temperature,
                   data.pressure,
                   data.humidity,
                   data.gas_resistance,
                   data.status);
#endif
            sleep(params.sample_sleep_secs);
            sample_count++;
        }
    }

    bme69x_I2C_deinit();

    return rslt;
}
