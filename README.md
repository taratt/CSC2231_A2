# CSC2231. Lab-2 Spectre Attacks

In this assignment, you will learn to exploit a Spectre vulnerability. 

We have created a shared library that has a spectre vulnerability embedded in it. You will interact with this shared library API, provide data inputs to the API that activate the vulnerability and leak datathat you do not have access to via a side-channel.

## Introduction
In this lab we will be putting the Spectre attack into practice. You will complete CTF style problems of increasing difficulty to leak a secret. In Part 1, we will begin by implementing a (non-speculative) Flush+Reload attack against a memory region shared with the API. In Part 2, you will implement a standard Spectre attack where you will manipulate the speculative execution behavior of the victim program.

### Infrastructure
In this lab, we have created two data arrays, one called `shared_mem` residing in the userspace, and the other called `secret_partX` residing in the shared library (that you do not have access to). The `secret_partX` array contains a secret string that you aim to leak. The shared library API is designed to take a user input (usually an integer), perform some simple computation on it, and then perform a read access on the `shared_mem` array. In this lab you will leverage cache side channels to monitor its accesses to the shared_mem array to leak the secret.

### The Secret
Each secret is a string of the form `UofT{some_secret_value}`. The string can be up to 64 bytes in length including the NULL terminator (`0x00`). You can consider the secret complete once you leak the NULLterminator.

Do not make any assumption about the secret other than it is a NULL terminated string of length up to 64 bytes (including the NULL terminator). The secrets will not change from run to run (they are constant for the lifetime of the shared library). During grading, we may use different secret values to evaluate your implementation.


### Code Skeleton
* `inc/labspectre.h.h` and `src-common/spectre_lab_helper.c` provide a set of utility functions for you to use.
* `src-common/main.c` is used in both parts of this assignment. The main function sets up a shared memory region (`shared_memory`) of size LAB_SHARED_MEMORY_SIZE bytes. The `shared_memory` region is shared between the process and the shared library.
* `part1-src/attacker-part1.c` is the file you will modify in Part 1 to implement your Flush+Reload attack. The method `call_kernel_part1` can be used for calling into the shared library.
* `part2-src/attacker-part2.c` is the file you will modify in Part 2 to implement your Spectre attack. The method `call_kernel_part2` can be used to call into the shared library.

You can check out the code of the `lib/shared_lib.o` in `module_src/shared_lib.c` if it interests you. The secrets in the c code are different from the object file.

### Compile and Run
After you hand in your code, we will embed different secret strings in the shared library and rerun your code to see whether it effectively leaks these strings. Instructions for compiling the code and running the code are below:

* **Compilation:** From the base directory, use `make all` to compile the project. The binaries `part1` and `part2` will be produced in the same directory.
* **Run:** You can run each part by calling `./part1` or `./part2`. The results of your code will be printed to the console – on success you should see the secret leaked from shared library printed to the console (a string of the format: `UofT{some_secret_value}`).

An example of the expected output is below:

```$ ./part1
UofT{part1_secret_value}
```

## Part-1: Leak Shared Library Secrets via Flush+Reload (50%)

In this part you will set up a cache-based side channel to leak information from a shared library using Flush+Reload.

To implement a Flush+Reload attack, you will need timing information to distinguish between cache access latency and DRAM latency. You can use the threshold you discovered in the First Lab Assignment for this.

### Attack Overview
The pseudocode for the victim code of Part 1 in the shared-library is shown below.

```
def victim_part1(shared_mem, offset):
    secret_data = secret_part1[offset]
    load shared_mem[4096 * secret_data]
```

The victim function takes a pointer `shared_mem` (pointing to the starting of a shared memory region) and an integer `offset` as input. First, the code loads a secret byte from a secret array named `part1_secret`. The byte to leak is chosen by the attacker-controlled `offset`. When the `offset` is `0`, the first secret byte will be loaded; when `offset` is `1`, the second byte will be loaded, and so on. Next, the victim mutiplies the secret byte with 4096 and uses the result as an index into the shared memory array. For example, if the secret data was the character ‘A’ (ascii code 0x41), then the 0x41’th page in the shared memory region will be loaded into the cache.

Recall that the secret is a string up to 64 characters long (including the NULL terminator). The attacker can leak the secret one byte at a time using Flush+Reload as follows:
* Flush the shared memory region from the cache using clflush
* Call the victim method using the shared memory pointer and the desired offset in the secret string to leak the associated secret byte.
* Reload the shared memory region, measure the latency of accessing each address, and use the latency to derive the value of the secret. When the value is `0x00` (i.e. NULL), the attack is complete.

### Specific Tasks

You need to implement the following in `run_attacker()` in `part1-src/attacker-part1.c`:
* Implement the Flush+Reload attack to leak the secret string. 
* Build your attack step-by-step: start by leaking the first character. Then try to leak the whole string byte by byte.
* Test the code with `make all ; ./part1`. 

Please do not modify the provided `call_kernel_part1` method. You need to this to interact with the API. This function takes two arguments: a pointer to the shared memory region, and the desired offset. `shared_memory` can be directly passed from the `run_attacker()` to this method without modification. The offset to use for a given invocation is up to you.

You can define your own helper methods as you desire. You can use any method in `inc/labspectre.h` as well as the provided methods in `attacker-part1.c`.

### Submission and Grading

Full credit will be awarded to solutions that report the correct secret more than 95% of the time, while partial credit will be awarded for solutions which perform worse than that. Each attempt should take no longer than 30 seconds.

Please submit a single tar file for both parts, containing the binaries and source code: `tar -zvcf lab2.tar.gz uoft-csc2231-lab2`.

## Part 2: Spectre Attack (50%)

Now that Flush+Reload is working, let’s move on to actually implementing a Spectre attack! Below is the pseudocode for Part 2’s victim code:
```
part2_limit = 4
def victim_part2 (shared_mem, offset):
    secret_data = secret_part2[offset]
    mem_index = 4096 * secret_data

    # to delay the subsequent branch
    flush(part2_limit)

    if offset < part2_limit:
        load shared_mem[mem_index]    
```
This victim is quite similar to Part 1, except it will only perform the load if the offset is within a specific range (e.g., offset<4).

### Attack Outline
Below are the steps required to leak a single byte. You may need to alter your approach to account for system noise.

* Train the branch predictor to speculatively perform the load operation (i.e. take the branch).
* Flush the shared memory region from the cache using clflush.
* Call the victim function with an offset beyond the limit, leaking the secret byte during speculative execution.
* Reload the memory region, measure the latency of accessing each address, and use the latency to determine the value of the secret.

Try to copy over the attack code from Part-1 for this attack, and see if it works. Think: What part of the secret_part2 can you leak? What part can you not?

As you’ve observed in previous labs, side channel attacks generally do not work on the first attempt. Here’s some hints that may help you if you get stuck.
* *Noise*: It is common to observe noise when performing any sorts of side channel attacks. Calling the victim method just once may not produce the most accurate results. You may benefit from repeating the attack multiple times, using statistical methods to decode the secret with a higher precision.
* *Branch Predictor*: Modern processors employ branch predictors with significant complexity. Branch predictors can use global prediction histories, which allow different branches to interfere each other. If the speculation is not working as expected, you may need to reduce number of branches in your attack code.
* *Mixing Measurements and Output:* Due to the sensitivity of measurements, it is a bad idea to print latencies while measuring cache-based side channels. Instead, it is better to perform all measurements in one loop, store the results in an array, then print the results in a second loop. This prevents your reporting operations from negatively affecting the branch history/cache state.

### Tasks
* Implement the Spectre attack in `attacker-part2.c` to leak the secret string.
* Similar to the previous part, first try to leak one byte beyond the part2_limit (e.g. 5th byte of the part2_secret). Once that works, leak the entire secret byte by byte until you hit a NULL (0x00).
* Build the project with `make all` and run `./part2`. Once you are confident it works, run `./check.py 2` from the main directory to check whether your code passes.

### Submission and Grading
This part is graded in the same way as Part 1. Full credit will be awarded to solutions that report the correct secret more than 95% of the time, while partial credit will be awarded for solutions which perform worse than that. Each attempt should take no longer than 30 seconds.

Please submit a single tar file for both parts, containing the binaries and source code: `tar -zvcf lab2.tar.gz uoft-csc2231-lab2`.

Please also provide screenshots of your results from Part-1 and Part-2 (when you run `./part1` and `./part2`) in a PDF within the tar submission.
