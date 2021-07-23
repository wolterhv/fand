#include "fan_interface.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ini.h"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define CLIP(min,max,x) (MAX((min),(MIN((max),(x)))))

// This source code implements a valid interface
//
struct FanIFConfig {
    char     pigpio_server_address[100];
    char     pigpio_server_port[10];
    uint8_t  gpio_pin;
    uint16_t pwm_freq;
    uint16_t pwm_dc_off;
    uint16_t pwm_dc_minsig;
    uint16_t pwm_dc_min;
    uint16_t pwm_dc_max;
    uint16_t pwm_dc_base;
} FanIFConfig;

struct FanIFState {
    // int pigpio_client_id;
    FILE *pigpio_pipe;
} FanIFState;

static bool config_reloaded = false;

struct FanIFConfig*
fan_if_config_new()
{
    return (struct FanIFConfig*) malloc(sizeof(struct FanIFConfig));
}

struct FanIFState*
fan_if_state_new()
{
    return (struct FanIFState*) malloc(sizeof(struct FanIFState));
}

static int
fan_if_config_iniload_handler(      void *user,
                              const char *section,
                              const char *name,
                              const char *value)
{
#define MATCH(s, n) ((strcmp(section, s) == 0) && (strcmp(name, n) == 0))
#define SET_OPT_I(s,p,type) else if (MATCH(#s,#p)) { ((struct FanIFConfig*) user)->p = (type)  atoi(value); }
#define SET_OPT_F(s,p)      else if (MATCH(#s,#p)) { ((struct FanIFConfig*) user)->p = (float) atof(value); }
#define SET_OPT_S(s,p)      else if (MATCH(#s,#p)) { strcpy(((struct FanIFConfig*) user)->p, value); }

    if (false) { return 0; }
    SET_OPT_I(fan_interface, gpio_pin,      uint8_t)
    SET_OPT_I(fan_interface, pwm_freq,      uint16_t)
    SET_OPT_I(fan_interface, pwm_dc_off,    uint16_t)
    SET_OPT_I(fan_interface, pwm_dc_minsig, uint16_t)
    SET_OPT_I(fan_interface, pwm_dc_min,    uint16_t)
    SET_OPT_I(fan_interface, pwm_dc_max,    uint16_t)
    SET_OPT_I(fan_interface, pwm_dc_base,   uint16_t)
    SET_OPT_S(fan_interface, pigpio_server_address)
    SET_OPT_S(fan_interface, pigpio_server_port)
    else       { return 0; }

#undef SET_OPT_I
#undef SET_OPT_F
#undef SET_OPT_S
#undef MATCH
}

int
fan_if_load_config(struct FanIFConfig *config,
                   struct FanIFState  *state)
{
    static char config_fpath[255];

    sprintf(config_fpath, "%s/fand/config.ini", getenv("XDG_CONFIG_HOME"));
    ini_parse(config_fpath, fan_if_config_iniload_handler, (void*) config); // TODO error catching
    printf("fan_if_pigpiodif2 config loaded.\n");

    config_reloaded = true;

    return 0;
}

int
fan_if_set_intensity(const struct FanIFConfig *config,
                     const struct FanIFState  *state,
                     float intensity)
{
    float pwm_dc = 0;

    // TODO assert that intensity is in domain

    // Could be intensity == FAN_INTENSITY_OFF 
    // but checking equality with a floating point is not recommended
    if (intensity < FAN_INTENSITY_MIN) {
        pwm_dc = 0;
    } else {
        pwm_dc = CLIP(config->pwm_dc_min, 
                      config->pwm_dc_max,
                        config->pwm_dc_min 
                      + ((uint8_t) ((float)   (config->pwm_dc_max - config->pwm_dc_min))
                                            / (FAN_INTENSITY_MAX  - FAN_INTENSITY_MIN)
                                            * (intensity          - FAN_INTENSITY_MIN)));
    }
    // NOTE The pigpio interface probably already checks frequency / DC before
    // trying to set them again.
    
    fprintf(state->pigpio_pipe,
            "p %d %d\n", config->gpio_pin, (uint16_t) pwm_dc);
    fflush(state->pigpio_pipe);

    if (config_reloaded) {
        // Set PWM frequency
        fprintf(state->pigpio_pipe,
                "pfs %d %d\n", config->gpio_pin, config->pwm_freq);
        fflush(state->pigpio_pipe);

        config_reloaded = false;
    }

    return 0;
}

int
fan_if_init(struct FanIFConfig *config,
            struct FanIFState  *state)
{
    fan_if_load_config(config, state);

    config_reloaded = false;
    state->pigpio_pipe = fopen("/dev/pigpio","w");
    fprintf(state->pigpio_pipe,
            "pfs %d %d", config->gpio_pin, config->pwm_freq);
    // TODO use intensity off?
    fan_if_set_intensity(config, state, FAN_INTENSITY_OFF);
    return 0;
}


int
fan_if_term(struct FanIFConfig *config,
            struct FanIFState  *state)
{
    fan_if_set_intensity(config, state, FAN_INTENSITY_OFF);
    fclose(state->pigpio_pipe);
    return 0;
}
