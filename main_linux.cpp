#include <fcntl.h>      
#include <semaphore.h>  
#include <sys/mman.h>   
#include <sys/stat.h>
#include <unistd.h>     

#include <chrono>
#include <cstdio>
#include <random>
#include <thread>

static const char* SHM_NAME = "/lab6_shm";
static const char* SEM_NAME = "/lab6_sem";
constexpr int LIMIT = 1000;

int main() {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return 1; }
    if (ftruncate(fd, sizeof(int)) == -1) { perror("ftruncate"); return 1; }  

    void* addr = mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) { perror("mmap"); return 1; }
    close(fd);  
    volatile int* counter = static_cast<volatile int*>(addr);

    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) { perror("sem_open"); return 1; }

    std::mt19937 rng(std::random_device{}() ^ static_cast<unsigned>(getpid()));
    std::uniform_int_distribution<int> coin(0, 1);  

    const int pid = getpid();
    while (true) {
        sem_wait(sem);                       

        int value = *counter;                
        if (value >= LIMIT) { sem_post(sem); break; }

        while (coin(rng) == 1 && value < LIMIT) {
            ++value;
            *counter = value;
            std::printf("[pid %d] wrote %d\n", pid, value);
            std::fflush(stdout);
        }

        sem_post(sem);                       
        usleep(1);
    }

    std::printf("[pid %d] done\n", pid);

    munmap(addr, sizeof(int));
    sem_close(sem);
    sem_unlink(SEM_NAME);  
    shm_unlink(SHM_NAME);
    return 0;
}

