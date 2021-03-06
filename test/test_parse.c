#include <libopticon/var_parse.h>
#include <libopticon/var_dump.h>
#include <libopticon/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_input (const char *txt) {
    var *env = var_alloc();
    var *env_collector = NULL;
    var *env_colors = NULL;
    const char *tstr;
    assert (var_parse_json (env, txt));
    assert (env_collector = var_get_dict_forkey (env, "collector"));
    assert (var_get_int_forkey (env_collector, "listenport") == 3333);
    assert (tstr = var_get_str_forkey (env_collector, "address"));
    assert (strcmp (tstr, "192.168.1.1") == 0);
    assert (tstr = var_get_str_forkey (env_collector, "key"));
    assert (strcmp (tstr, "johnWithTheLongShanks") == 0);
    assert (env_colors = var_get_array_forkey (env, "colors"));
    assert (var_get_count (env_colors) == 3);
    assert (tstr = var_get_str_atindex (env_colors, 0));
    assert (strcmp (tstr, "red") == 0);
    assert (tstr = var_get_str_atindex (env_colors, 2));
    assert (strcmp (tstr, "blue") == 0);
    var_free (env);
}

void test_faulty_input (const char *txt) {
    var *env = var_alloc();
    assert (! var_parse_json (env, txt));
    log_info ("Caught error: %s", parse_error());
    var_free (env);
}

void dump_input (const char *txt) {
    var *env = var_alloc();
    assert (var_parse_json (env, txt));
    var_dump (env, stdout);
    var_free (env);
}

int main (int argc, const char *argv[]) {
    test_input (
        "collector {"
        "  listenport 3333"
        "  address 192.168.1.1"
        "  key \"johnWithTheLongShanks\""
        "},"
        "colors [red,green,blue]"
    );
    
    test_input (
        "collector: {\n"
        "    # Network settings\n"
        "    listenport:3333\n"
        "    address: 192.168.1.1# Primary\n"
        "    key:\"johnWithTheLongShanks\"\n"
        "}\n"
        "colors: [\"red\",\"green\",\"blue\"]\n"
    );
    
    dump_input (
        "collector: {\n"
        "    # Network settings\n"
        "    listenport:3333\n"
        "    address: 192.168.1.1# Primary\n"
        "    key:\"johnWithTheLongShanks\"\n"
        "}\n"
        "colors: [\"red\",\"green\",\"blue\"]\n"
    );
    
    test_faulty_input (
        "collector: {\n"
        "    # Network settings\n"
        "    listenport:3333\n"
        "    address: 192.168.1.1# Primary\n"
        "    key:\"johnWithTheLongShanks\"\n"
        "}\n"
        "colors: <\"red\",\"green\",\"blue\"]\n"
    );
    
    
    
    return 0;
}
