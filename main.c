#include "tuxdump.h"

#include <errno.h>
#include <libconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool tdmain_load_config(config_t *cfg, const char* file)
{
    config_init(cfg);

    if (!config_read_file(cfg, file)) {
        fprintf(stderr, "Config Error: %s:%d - %s\n",
                config_error_file(cfg),
                config_error_line(cfg),
                config_error_text(cfg));
        config_destroy(cfg);
        return false;
    }
    return true;
}

void print_usage()
{
    printf("Usage: tuxdump [pid]\n");
    exit(1);
}

#ifdef DEBUG
void print_module_info(struct tuxdump_module* module)
{
    printf("\t%-30s[%#lx - %#lx]\n", module->name, module->start, module->end);
}
#endif

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
    }

    int pid = strtol(argv[1], NULL, 10);
    if (pid == 0 || errno == ERANGE) {
        print_usage();
    }

    struct tuxdump_module client = {0};
    struct tuxdump_module engine = {0};

    if (!tuxdump_load_module(pid, "client_panorama_client.so", &client)) {
        printf("Failed to find client_panorama_client.so in process: %i\n", pid);
        return 1;
    }

    if (!tuxdump_load_module(pid, "engine_client.so", &engine)) {
        printf("Failed to find engine_client.so in process: %i\n", pid);
        return 1;
    }

#ifdef DEBUG
    printf("Module Information:\n");
    print_module_info(&client);
    print_module_info(&engine);
#endif

    config_t cfg;
    config_setting_t* root;
    config_setting_t* signatures;
    if (!tdmain_load_config(&cfg, "csgo.cfg")) {
        return EXIT_FAILURE;
    }

    root = config_root_setting(&cfg);
    signatures = config_setting_get_member(root, "signatures");
    if (!signatures) {
        fprintf(stderr, "Missing signatures entry in config file.\n");
    }

    size_t signature_count = config_setting_length(signatures);

#ifdef DEBUG
    printf("\nSignatures: %li\n\n", signature_count);
#endif
    
    printf("%-35s %-20s %20s\n", "Module", "Signature", "Offset");
    for (size_t i = 0; i < signature_count; ++i) {
        config_setting_t* sig_root;
        const char *module, *pattern;
        int offset, extra, relative;
        sig_root = config_setting_get_elem(signatures, i);
        if ( !(config_setting_lookup_string(sig_root, "module", &module) &&
               config_setting_lookup_string(sig_root, "pattern", &pattern) &&
               config_setting_lookup_int(sig_root, "offset", &offset) &&
               config_setting_lookup_int(sig_root, "extra", &extra) &&
               config_setting_lookup_int(sig_root, "relative", &relative))) {
            continue;
        }

        uintptr_t lookup = 0;

        if (strcmp(module, client.name) == 0) {
            if (tuxdump_find_address(&client, pattern, offset, &lookup)) {
                lookup = tuxdump_call_address(&client, lookup) + extra;
            }
        } else if (strcmp(module, engine.name) == 0) {
            if (tuxdump_find_address(&engine, pattern, offset, &lookup)) {
                lookup = tuxdump_call_address(&client, lookup) + extra;
            }
        }
        if (lookup != 0) {
            printf("%-35s %-20s %#20lx\n", module, config_setting_name(sig_root), 
                    lookup - (relative ? client.start : 0));
        } else {
            printf("%-35s %-20s %20s\n", module, config_setting_name(sig_root), "?");
        }
    }

    config_destroy(&cfg);
    tuxdump_free_module(&client);
    tuxdump_free_module(&engine);
    return 0;
}

