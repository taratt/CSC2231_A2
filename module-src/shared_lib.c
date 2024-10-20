#include "labspectre.h"
#include "labspectreipc.h"


// The secrets you're trying to leak!
static volatile char __attribute__((aligned(32768))) secret_part2[SHD_SPECTRE_LAB_SECRET_MAX_LEN] = "UofT{part2_secret}";
static volatile char __attribute__((aligned(32768))) secret_part1[SHD_SPECTRE_LAB_SECRET_MAX_LEN] = "UofT{part1_secret}";

// The bounds checks that you need to use speculative execution to bypass
static volatile size_t __attribute__((aligned(32768))) secret_leak_limit_part2 = 4;

/*
 * lab2_sharedlib_victim
 *
 * Input: A lab2_command struct for us to parse.
 * Output: Some int.
 * Side Effects: Will trigger a spectre bug based on the user_cmd.kind
 */

int lab2_sharedlib_victim(spectre_lab_command user_cmd) {


  // arg1 is the shared_array
  char* shared_array = (char*) user_cmd.arg1 ; 

  // arg2 is always the secret index to use
  if (!(user_cmd.arg2 < SHD_SPECTRE_LAB_SECRET_MAX_LEN)) {
    printf("[lab2-sharedlib:] ERROR: Tried to access a secret that is too large! Requested offset %lu\n", user_cmd.arg2);
    exit(1);
  }

  // output
  char tmp = '0';
  char secret_data = '0';
  
  // Process this command packet
  switch (user_cmd.kind) {

  // Part 1 is Flush+Reload, so access a secret without a bounds check
  case COMMAND_PART1:
 
    secret_data = secret_part1[user_cmd.arg2]; 
    if (secret_data < SHD_SPECTRE_LAB_SHARED_MEMORY_NUM_PAGES) { 
      tmp = shared_array[SHD_SPECTRE_LAB_PAGE_SIZE * secret_data]; 
    }
    break;

        
    // Part 2 is Spectre, so access a secret bounded by a bounds check
  case COMMAND_PART2:
    // Load the secret:
    secret_data = secret_part2[user_cmd.arg2];
    
    // Flush the limit variable to make this if statement take a long time to resolve
    clflush((void*)&secret_leak_limit_part2);
    if (user_cmd.arg2 < secret_leak_limit_part2) {
      // Perform the speculative leak
      tmp = shared_array[SHD_SPECTRE_LAB_PAGE_SIZE * secret_data]; 
    }
    break;

  }

  tmp = tmp ^ tmp;
  return (int) tmp;
}
