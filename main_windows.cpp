// Lab 6 - Windows (Win32): two processes count 1..1000 through a memory-mapped
#include <windows.h>

#include <chrono>
#include <cstdio>
#include <random>
#include <thread>

static const char* SHM_NAME = "Local\\lab6_shm";
static const char* SEM_NAME = "Local\\lab6_sem";
constexpr int LIMIT = 1000;

int main() {
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                     0, sizeof(int), SHM_NAME);
    if (!hMap) { std::fprintf(stderr, "CreateFileMapping failed: %lu\n", GetLastError()); return 1; }

    void* addr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (!addr) { std::fprintf(stderr, "MapViewOfFile failed: %lu\n", GetLastError()); return 1; }
    volatile int* counter = static_cast<volatile int*>(addr);

    HANDLE hSem = CreateSemaphoreA(nullptr, 1, 1, SEM_NAME);
    if (!hSem) { std::fprintf(stderr, "CreateSemaphore failed: %lu\n", GetLastError()); return 1; }

    std::mt19937 rng(std::random_device{}() ^ static_cast<unsigned>(GetCurrentProcessId()));
    std::uniform_int_distribution<int> coin(0, 1);  

    const DWORD pid = GetCurrentProcessId();
    while (true) {
        WaitForSingleObject(hSem, INFINITE);     

        int value = *counter;                    
        if (value >= LIMIT) { ReleaseSemaphore(hSem, 1, nullptr); break; }

        while (coin(rng) == 1 && value < LIMIT) {
            ++value;
            *counter = value;
            std::printf("[pid %lu] wrote %d\n", pid, value);
            std::fflush(stdout);
        }

        ReleaseSemaphore(hSem, 1, nullptr);   

        Sleep(1);
    }

    std::printf("[pid %lu] done\n", pid);

    UnmapViewOfFile(addr);
    CloseHandle(hMap);
    CloseHandle(hSem);
    return 0;
}
