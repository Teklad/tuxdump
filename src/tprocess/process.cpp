#include "process.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>

namespace TProcess {

bool Process::Attach(const char* name, size_t timeout)
{
    DIR *procDir = opendir("/proc");
    if (!procDir) {
        return false;
    }
    while (timeout > 0 && this->PID() == -1) {
        struct dirent* dirEntry;
        while ((dirEntry = readdir(procDir)) != nullptr) {
            int id = strtol(dirEntry->d_name, NULL, 10);
            if (id != 0) {
                char symlinkPath[PATH_MAX];
                char exePath[PATH_MAX];
                snprintf(symlinkPath, PATH_MAX, "/proc/%i/exe", id);
                ssize_t bytesRead = readlink(symlinkPath, exePath, PATH_MAX);
                if (bytesRead == -1) {
                    continue;
                }
                exePath[bytesRead] = 0;
                const char* exeName = basename(exePath);
                if (strcmp(exeName, name) == 0) {
                    this->SetPID(id);
                    const char* dirName = dirname(symlinkPath);
                    strcpy(m_szProcDir, dirName);
                    closedir(procDir);
                    return true;
                }
            }
        }
        timeout--;
        sleep(1);
    }
    closedir(procDir);
    return false;
}

bool Process::ProcessPresent() const
{
    struct stat st;
    return (stat(m_szProcDir, &st) == 0 && S_ISDIR(st.st_mode));
}

/**
 * @brief Given a name, looks up known memory regions and returns a struct
 *        containing the region's offsets and short name.
 *
 * @param name Short name of the region (i.e. client.so)
 * @param region_out Region object to modify
 *
 * @return bool true if the region exists, otherwise false
 */
bool Process::GetRegion(const char* name, Region& region_out) const
{
    for (auto&& r : m_regions) {
        if (strcmp(r.name, name) == 0) {
            region_out = r;
            return true;
        }
    }
    return false;
}

/**
 * @brief Given a name, checks to see if a memory region is present
 *
 * @param name short name of the region (i.e. client.so)
 *
 * @return bool true if the region exists, otherwise false
 */
bool Process::HasRegion(const char* name) const
{
    for (auto &&r : m_regions) {
        if (strcmp(r.name, name) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Given a region name, tries to update the internal regions vector
 *        with updated offsets.  If a match exists but the memory is not
 *        contiguous, this will create a new entry to prevent overlapping.
 *
 * @param name Short name of the region to update
 * @param start Start address of the region
 * @param end   End address of the region
 */
void Process::UpdateRegion(const char* name, uintptr_t start, uintptr_t end)
{
    for (auto&& r : m_regions) {
        if (strcmp(r.name, name) == 0) {
            if (start == r.end) {
                r.end = end;
            } else {
                continue;
            }
            if (start < r.start) {
                r.start = start;
            }
            return;
        }
    }
    TProcess::Region region(name, this->PID(), start, end);
    m_regions.push_back(std::move(region));
}


/**
 * @brief Parses /proc/$pid/maps and updates the internal regions vector
 *        with new offsets.  If the region is not named, it is skipped.
 *
 * @return bool: true if the file was opened successfully, false otherwise
 */
bool Process::ParseMaps()
{
    constexpr size_t bufSize = 0x1000;
    char maps_path[PATH_MAX];
    snprintf(maps_path, PATH_MAX, "/proc/%i/maps", this->PID());
    FILE* fp = fopen(maps_path, "r");
    if (fp) {
        char line[bufSize];
        while (fgets(line, sizeof(line), fp)) {
            line[strlen(line) - 1] = 0;
            char* sep = strchr(line, '-');
            char* name = strrchr(line, '/');
            if (sep != NULL) {
                uintptr_t start = strtoul(line, NULL, 16);
                uintptr_t end = strtoul(sep + 1, NULL, 16);
                if (name == NULL) {
                    name = strrchr(line, ' ');
                    if (name == NULL) {
                        continue;
                    }
                }
                name += 1;
                if (strlen(name) > 0) {
                    this->UpdateRegion(name, start, end);
                }
            }
        }
        fclose(fp);
        return true;
    }
    return false;
}

}
