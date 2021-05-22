#include "fand.h"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define CLIP(min,max,x) (MAX((min),(MIN((max),(x)))))

#define LOG_ROW_MAXLEN     1000
#define DATETIMESTR_MAXLEN 25

#define APP_STATE_INIT        0b00000001
#define APP_STATE_OPER        0b00000010
#define APP_STATE_TERM        0b00000100
#define APP_STATE_QUIT        0b00001000
#define APP_STATE_CONF_RELOAD 0b00010000
#define APP_STATE_LOG_CUE     0b00100000

static char app_state = APP_STATE_INIT;

void
app_sighandler(int sig)
{
    if        (sig == SIGINT) {
        printf("\rSIGINT received.\n");
        app_state = APP_STATE_TERM;
    } else if (sig == SIGUSR1) {
        printf("\rSIGUSR1 received.\n");
        app_state |= APP_STATE_CONF_RELOAD;
    } else if (sig == SIGALRM) {
        printf("\rSIGALRM received.\n");
        app_state |= APP_STATE_LOG_CUE;
    }
}

int
statevec_init(struct StateVec *statevec)
{
    statevec->prev                                =          NULL;
    statevec->cycle_period.tv_sec                 =  (time_t) 0.0;
    statevec->cycle_period.tv_nsec                =             0;
    statevec->datetime                            =  (time_t) 0.0;
    statevec->time                                =           0.0;
    statevec->cpu_tempr                           =           0.0;
    statevec->cpu_use                             =           0.0;
    statevec->error                               =           0.0;
    statevec->error_int                           =           0.0;
    statevec->error_der                           =           0.0;
    statevec->fan_intensity                       =             0;
    statevec->fan_hyst                            =         false;
    statevec->log_fobj                            =          NULL;
    statevec->log_timer_id                        =             0;
    statevec->log_timer_value.it_interval.tv_sec  = (time_t)  0.0;
    statevec->log_timer_value.it_interval.tv_nsec =             0;
    statevec->log_timer_value.it_value.tv_sec     = (time_t)  0.0;
    statevec->log_timer_value.it_value.tv_nsec    =             0;
    return 0;
}

int
statevec_copy(const struct StateVec *src_statevec,
                    struct StateVec *dst_statevec)
{
    dst_statevec->cycle_period.tv_sec                 = src_statevec->cycle_period.tv_sec;
    dst_statevec->cycle_period.tv_nsec                = src_statevec->cycle_period.tv_nsec;
    dst_statevec->datetime                            = src_statevec->datetime;
    dst_statevec->time                                = src_statevec->time;
    dst_statevec->cpu_tempr                           = src_statevec->cpu_tempr;
    dst_statevec->cpu_use                             = src_statevec->cpu_use;
    dst_statevec->error                               = src_statevec->error;
    dst_statevec->error_int                           = src_statevec->error_int;
    dst_statevec->error_der                           = src_statevec->error_der;
    dst_statevec->fan_intensity                       = src_statevec->fan_intensity;
    dst_statevec->log_fobj                            = src_statevec->log_fobj;
    dst_statevec->log_timer_id                        = src_statevec->log_timer_id;
    dst_statevec->log_timer_value.it_interval.tv_sec  = src_statevec->log_timer_value.it_interval.tv_sec;
    dst_statevec->log_timer_value.it_interval.tv_nsec = src_statevec->log_timer_value.it_interval.tv_nsec;
    dst_statevec->log_timer_value.it_value.tv_sec     = src_statevec->log_timer_value.it_value.tv_sec;
    dst_statevec->log_timer_value.it_value.tv_nsec    = src_statevec->log_timer_value.it_value.tv_nsec;
    return 0;
}

int
config_load(struct Config   *config,
            struct StateVec *statevec)
{
    static char config_fpath[255];

    sprintf(config_fpath, "%s/fand/config.ini", getenv("XDG_CONFIG_HOME"));
    ini_parse(config_fpath, config_iniload_handler, (void*) config); // TODO error catching

    timespec_init(&(statevec->cycle_period),             (float) config->sys_cycle_period);
    timespec_init(&(statevec->log_timer_value.it_value), (float) config->log_period);

    printf("config loaded.\n");
    return 0;
}

static int
config_iniload_handler(      void *user,
                       const char *section,
                       const char *name,
                       const char *value)
{
#define MATCH(s, n) ((strcmp(section, s) == 0) && (strcmp(name, n) == 0))
#define SET_PARAM_I(p,type) else if (MATCH("params",#p)) { ((struct Config*) user)->p = (type)  atoi(value); }
#define SET_PARAM_F(p)      else if (MATCH("params",#p)) { ((struct Config*) user)->p = (float) atof(value); }
#define SET_PARAM_S(p)      else if (MATCH("params",#p)) { strcpy(((struct Config*) user)->p, value); }

    if (false) { return 0; }
    SET_PARAM_F(sys_cycle_period)
    SET_PARAM_F(cpu_tempr_tgt)
    SET_PARAM_F(cpu_tempr_tgt)
    SET_PARAM_F(cpu_tempr_on)
    SET_PARAM_F(cpu_tempr_off)
    SET_PARAM_F(cpu_tempr_lpf_g)
    SET_PARAM_F(error_der_lpf_g)
    SET_PARAM_F(pid_kp)
    SET_PARAM_F(pid_tn)
    SET_PARAM_F(pid_tv)
    SET_PARAM_F(pid_imax)
    SET_PARAM_F(pid_imin)
    SET_PARAM_S(log_fpath)
    SET_PARAM_I(log_period,uint16_t)
    else { return 0; }

#undef SET_PARAM_I
#undef SET_PARAM_F
#undef SET_PARAM_S
#undef MATCH
}

int
sys_controller_update(struct Config   *config,
                      struct StateVec *statevec)
{
    float dt            = 0;
    float pd_contrib    = 0;

    dt = MIN(statevec->time - statevec->prev->time, config->sys_cycle_period);

    statevec->error     =  -(config->cpu_tempr_tgt - statevec->cpu_tempr);
    statevec->error_der =  filter_folpf(config->error_der_lpf_g,
                                        statevec->error_der,
                                        (statevec->error - statevec->prev->error) / dt,
                                        false);
    // TODO find the root cause of der. error going to inf.
    statevec->error_der = (isinf(statevec->error_der))? 
                            statevec->prev->error_der 
                          : statevec->error_der;
    // Static integrator clipping
    // statevec->error_int = CLIP(config->pid_imin,
    //                            config->pid_imax,
    //                              statevec->prev->error_int 
    //                            + statevec->error * dt);

    // Dynamic integrator clipping
    // https://e2e.ti.com/blogs_/b/industrial_strength/posts/teaching-your-pi-controller-to-behave-part-vii
    pd_contrib =   config->pid_kp                * statevec->error;
                 + config->pid_kp*config->pid_tv * statevec->error_der;

    // TODO Try clipping after calculation too
    statevec->error_int = CLIP(MIN(FAN_INTENSITY_MIN - pd_contrib, 0),
                               MAX(FAN_INTENSITY_MAX - pd_contrib, 0),
                                 statevec->prev->error_int 
                               + statevec->error * dt);

    // Fan intensity clipping
    statevec->fan_intensity = CLIP(FAN_INTENSITY_MIN,
                                   FAN_INTENSITY_MAX,
                                     config->pid_kp                * statevec->error
                                   + config->pid_kp/config->pid_tn * statevec->error_int
                                   + config->pid_kp*config->pid_tv * statevec->error_der);

    // Temperature limit hysteresis
    if      (statevec->cpu_tempr >= config->cpu_tempr_on)
        statevec->fan_hyst = true;
    else if (statevec->cpu_tempr <  config->cpu_tempr_off)
        statevec->fan_hyst = false;

    // // PWM DC hysteresis
    statevec->fan_intensity = (statevec->fan_hyst)?
                                statevec->fan_intensity
                              : FAN_INTENSITY_OFF;
    // /* state->intensity = pwm_dc_raw; */

    return 0;
}

float
filter_folpf(float gamma,
             float prev,
             float new,
             bool  bypass)
{
    return (bypass) ? new : (gamma*new + (1-gamma)*prev);
}

int
sys_plant_init(struct Config   *config,
               struct StateVec *statevec)
{
    return 0;
}

int
sys_plant_term(struct Config   *config,
                    struct StateVec *statevec)
{
    return 0;
}

int
sys_plant_update_inputs(struct Config   *config,
                        struct StateVec *statevec)
{
    static struct timespec tp;

    statevec->datetime = time(NULL);

    clock_gettime(CLOCK_MONOTONIC, &tp);
    statevec->time = ((float) (1000.0*tp.tv_sec) + (float) (tp.tv_nsec/1e6))/1000.0;

    statevec->cpu_tempr = filter_folpf(config->cpu_tempr_lpf_g,
                                       statevec->prev->cpu_tempr,
                                       statevec->cpu_tempr,
                                       (statevec->prev->cpu_tempr == 0.0));
    return 0;
}

int
format_datetime(char *dt_str, time_t time)
{
    struct tm *lt;
    lt = localtime(&time);
    strftime(dt_str, 20, "%FT%T%z", lt);
    return 0;
}

int
timespec_init(struct timespec *ts, 
              float duration) 
{
    ts->tv_sec  = (time_t)   floor(duration);
    ts->tv_nsec = (long int) ((duration - ts->tv_sec)*1e9);
    return 0;
}

int
statevec_log_timer_update(const struct Config *config,
                                struct StateVec *statevec)
{
    return 0;
}

int
sys_logger_init(struct Config   *config,
                struct StateVec *statevec)
{
    statevec->log_fobj = fopen(config->log_fpath,"a"); // TODO check errors
    timer_create(CLOCK_MONOTONIC, NULL, &(statevec->log_timer_id));
    sys_logger_reset_timer(config, statevec);
    return 0;
}

int
sys_logger_term(struct Config   *config,
                     struct StateVec *statevec)
{
    timer_delete(statevec->log_timer_id);
    fclose(statevec->log_fobj);
    return 0;
}

int
sys_logger_update(struct Config   *config,
                  struct StateVec *statevec)
{
    if (app_state & APP_STATE_LOG_CUE) {
        struct stat buf;
        char   row[LOG_ROW_MAXLEN];
        char   dt_str[DATETIMESTR_MAXLEN];
        // 2021-05-14T15:49:30+0100
        // use fstat to see if file was removed (st_nlinks would be 0)
        fstat(fileno(statevec->log_fobj), &buf);
        if (buf.st_nlink == 0) {
            sys_logger_init(config, statevec);
        }
        // Row: time cpu_use cpu_tempr fan_dc
        format_datetime(dt_str, statevec->datetime);
        fprintf(statevec->log_fobj, 
                "%.2f;%s;%u;%.2f;%.2f\n", (double)       statevec->time, 
                                                         dt_str,
                                          (unsigned int) statevec->cpu_use,
                                          (double)       statevec->cpu_tempr,
                                          (double)       statevec->fan_intensity);
        fflush(statevec->log_fobj);

        printf("state logged.\n");

        sys_logger_reset_timer(config, statevec);
        // Clear log cue app_state
        app_state ^= APP_STATE_LOG_CUE;
    }

    printf("(T e i d u) = %.2f\t%.2f\t%.2f\t%.2f\t%.2f\n", 
           statevec->cpu_tempr,
           statevec->error,
           statevec->error_int,
           statevec->error_der,
           statevec->fan_intensity);

    return 0;
}

int
sys_logger_reset_timer(struct Config   *config,
                       struct StateVec *statevec)
{
    timer_settime(statevec->log_timer_id,
                  0,
                  &(statevec->log_timer_value),
                  NULL);
}

int
main(int   argc, 
     char *argv[])
{
    struct Config   config;
    struct StateVec statevec;
    struct StateVec statevec_prev;

    struct FanIFConfig *fan_if_config = fan_if_config_new();
    struct FanIFState  *fan_if_state  = fan_if_state_new();
    struct CPUIFConfig *cpu_if_config = cpu_if_config_new();
    struct CPUIFState  *cpu_if_state  = cpu_if_state_new();

    signal(SIGINT, app_sighandler);
    signal(SIGUSR1, app_sighandler);
    signal(SIGALRM, app_sighandler);

    while (!(app_state & APP_STATE_QUIT)) {
        /* for (uint16_t k = 0; k < taskset.size; k++) { */
        /*     struct Task *task = taskset.tasks[k]; */
        /*     if (statevec.time % task->period == 0) { */
        /*         task->function(&config, &statevec); */
        /*     } */
        /* } */

        if        (app_state & APP_STATE_INIT) {
            printf("APP_STATE_INIT\n");
            statevec_init(&statevec);
            statevec_init(&statevec_prev);
            statevec.prev = &statevec_prev;

            config_load(&config, &statevec);

            sys_logger_init(&config, &statevec);
            sys_plant_init(&config, &statevec);

            // fan_init(&config, &statevec);
            fan_if_init(fan_if_config, fan_if_state);
            cpu_if_init(cpu_if_config, cpu_if_state);

            sys_plant_update_inputs (&config, &statevec);

            app_state = APP_STATE_OPER;
        } else if (app_state & APP_STATE_OPER) {
            // printf("APP_STATE_OPER\n");
            // if        (statevec.time % 100) {
            //
                if (app_state & APP_STATE_CONF_RELOAD) {
                    config_load(&config, &statevec);
                    fan_if_load_config(fan_if_config, fan_if_state);
                    cpu_if_load_config(cpu_if_config, cpu_if_state);

                    app_state ^= APP_STATE_CONF_RELOAD;
                }

                // Inputs
                // ------
                cpu_if_read_temperature(cpu_if_config,
                                        cpu_if_state,
                                        &(statevec.cpu_tempr));

                sys_plant_update_inputs  (&config, &statevec);

                // Processing
                // ----------
                sys_controller_update    (&config, &statevec);

                // Outputs
                // -------
                sys_logger_update        (&config, &statevec);
                // sys_plant_update_outputs (&config, &statevec);

                fan_if_set_intensity(fan_if_config,
                                     fan_if_state,
                                     statevec.fan_intensity);

            // } else (statevec.time %  10) {
            // } else (statevec.time %   1) {
            // }
        } else if (app_state & APP_STATE_TERM) {
            printf("APP_STATE_TERM\n");

            fan_if_term(fan_if_config,
                        fan_if_state);
            cpu_if_term(cpu_if_config,
                        cpu_if_state);

            sys_plant_term(&config, &statevec);
            sys_logger_term(&config, &statevec);
            app_state = APP_STATE_QUIT;
        }
        statevec_copy(&statevec, &statevec_prev);
        nanosleep(&(statevec.cycle_period), NULL);
    }

    free(fan_if_config);
    free(fan_if_state);
    free(cpu_if_config);
    free(cpu_if_state);

    return 0;
}
