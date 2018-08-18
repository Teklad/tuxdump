#define _GNU_SOURCE
#include "tuxdump.h"

#include <sys/uio.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TUXDUMP_PATTERN_MAX 256
#define TUXDUMP_LINE_MAX 512
#define TUXDUMP_BUFFER_SIZE 0x1000

struct tuxdump_binary {
    bool    mask[TUXDUMP_PATTERN_MAX];
    uint8_t byte[TUXDUMP_PATTERN_MAX];
    size_t len;
};

/**
 * @brief Initializes the lookup table if it isn't already.  Return hltbl
 *        containing valid hexadecimal characters
 *
 * @return hexadecimal lookup table
 */
static uint8_t *tuxdump_hextable()
{
    static uint8_t hltbl[255] = {0};
    if (hltbl['1'] == 0) {
        memset(hltbl, 256, sizeof(hltbl));
        hltbl['0'] = 0;
        hltbl['1'] = 1;
        hltbl['2'] = 2;
        hltbl['3'] = 3;
        hltbl['4'] = 4;
        hltbl['5'] = 5;
        hltbl['6'] = 6;
        hltbl['7'] = 7;
        hltbl['8'] = 8;
        hltbl['9'] = 9;
        hltbl['a'] = 10;
        hltbl['A'] = 10;
        hltbl['b'] = 11;
        hltbl['B'] = 11;
        hltbl['c'] = 12;
        hltbl['C'] = 12;
        hltbl['d'] = 13;
        hltbl['D'] = 13;
        hltbl['e'] = 14;
        hltbl['E'] = 14;
        hltbl['f'] = 15;
        hltbl['F'] = 15;
    }
    return hltbl;
}

/**
 * @brief Constructs flexible binary patterns with lenient syntax.
 *        Each '?' character counts as one binary wildcard.
 *        Valid syntaxes:
 *            - "ab?cd?ef"
 *            - "a b c ? e f"
 *            - "ab c?d ef?"
 *
 * @param dst The destination tuxdump_binary structure to fill
 * @param src The original source string containing hexadecimal characters
 * @param len The length of the source string
 *
 * @return true if parsing encountered no errors, otherwise false
 */
static bool tuxdump_hex2bin(struct tuxdump_binary* dst, const char* src, size_t len)
{
    const uint8_t* usrc = (const uint8_t*)src;
    const uint8_t* hl = tuxdump_hextable();
    int16_t byte = -1;

    memset(dst->byte, 0, TUXDUMP_PATTERN_MAX);
    memset(dst->mask, 0, TUXDUMP_PATTERN_MAX);
    dst->len = 0;

    for (size_t i = 0; i < len; i++) {
        if (byte != -1) {
            if(usrc[i] == ' ' || usrc[i] == '?') {
                dst->byte[dst->len] = byte;
                dst->mask[dst->len] = false;
                if (usrc[i] == '?') {
                    dst->len++;
                    dst->mask[dst->len] = true;
                }
            } else {
                if (!isxdigit(usrc[i])) {
                    return false;
                }
                dst->byte[dst->len] = (byte << 4) | hl[usrc[i]];
                dst->mask[dst->len] = false;
            }
            byte = -1;
            dst->len++;
        } else {
            if (usrc[i] == ' ') continue;
            if (usrc[i] == '?') {
                dst->mask[dst->len] = true;
                dst->len++;
            } else {
                if (!isxdigit(usrc[i])) {
                    return false;
                }
                byte = hl[usrc[i]];
            }
        }
    }

    if (byte != -1) {
        dst->byte[dst->len] = byte;
        dst->mask[dst->len] = false;
        dst->len++;
    }
    return true;
}

static bool tuxdump_read(int pid, uintptr_t addr, void* out, size_t len)
{
    struct iovec local;
    struct iovec remote;

    local.iov_base = out;
    local.iov_len = len;
    remote.iov_base = (void*)(addr);
    remote.iov_len = len;

    return process_vm_readv(pid, &local, 1, &remote, 1, 0) > 0;
}


uintptr_t tuxdump_absolute_address(const struct tuxdump_module* module,
        uintptr_t addr, size_t offset, size_t extra)
{
    uint32_t code = 0;
    if (tuxdump_read(module->pid, addr + offset, &code, sizeof(uint32_t))) {
        return addr + extra + code;
    }
    return 0;
}

uintptr_t tuxdump_call_address(const struct tuxdump_module* module, uintptr_t addr)
{
    return tuxdump_absolute_address(module, addr, 1, 5);
}

bool tuxdump_load_module(int pid, const char* name, struct tuxdump_module* module_out)
{
    char maps_path[PATH_MAX];
    snprintf(maps_path, PATH_MAX, "/proc/%i/maps", pid);
    FILE* fp = fopen(maps_path, "r");
    
    if (fp != NULL) {
        size_t name_len = strlen(name);
        module_out->name = malloc((name_len * sizeof(char)) + 1);
        module_out->pid = pid;
        module_out->start = 0;
        module_out->end = 0;
        
        memcpy(module_out->name, name, name_len + 1);

        char line[TUXDUMP_LINE_MAX];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, name) != NULL) {
                char* sep = strchr(line, '-');
                if (sep != NULL) {
                    if (module_out->start == 0) {
                        module_out->start = strtoul(line, NULL, 16);
                    }
                    module_out->end = strtoul(sep + 1, NULL, 16);
                }
            }
        }

        fclose(fp);
        return (module_out->start != 0);
    }

    return false;
}


bool tuxdump_find_address(const struct tuxdump_module* module,
        const char* pattern, size_t offset, uintptr_t* addr_out)
{
    uint8_t buffer[TUXDUMP_BUFFER_SIZE];

    struct tuxdump_binary bin;
    if (!tuxdump_hex2bin(&bin, pattern, strlen(pattern))) {
        printf("Error in tuxdump_hex2bin: Parsing failure. '%s' contains invalid characters\n", pattern);
        return false;
    }
    
    uintptr_t addr = 0;
    size_t matches = 0;
    size_t chunk = 0;
    size_t total_size = module->end - module->start;
    while (total_size > 0) {
        size_t read_size = (total_size < TUXDUMP_BUFFER_SIZE) ? total_size : TUXDUMP_BUFFER_SIZE;
        uintptr_t read_addr = module->start + (TUXDUMP_BUFFER_SIZE * chunk);
        memset(buffer, 0, TUXDUMP_BUFFER_SIZE);
        if (tuxdump_read(module->pid, read_addr, &buffer, read_size)) {
            for (uintptr_t b = 0; b < read_size; b++) {
                if (bin.mask[matches] || bin.byte[matches] == buffer[b]) {
                    matches++;
                    if (addr == 0) {
                        addr = read_addr + b;
                    }
                    if (matches == bin.len) {
                        *addr_out = addr + offset;
                        return true;
                    }
                } else {
                    addr = 0;
                    matches = 0;
                }

            }
        }
        total_size -= read_size;
        chunk++;
    }
    return false;
}

void tuxdump_free_module(struct tuxdump_module* module)
{
    free(module->name);
}
