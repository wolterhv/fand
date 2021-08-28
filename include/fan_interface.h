#ifndef   _FAN_INTERFACE_
#define   _FAN_INTERFACE_

#define FAN_INTENSITY_OFF  -1
#define FAN_INTENSITY_MIN   0
#define FAN_INTENSITY_MAX 100

struct FanIFConfig;

struct FanIFState;

struct FanIFConfig*
fan_if_config_new();

struct FanIFState*
fan_if_state_new();

int
fan_if_load_config(struct FanIFConfig *config,
                   struct FanIFState  *state);

int
fan_if_set_intensity(const struct FanIFConfig *config,
                     const struct FanIFState  *state,
                     float intensity);

int
fan_if_init(struct FanIFConfig *config,
            struct FanIFState  *state);


int
fan_if_term(struct FanIFConfig *config,
            struct FanIFState  *state);

#endif // _FAN_INTERFACE_

