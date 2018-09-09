#include "process.h"

#include <cstring>

#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>

namespace TProcess {

int Memory::m_iPid = -1;
/**
 * @brief Finds a process with the given name and binds m_iPid to it
 *        for easier memory transactions
 *
 * @param name The name of the process
 *
 * @return true if the process was found, false otherwise
 */
bool Process::Attach(const std::string& name)
{
    DIR* procDir = opendir("/proc");
    if (procDir == nullptr) {
        return false;
    }

    struct dirent* dirEntry;
    while ((dirEntry = readdir(procDir)) != nullptr) {
        int id = strtol(dirEntry->d_name, nullptr, 10);
        if (id > 0) {
            char symlinkPath[FILENAME_MAX] = {0};
            char exePath[FILENAME_MAX] = {0};

            snprintf(symlinkPath, FILENAME_MAX, "/proc/%i/exe", id);

            ssize_t bytesRead = readlink(symlinkPath, exePath, FILENAME_MAX);
            if (bytesRead > 0) {
                const char* exeName = basename(exePath);
                if (strcmp(exeName, name.c_str()) == 0) {
                    const char* dirName = dirname(symlinkPath);
                    strcpy(m_szProcDir, dirName);
                    m_iPid = id;
                    closedir(procDir);
                    return true;
                }
            }
        }
    }
    closedir(procDir);
    return false;
}

/**
 * @brief Verifies that the attached process is running
 *
 * @return true if it is, false otherwise
 */
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
bool Process::GetRegion(const std::string& name, Region& region_out)
{
    for (Region& r : m_regions) {
        char* fileName = basename(r.pathName);
        if (name.compare(fileName) == 0) {
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
bool Process::HasRegion(const std::string& name)
{
    for (Region& r : m_regions) {
        char* fileName = basename(r.pathName);
        if (name.compare(fileName) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Parses /proc/$pid/maps and updates the internal regions vector
 *        with new offsets.  If the region is not named, it is skipped.
 *
 * @return bool: true if the file was opened successfully, false otherwise
 */
bool Process::ParseMaps()
{
    char mapsPath[FILENAME_MAX] = {0};
    snprintf(mapsPath, FILENAME_MAX, "/proc/%i/maps", m_iPid);

    FILE* mapsFile = fopen(mapsPath, "r");
    if (mapsFile == nullptr) {
        return false;
    }

    char *line = nullptr;
    size_t len = 0;

    while (getline(&line, &len, mapsFile) != -1) {
        char pathName[FILENAME_MAX] = {0};
        uintptr_t start, end;

        bool ret = sscanf(line, "%lx-%lx %*4s %*p %*2d:%*2d %*d %[^\t\n]",
                &start, &end, pathName);

        if (ret != 1) {
            fclose(mapsFile);
            free(line);
            return false;
        }

        if (!m_regions.empty() && strcmp(m_regions.back().pathName, pathName) == 0) {
            if (m_regions.back().start > start) {
                m_regions.back().start = start;
            }
            if (m_regions.back().end < end) {
                m_regions.back().end = end;
            }
        } else {
            Region region;
            region.pid = m_iPid;
            region.start = start;
            region.end = end;
            strcpy(region.pathName, pathName);
            m_regions.push_back(region);
        }

    }
    fclose(mapsFile);
    free(line);
    return true;
}

}
