/*
 * main
 * Initialization code for the Spectre Lab
 * MIT Secure Hardware Design
 * Joseph Ravichandran Spring 2022, Spring 2023
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

#include "labspectre.h"
#include "labspectreipc.h"

/*
 * main
 * Setup shared memory and launch student code.
 */
int main(int argc, char *argv[]) {
    char *shared_memory;

    // Create some shared memory that will be shared by both client and server
    shared_memory = mmap(NULL, SHD_SPECTRE_LAB_SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);

    if (NULL == shared_memory) {
        perror("mmap() error");
        exit(EXIT_FAILURE);
    }

    // Setup memory
    init_shared_memory(shared_memory, SHD_SPECTRE_LAB_SHARED_MEMORY_SIZE);

    // Run the attacker code :)
    return run_attacker(shared_memory);
}
