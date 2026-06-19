/**
 * Copyright (C) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
 /**
    * Amended to work with Raspberry PI Aug 2025 kmca
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

/*
 * Macro definition for valid new data (0x80) AND
 * heater stability (0x10) AND gas resistance (0x20) values
 */
#define BME69X_VALID_DATA  UINT8_C(0xB0)

/***********************************************************************/
/*                    Parameter Structure                              */
/***********************************************************************/

struct parallel_mode_params {
    uint8_t i2c_bus;
    uint8_t i2c_port;
    uint8_t os_hum;
    uint8_t os_temp;
    uint8_t os_pres;
    uint8_t filter;
    uint8_t odr;
    uint16_t temp_prof[10];
    uint16_t mul_prof[10];
    uint16_t sample_count;
    uint32_t sample_sleep_secs;
    uint8_t infinite_mode;
};

/***********************************************************************/
/*                    Helper Functions                                 */
/***********************************************************************/

void init_default_params(struct parallel_mode_params *params)
{
    params->i2c_bus = 1;
    params->i2c_port = 0x76;
    params->os_hum = 16;
    params->os_temp = 16;
    params->os_pres = 16;
    params->filter = 0;
    params->odr = 8;
    params->sample_count = 50;
    params->sample_sleep_secs = 0;
    params->infinite_mode = 0;
    
    /* Default heater temperature profile */
    uint16_t default_temps[10] = { 320, 100, 100, 100, 200, 200, 200, 320, 320, 320 };
    memcpy(params->temp_prof, default_temps, sizeof(default_temps));
    
    /* Default heater duration multipliers */
    uint16_t default_muls[10] = { 5, 2, 10, 30, 5, 5, 5, 5, 5, 5 };
    memcpy(params->mul_prof, default_muls, sizeof(default_muls));
}

uint8_t map_os_value(uint16_t val)
{
    switch (val) {
        case 0:  return BME69X_OS_NONE;
        case 1:  return BME69X_OS_1X;
        case 2:  return BME69X_OS_2X;
        case 4:  return BME69X_OS_4X;
        case 8:  return BME69X_OS_8X;
        case 16: return BME69X_OS_16X;
        default: return BME69X_OS_16X;
    }
}

uint8_t map_filter_value(uint16_t val)
{
    switch (val) {
        case 0:   return BME69X_FILTER_OFF;
        case 2:   return BME69X_FILTER_SIZE_1;
        case 4:   return BME69X_FILTER_SIZE_3;
        case 8:   return BME69X_FILTER_SIZE_7;
        case 16:  return BME69X_FILTER_SIZE_15;
        case 32:  return BME69X_FILTER_SIZE_31;
        case 64:  return BME69X_FILTER_SIZE_63;
        case 128: return BME69X_FILTER_SIZE_127;
        default:  return BME69X_FILTER_OFF;
    }
}

uint8_t map_odr_value(uint16_t val)
{
    if (val >= 8) return BME69X_ODR_NONE;
    return (uint8_t)val;
}

void free_config_list(config_option_t config)
{
    config_option_t current = config;
    while (current != NULL) {
        config_option_t prev = current->prev;
        free(current);
        current = prev;
    }
}

int parse_profile_array(const char *value_str, uint16_t *array, int array_size)
{
    if (!value_str || !array) return -1;
    
    char *copy = malloc(strlen(value_str) + 1);
    if (!copy) return -1;
    strcpy(copy, value_str);
    
    int count = 0;
    char *token = strtok(copy, ",");
    
    while (token && count < array_size) {
        /* Skip whitespace */
        while (*token == ' ' || *token == '\t') token++;
        array[count] = (uint16_t)strtoul(token, NULL, 0);
        count++;
        token = strtok(NULL, ",");
    }
    
    free(copy);
    return count;
}

void load_parallel_mode_params(const char *config_path, struct parallel_mode_params *params)
{
    init_default_params(params);
    
    if (!config_path) return;
    
    config_option_t config = read_config_file((char *)config_path);
    if (!config) {
        fprintf(stderr, "Warning: Could not load config file, using defaults\n");
        return;
    }
    
    config_option_t current = config;
    while (current != NULL) {
        unsigned long val = strtoul(current->value, NULL, 0);
        
        if (strcmp(current->key, "bus") == 0) {
            params->i2c_bus = (uint8_t)val;
        } else if (strcmp(current->key, "port") == 0) {
            params->i2c_port = (uint8_t)val;
        } else if (strcmp(current->key, "osh") == 0) {
            params->os_hum = (uint8_t)val;
        } else if (strcmp(current->key, "ost") == 0) {
            params->os_temp = (uint8_t)val;
        } else if (strcmp(current->key, "osp") == 0) {
            params->os_pres = (uint8_t)val;
        } else if (strcmp(current->key, "lpf") == 0) {
            params->filter = (uint8_t)val;
        } else if (strcmp(current->key, "slp") == 0) {
            params->odr = (uint8_t)val;
        } else if (strcmp(current->key, "samples") == 0) {
            if (strcmp(current->value, "NULL") == 0) {
                params->infinite_mode = 1;
                params->sample_count = 0;
            } else {
                params->sample_count = (uint16_t)val;
                params->infinite_mode = 0;
            }
        } else if (strcmp(current->key, "sleep_sec") == 0) {
            params->sample_sleep_secs = (uint32_t)val;
        } else if (strcmp(current->key, "temp_prof") == 0) {
            parse_profile_array(current->value, params->temp_prof, 10);
        } else if (strcmp(current->key, "mul_prof") == 0) {
            parse_profile_array(current->value, params->mul_prof, 10);
        }
        
        current = current->prev;
    }
    
    free_config_list(config);
}

void print_config_summary(struct parallel_mode_params *params)
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
    uint8_t n_fields;
    uint32_t time_ms = 0;
    uint16_t sample_count = 1;
    struct parallel_mode_params params;
    
    const char *config_path = (argc > 1) ? argv[1] : "./parallel_mode.cfg";
    
    /* Load parameters from config file */
    load_parallel_mode_params(config_path, &params);
    print_config_summary(&params);
    
    /* Set I2C bus and address before interface init */
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

    /* Configure sensor based on parameters */
    conf.filter = map_filter_value(params.filter);
    conf.odr = map_odr_value(params.odr);
    conf.os_hum = map_os_value(params.os_hum);
    conf.os_pres = map_os_value(params.os_pres);
    conf.os_temp = map_os_value(params.os_temp);
    rslt = bme69x_set_conf(&conf, &bme);
    bme69x_check_rslt("bme69x_set_conf", rslt);

    /* Configure heater with profiles */
    heatr_conf.enable = BME69X_ENABLE;
    heatr_conf.heatr_temp_prof = params.temp_prof;
    heatr_conf.heatr_dur_prof = params.mul_prof;

    /* Shared heating duration in milliseconds */
    heatr_conf.shared_heatr_dur = (uint16_t)(140 - (bme69x_get_meas_dur(BME69X_PARALLEL_MODE, &conf, &bme) / 1000));

    heatr_conf.profile_len = 10;
    rslt = bme69x_set_heatr_conf(BME69X_PARALLEL_MODE, &heatr_conf, &bme);
    bme69x_check_rslt("bme69x_set_heatr_conf", rslt);

    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    rslt = bme69x_set_op_mode(BME69X_PARALLEL_MODE, &bme);
    bme69x_check_rslt("bme69x_set_op_mode", rslt);

    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    printf(
        "Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Status, Gas index, Meas index\n");
    
    while (params.infinite_mode || sample_count <= params.sample_count)
    {
        /* Calculate delay period in microseconds */
        del_period = bme69x_get_meas_dur(BME69X_PARALLEL_MODE, &conf, &bme) + (heatr_conf.shared_heatr_dur * 1000);
        bme.delay_us(del_period, bme.intf_ptr);

        time_ms = rpi_timestamp_ms();

        rslt = bme69x_get_data(BME69X_PARALLEL_MODE, data, &n_fields, &bme);
        if (rslt < BME69X_OK)
        {
            bme69x_check_rslt("bme69x_get_data", rslt);
            continue;
        }
        if (rslt == BME69X_W_NO_NEW_DATA)
        {
            continue;
        }

        /* Check if rslt == BME69X_OK, report or handle if otherwise */
        for (uint8_t i = 0; i < n_fields; i++)
        {
            if ((data[i].status & BME69X_VALID_DATA) == BME69X_VALID_DATA)
            {
#ifdef BME69X_USE_FPU
                printf("%u, %lu, %.2f, %.2f, %.2f, %.2f, 0x%x, %d, %d\n",
                       sample_count,
                       (long unsigned int)time_ms,
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
                       (long unsigned int)time_ms,
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
        }
        
        /* Sleep between samples if configured */
        if (params.sample_sleep_secs > 0 && (params.infinite_mode || sample_count <= params.sample_count)) {
            sleep(params.sample_sleep_secs);
        }
    }

    bme69x_I2C_deinit();

    return 0;
}
