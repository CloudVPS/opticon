#include <libopticon/react.h>
#include <libopticon/log.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

int CONF_LPORT = 0;
char CONF_ADDR[256];
char CONF_KEY[256];

int conf_listenport (const char *id, var *v, updatetype tp) {
    CONF_LPORT = var_get_int (v);
    switch (tp) {
        case UPDATE_CHANGE:
            log_info ("Changed listenport: %i", CONF_LPORT);
            break;
        
        case UPDATE_ADD:
            log_info ("Added listenport: %i", CONF_LPORT);
            break;
        
        case UPDATE_REMOVE:
            log_error ("Request to remove listenport");
            assert (0);
            break;
            
        default:
            break;
    }
    
    log_info ("Got listenport: %i", CONF_LPORT);
    return 1;
}

int conf_address (const char *id, var *v, updatetype tp) {
    if (tp != UPDATE_REMOVE) {
        strncpy (CONF_ADDR, var_get_str (v), 255);
        CONF_ADDR[255] = 0;
    }
    return 1;
}

int conf_key (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) {
        CONF_KEY[0] = 0;
        return 1;
    }
    strncpy (CONF_KEY, var_get_str (v), 255);
    CONF_KEY[255] = 0;
    return 1;
}

int main (int argc, const char *argv[]) {
    const char *tstr = NULL;
    
    opticonf_add_reaction ("collector/listenport", conf_listenport);
    opticonf_add_reaction ("collector/address", conf_address);
    opticonf_add_reaction ("collector/key", conf_key);
    
    var *env = var_alloc();
    var *env_collector = var_get_dict_forkey (env, "collector");
    var *env_colors = var_get_array_forkey (env, "colors");
    var_set_int_forkey (env_collector, "listenport", 3333);
    var_set_str_forkey (env_collector, "address", "127.0.0.1");
    var_set_str_forkey (env_collector, "key", "secret");
    var_clear_array (env_colors);
    var_add_str (env_colors, "red");
    var_add_str (env_colors, "green");
    var_add_str (env_colors, "blue");
    
    opticonf_handle_config (env);
    assert (CONF_LPORT == 3333);
    assert (strcmp (CONF_ADDR, "127.0.0.1") == 0);
    assert (strcmp (CONF_KEY, "secret") == 0);
    
    var_new_generation (env);
    var_set_int_forkey (env_collector, "listenport", 3333);
    var_set_str_forkey (env_collector, "address", "192.168.1.1");
    var_clear_array (env_colors);
    var_add_str (env_colors, "red");
    var_add_str (env_colors, "green");
    var_add_str (env_colors, "blue");
    
    opticonf_handle_config (env);

    assert (CONF_LPORT == 3333);
    assert (strcmp (CONF_ADDR, "192.168.1.1") == 0);
    assert (CONF_KEY[0] == 0);
    
    var_clean_generation (env);
    return 0;
}
