/**
 * Copyright (C) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
 /**
  * Amemded to work with Raspberry PI - Aug 2025 - kmca
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

/***********************************************************************/
/*                    Parameter Structure                              */
/***********************************************************************/

struct sequential_mode_params {
    uint8_t i2c_bus;
    uint8_t i2c_port;
    uint8_t os_hum;
    uint8_t os_temp;
    uint8_t os_pres;
    uint8_t filter;
    uint8_t odr;
    uint16_t temp_prof[10];
    uint16_t dur_prof[10];
    uint16_t sample_count;
    uint32_t sample_sleep_secs;
    uint8_t infinite_mode;
};

/***********************************************************************/
/*                    Helper Functions                                 */
/***********************************************************************/

static void init_default_params(struct sequential_mode_params *params)
{
    uint16_t default_temps[10] = { 200, 240, 280, 320, 360, 360, 320, 280, 240, 200 };
    uint16_t default_durs[10] = { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 };

    params->i2c_bus = 1;
    params->i2c_port = 0x76;
    params->os_hum = 16;
    params->os_temp = 2;
    params->os_pres = 1;
    params->filter = 0;
    params->odr = 8;
    params->sample_count = 300;
    params->sample_sleep_secs = 0;
    params->infinite_mode = 0;

    memcpy(params->temp_prof, default_temps, sizeof(default_temps));
    memcpy(params->dur_prof, default_durs, sizeof(default_durs));
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

static void parse_profile_array(const char *value, uint16_t profile[10])
{
    char buffer[CONFIG_ARG_MAX_BYTES];
    char *token;
    uint8_t idx = 0;

    if (value == NULL)
    {
        return;
    }

    strncpy(buffer, value, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    token = strtok(buffer, ",");
    while ((token != NULL) && (idx < 10))
    {
        while ((*token == ' ') || (*token == '\t'))
        {
            token++;
        }
        profile[idx++] = (uint16_t)strtoul(token, NULL, 0);
        token = strtok(NULL, ",");
    }
}

static int8_t load_sequential_mode_params(const char *cfg_path, struct sequential_mode_params *params)
{
    config_option_t head;
    config_option_t co;

    init_default_params(params);

    head = read_config_file((char *)cfg_path);
    if (head == NULL)
    {
        fprintf(stderr, "Could not read config file %s. Using defaults.\n", cfg_path);
        return BME69X_E_COM_FAIL;
    }

    for (co = head; co != NULL; co = co->prev)
    {
        if (strcmp(co->key, "bus") == 0)
        {
            params->i2c_bus = (uint8_t)strtoul(co->value, NULL, 0);
        }
        else if (strcmp(co->key, "port") == 0)
        {
            params->i2c_port = (uint8_t)strtoul(co->value, NULL, 0);
        }
        else if (strcmp(co->key, "osh") == 0)
        {
            params->os_hum = (uint8_t)strtoul(co->value, NULL, 0);
        }
        else if (strcmp(co->key, "ost") == 0)
        {
            params->os_temp = (uint8_t)strtoul(co->value, NULL, 0);
        }
        else if (strcmp(co->key, "osp") == 0)
        {
            params->os_pres = (uint8_t)strtoul(co->value, NULL, 0);
        }
        else if (strcmp(co->key, "lpf") == 0)
        {
            params->filter = (uint8_t)strtoul(co->value, NULL, 0);
        }
        else if (strcmp(co->key, "slp") == 0)
        {
            params->odr = (uint8_t)strtoul(co->value, NULL, 0);
        }
        else if (strcmp(co->key, "temp_prof") == 0)
        {
            parse_profile_array(co->value, params->temp_prof);
        }
        else if (strcmp(co->key, "dur_prof") == 0)
        {
            parse_profile_array(co->value, params->dur_prof);
        }
        else if (strcmp(co->key, "samples") == 0)
        {
            if (strcmp(co->value, "NULL") == 0)
            {
                params->infinite_mode = 1;
            }
            else
            {
                params->sample_count = (uint16_t)strtoul(co->value, NULL, 0);
                params->infinite_mode = 0;
            }
        }
        else if (strcmp(co->key, "sleep_sec") == 0)
        {
            params->sample_sleep_secs = (uint32_t)strtoul(co->value, NULL, 0);
        }
    }

    free_config_list(head);
    return BME69X_OK;
}

static void print_config_summary(const struct sequential_mode_params *params)
{
    printf("bus: %u, port: 0x%02X, osh: %u, ost: %u, osp: %u\n",
           params->i2c_bus,
           params->i2c_port,
           params->os_hum,
           params->os_temp,
           params->os_pres);
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
    struct bme69x_data data[3];
    uint32_t del_period;
    uint32_t time_ms = 0;
    uint8_t n_fields;
    uint16_t sample_count = 1;
    const char *cfg_path = "./sequential_mode.cfg";
    struct sequential_mode_params params;

    if (argc > 1)
    {
        cfg_path = argv[1];
    }

    (void)load_sequential_mode_params(cfg_path, &params);
    print_config_summary(&params);

    bme69x_set_i2c_bus_and_addr(params.i2c_bus, params.i2c_port);

    /* Interface preference is updated as a parameter
     * For I2C : BME69X_I2C_INTF
     * For SPI : BME69X_SPI_INTF
     */
    rslt = bme69x_interface_init(&bme, BME69X_I2C_INTF);
    bme69x_check_rslt("bme69x_interface_init", rslt);

    rslt = bme69x_init(&bme);
    bme69x_check_rslt("bme69x_init", rslt);

    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    rslt = bme69x_get_conf(&conf, &bme);
    bme69x_check_rslt("bme69x_get_conf", rslt);

    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    conf.filter = map_filter_value(params.filter);
    conf.odr = map_odr_value(params.odr);
    conf.os_hum = map_os_value(params.os_hum);
    conf.os_pres = map_os_value(params.os_pres);
    conf.os_temp = map_os_value(params.os_temp);
    rslt = bme69x_set_conf(&conf, &bme);
    bme69x_check_rslt("bme69x_set_conf", rslt);

    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    heatr_conf.enable = BME69X_ENABLE;
    heatr_conf.heatr_temp_prof = params.temp_prof;
    heatr_conf.heatr_dur_prof = params.dur_prof;
    heatr_conf.profile_len = 10;
    rslt = bme69x_set_heatr_conf(BME69X_SEQUENTIAL_MODE, &heatr_conf, &bme);
    bme69x_check_rslt("bme69x_set_heatr_conf", rslt);

    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    rslt = bme69x_set_op_mode(BME69X_SEQUENTIAL_MODE, &bme);
    bme69x_check_rslt("bme69x_set_op_mode", rslt);

    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    printf(
        "Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Status, Profile index, Measurement index\n");
    while (params.infinite_mode || (sample_count <= params.sample_count))
    {
        /* Calculate delay period in microseconds */
        del_period = bme69x_get_meas_dur(BME69X_SEQUENTIAL_MODE, &conf, &bme) + (heatr_conf.heatr_dur_prof[0] * 1000);
        bme.delay_us(del_period, bme.intf_ptr);

        time_ms = rpi_timestamp_ms();

        rslt = bme69x_get_data(BME69X_SEQUENTIAL_MODE, data, &n_fields, &bme);
        bme69x_check_rslt("bme69x_get_data", rslt);

        /* Check if rslt == BME69X_OK, report or handle if otherwise */
        for (uint8_t i = 0; i < n_fields; i++)
        {
#ifdef BME69X_USE_FPU
            printf("%u,%lu,%.2f,%.2f,%.2f,%.2f,0x%x,%d,%d\n",
                   sample_count,
                   (long unsigned int)time_ms + (i * (del_period / 2000)),
                   data[i].temperature,
                   data[i].pressure,
                   data[i].humidity,
                   data[i].gas_resistance,
                   data[i].status,
                   data[i].gas_index,
                   data[i].meas_index);
#else
            printf("%u, %lu, %d, %lu, %lu, %lu, 0x%x, %d, %d\n",
                   sample_count,
                   (long unsigned int)time_ms + (i * (del_period / 2000)),
                   (data[i].temperature),
                   (long unsigned int)data[i].pressure,
                   (long unsigned int)(data[i].humidity),
                   (long unsigned int)data[i].gas_resistance,
                   data[i].status,
                   data[i].gas_index,
                   data[i].meas_index);
#endif
            sample_count++;
        }

        if ((params.sample_sleep_secs > 0) && (params.infinite_mode || (sample_count <= params.sample_count)))
        {
            bme.delay_us(params.sample_sleep_secs * 1000000U, bme.intf_ptr);
        }
    }

   bme69x_I2C_deinit();

    return 0;
}
