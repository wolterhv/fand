#ifndef _FAND_H_
#define _FAND_H_

#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* #include "confini.h" */
#include "ini.h"

#include "fan_interface.h"
#include "cpu_interface.h"

struct StateVec {
    struct StateVec *prev;

    // Physical
    time_t   datetime;  // calendar time
    float    time;      // CLOCK_MONOTONIC time / s
    struct timespec cycle_period;
    float    cpu_tempr; // cpu temperature / °C
    float    cpu_use;   // cpu use / %
    // uint8_t  fan_dc;    // fan PWM duty cycle / 1
    float    fan_intensity;
    bool     fan_hyst;
    float    error;     // cpu temperature error / °C
    float    error_int; // ... integral / (°C s)
    float    error_der; // ... derivative / (°C/s)

    // Non-physical
    FILE    *log_fobj;
    timer_t  log_timer_id;
    struct itimerspec log_timer_value;
    // FILE    *cpu_tempr_fobj;
    // FILE    *cpu_use_fobj;
    // uint8_t  fan_pigpio_client_id;

    // Interfaces
    void *fan_if_obj;
    void *cpu_if_obj;
};

struct Config {
    float     sys_cycle_period;
    float     cpu_tempr_tgt; // target cpu temperature / °C
    float     cpu_tempr_on;
    float     cpu_tempr_off;
    float     cpu_tempr_lpf_g;
    // char      cpu_tempr_fpath[100];
    // char      cpu_use_fpath[100];
    float     pid_kp; // PID proportional coeff. / (1/°C)
    float     pid_tn; // PID integral     coeff.
    float     pid_tv; // PID differential coeff.
    float     pid_imin; // PID differential coeff. / (1/(°C/s))
    float     pid_imax; // PID differential coeff. / (1/(°C/s))
    // uint8_t   fan_pin;
    // uint16_t  fan_freq;
    // uint8_t   fan_dc_off;
    // uint8_t   fan_dc_min;
    // uint8_t   fan_dc_max;
    // uint8_t   fan_dc_minsig;
    float     fan_intensity_min;
    float     fan_intensity_max;
    float     error_der_lpf_g;
    // char      fan_pigpio_server_address[100];
    // char      fan_pigpio_server_port[20];
    char      log_fpath[255];
    uint16_t  log_period;
};

int
timespec_init(struct timespec *ts, 
              float duration);

static int
config_iniload_handler(void *user,
                       const char *section,
                       const char *name,
                       const char *value);

int
sys_logger_reset_timer(struct Config   *config,
                       struct StateVec *statevec);

float
filter_folpf(float gamma,
             float prev,
             float new,
             bool  bypass);

#endif // _FAND_H_
