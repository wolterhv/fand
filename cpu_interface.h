#ifndef   _CPU_INTERFACE_
#define   _CPU_INTERFACE_

struct CPUIFConfig;
struct CPUIFState;

struct CPUIFConfig*
cpu_if_config_new();

struct CPUIFState*
cpu_if_state_new();

int 
cpu_if_load_config(struct CPUIFConfig *config,
                   struct CPUIFState  *state);


int
cpu_if_read_temperature(const struct CPUIFConfig *config,
                        const struct CPUIFState  *state,
                        float *tempr);

int
cpu_if_read_use(const struct CPUIFConfig *config,
                const struct CPUIFState  *state,
                float *use);

int
cpu_if_init(struct CPUIFConfig *config,
            struct CPUIFState  *state);

int
cpu_if_term(struct CPUIFConfig *config,
            struct CPUIFState  *state);


#endif // _CPU_INTERFACE_
