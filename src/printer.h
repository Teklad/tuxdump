#ifndef  __TUXDUMP_PRINTER_H__
#define  __TUXDUMP_PRINTER_H__
#include <cstdlib>

class Printer {
    public:
        static void PrintFileHeader(const char* name);
        static void PrintFileFooter(const char* name);
        static void PrintOffset(const char* name, size_t offset, int depth = 1);
        static void PrintSignaturesStart();
        static void PrintSignaturesEnd();
        static void PrintNetvarsStart();
        static void PrintNetvarsEnd();
        static void PrintNetvarTableStart(const char* name, int depth);
        static void PrintNetvarTableEnd(const char* name, int depth);
        static void PrintTimestamp();
    private:
        static void PrintDepth(int depth);
};


#endif //__TUXDUMP_PRINTER_H__
