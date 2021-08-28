#include "cpu_interface.h"

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

struct CPUIFConfig {
    char tempr_fpath [255];
    char use_fpath   [255];
} CPUIFConfig;

struct CPUIFState {
    FILE *tempr_fobj;
    FILE *use_fobj;
} CPUIFState;

struct CPUIFConfig*
cpu_if_config_new()
{
    return (struct CPUIFConfig*) malloc(sizeof(struct CPUIFConfig));
}

struct CPUIFState*
cpu_if_state_new()
{
    return (struct CPUIFState*) malloc(sizeof(struct CPUIFState));
}

static int
cpu_if_config_iniload_handler(      void *user,
                              const char *section,
                              const char *name,
                              const char *value)
{
#define MATCH(s, n) ((strcmp(section, s) == 0) && (strcmp(name, n) == 0))
#define SET_OPT_I(s,p,type) else if (MATCH(#s,#p)) { ((struct CPUIFConfig*) user)->p = (type)  atoi(value); }
#define SET_OPT_F(s,p)      else if (MATCH(#s,#p)) { ((struct CPUIFConfig*) user)->p = (float) atof(value); }
#define SET_OPT_S(s,p)      else if (MATCH(#s,#p)) { strcpy(((struct CPUIFConfig*) user)->p, value); }

    if (false) { return 0; }
    SET_OPT_S(cpu_interface, tempr_fpath)
    SET_OPT_S(cpu_interface, use_fpath)
    else       { return 0; }

#undef SET_OPT_I
#undef SET_OPT_F
#undef SET_OPT_S
#undef MATCH
}

int
cpu_if_load_config(struct CPUIFConfig *config,
                   struct CPUIFState  *state)
{
    static char config_fpath[255];

    sprintf(config_fpath, "%s/fand/config.ini", getenv("XDG_CONFIG_HOME"));
    ini_parse(config_fpath, cpu_if_config_iniload_handler, (void*) config); // TODO error catching
    printf("cpu_if_generic config loaded.\n");
    return 0;
}

int
cpu_if_read_temperature(const struct CPUIFConfig *config,
                        const struct CPUIFState  *state,
                        float *tempr)
{
#define CHARCOUNT 20
    static char tempr_str[CHARCOUNT];
    strcpy(tempr_str,"");
    fseek(state->tempr_fobj, 0, SEEK_SET);
    fread_unlocked((char*) tempr_str, 1, CHARCOUNT, state->tempr_fobj);
#undef CHARCOUNT
    *tempr = (atof(tempr_str)/1000.0);
    return 0;
}

// int
// cpu_if_read_use(const struct CPUIFConfig *config,
//                 const struct CPUIFState  *state,
//                 float *use);

int
cpu_if_init(struct CPUIFConfig *config,
            struct CPUIFState  *state)
{
    cpu_if_load_config(config, state);

    state->tempr_fobj = fopen(config->tempr_fpath, "r");
    // state->use_fobj = fopen(config->use_fpath, "r");
    return 0;
}

int
cpu_if_term(struct CPUIFConfig *config,
            struct CPUIFState  *state)
{
    fclose(state->tempr_fobj);
    // fclose(state->use_fobj);
    
    return 0;
}
