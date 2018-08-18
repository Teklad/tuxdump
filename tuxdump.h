#ifndef  __TUXDUMP_H__
#define  __TUXDUMP_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief A struct containing the needed information to interact with a memory address
 */
struct tuxdump_module {
    char *name;
    uintptr_t start;
    uintptr_t end;
    int pid;
};


uintptr_t tuxdump_absolute_address(const struct tuxdump_module* module,
        uintptr_t addr, size_t offset, size_t extra);
uintptr_t tuxdump_call_address(const struct tuxdump_module* module, uintptr_t addr);
bool tuxdump_find_address(const struct tuxdump_module* module,
        const char* pattern, size_t offset, uintptr_t *addr_out);
bool tuxdump_load_module(int pid, const char* name, struct tuxdump_module* module_out);
void tuxdump_free_module(struct tuxdump_module* module);

#endif //__TUXDUMP_H__
