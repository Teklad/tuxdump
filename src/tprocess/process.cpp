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
bool Process::Attach(const char* name)
{
    m_iPid = -1;
    DIR* directory = opendir("/proc");
    if (directory) {
        struct dirent* entry;
        while ((entry = readdir(directory)) != nullptr) {
            int id = strtol(entry->d_name, nullptr, 10);
            if (id > 0) {
                char symlinkFile[128];
                snprintf(symlinkFile, sizeof(symlinkFile), "/proc/%i/exe", id);

                char executablePath[FILENAME_MAX] = {0};
                ssize_t bytesRead = readlink(symlinkFile, executablePath,
                        sizeof(executablePath));
                if (bytesRead > 0) {
                    const char* executableFile = basename(executablePath);
                    if (strcmp(executableFile, name) == 0) {
                        snprintf(m_szProcDir, sizeof(m_szProcDir), "/proc/%i", id);
                        m_iPid = id;
                        break;
                    }
                }
            }
        }
        closedir(directory);
    }
    return (m_iPid != -1);
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
Region& Process::GetRegion(const std::string& name)
{
    for (Region& r : m_regions) {
        char* fileName = basename(r.pathName);
        if (name.compare(fileName) == 0) {
            return r;
        }
    }
    throw std::out_of_range("Process::GetRegion");
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
        m_regions.push_back(Region(m_iPid, pathName, start, end));
    }
    fclose(mapsFile);
    free(line);
    return true;
}

}
