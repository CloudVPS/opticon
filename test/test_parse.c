#include <libopticonf/parse.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main (int argc, const char *argv[]) {
    const char *txt = 
        "collector {"
        "  listenport 3333"
        "  address 192.168.1.1"
        "  key \"johnWithTheLongShanks\""
        "},"
        "colors [red,green,blue]";
    
    var *env = var_alloc();
    var *env_collector = NULL;
    const char *addr;
    assert (parse_config (env, txt));
    assert (env_collector = var_get_dict_forkey (env, "collector"));
    assert (var_get_int_forkey (env_collector, "listenport") == 3333);
    assert (addr = var_get_str_forkey (env_collector, "address"));
    assert (strcmp (addr, "192.168.1.1") == 0);
    return 0;
}
