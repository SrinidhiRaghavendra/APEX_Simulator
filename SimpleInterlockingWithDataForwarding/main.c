/*
 *  main.c
 *
 *  Author :
 *  Gaurav Kothari (gkothar1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "cpu.h"

int
main(int argc, char const* argv[])
{
  if (argc != 4) {
    fprintf(stderr, "APEX_Help : Usage %s <input_file> function cycles\n", argv[0]);
    exit(1);
  }
  int no_of_cycles = strtol(argv[3], NULL, 0);
  const char* function = argv[2];
  int simulate = 0;
  if(strcmp(function, "simulate") == 0) {
    simulate = 1;
  }
  APEX_CPU* cpu = APEX_cpu_init(argv[1]);
  if (!cpu) {
    fprintf(stderr, "APEX_Error : Unable to initialize CPU\n");
    exit(1);
  }

  APEX_cpu_run(cpu, no_of_cycles, simulate);
  APEX_cpu_stop(cpu);
  return 0;
}