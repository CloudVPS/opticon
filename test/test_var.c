#include <libopticonf/var.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main (int argc, const char *argv[]) {
    const char *tstr = NULL;
    var *env = var_alloc();
    var *env_collector = var_get_dict (env, "collector");
    var_set_int (env_collector, "listenport", 3333);
    var_set_str (env_collector, "address", "127.0.0.1");
    var_set_str (env_collector, "key", "secret");
    
    assert (var_get_int (env_collector, "listenport") == 3333);
    assert (tstr = var_get_str (env_collector, "address"));
    assert (strcmp (tstr, "127.0.0.1") == 0);
    assert (tstr = var_get_str (env_collector, "key"));
    assert (strcmp (tstr, "secret") == 0);
    assert (var_get_count (env_collector) == 3);
    
    var_new_generation (env);
    var_set_int (env_collector, "listenport", 3333);
    var_set_str (env_collector, "address", "192.168.1.1");
    
    var *lport, *addr, *key;
    lport = var_find_key (env_collector, "listenport");
    addr = var_find_key (env_collector, "address");
    key = var_find_key (env_collector, "key");
    assert (lport);
    assert (addr);
    assert (key);
    
    /* collector.listenport should be unchanged from first generation */
    assert (lport->generation == env->generation);
    assert (lport->lastmodified < env->generation);
    assert (lport->firstseen == lport->lastmodified);
    
    /* collector.addr should be changed this generation */
    assert (addr->generation == env->generation);
    assert (addr->lastmodified == env->generation);
    assert (addr->firstseen == env->firstseen);
    
    /* collector.key should be stale */
    assert (key->generation < env->generation);
    
    var_clean_generation (env);
    
    /* collector.key should now be deleted */
    key = var_find_key (env_collector, "key");
    assert (key == NULL);
    return 0;
}
