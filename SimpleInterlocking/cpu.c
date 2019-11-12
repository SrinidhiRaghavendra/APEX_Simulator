/*
 *  cpu.c
 *  Contains APEX cpu pipeline implementation
 *
 *  Author :
 *  Gaurav Kothari (gkothar1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

/* Set this flag to 1 to enable debug messages */
int ENABLE_DEBUG_MESSAGES = 1;
int flush_and_reload_pc = 0;
int halt_and_flush = 0;

/*
 * This function creates and initializes APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
APEX_CPU*
APEX_cpu_init(const char* filename)
{
  if (!filename) {
    return NULL;
  }

  APEX_CPU* cpu = malloc(sizeof(*cpu));
  if (!cpu) {
    return NULL;
  }

  /* Initialize PC, Registers and all pipeline stages */
  cpu->pc = 4000;
  memset(cpu->regs, 0, sizeof(int) * 32);
  memset(cpu->regs_valid, 1, sizeof(int) * 32);
  memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES);
  memset(cpu->data_memory, 0, sizeof(int) * 4000);

  /* Parse input file and create code memory */
  cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

  if (!cpu->code_memory) {
    free(cpu);
    return NULL;
  }

  if (ENABLE_DEBUG_MESSAGES) {
    fprintf(stderr,
            "APEX_CPU : Initialized APEX CPU, loaded %d instructions\n",
            cpu->code_memory_size);
    fprintf(stderr, "APEX_CPU : Printing Code Memory\n");
    printf("%-9s %-9s %-9s %-9s %-9s %-9s\n", "opcode", "rd", "rs1", "rs2", "rs3", "imm");

    for (int i = 0; i < cpu->code_memory_size; ++i) {
      printf("%-9s %-9d %-9d %-9d %-9d %-9d\n",
             cpu->code_memory[i].opcode,
             cpu->code_memory[i].rd,
             cpu->code_memory[i].rs1,
             cpu->code_memory[i].rs2,
             cpu->code_memory[i].rs3,
             cpu->code_memory[i].imm);
    }
  }

  /* Make all stages busy except Fetch stage by setting their pc value to 0, initally to start the pipeline */
  for (int i = 1; i < NUM_STAGES; ++i) {
    cpu->stage[i].pc = 0;
  }

  return cpu;
}

/*
 * This function de-allocates APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
void
APEX_cpu_stop(APEX_CPU* cpu)
{
  free(cpu->code_memory);
  free(cpu);
}

/* Converts the PC(4000 series) into
 * array index for code memory
 *
 * Note : You are not supposed to edit this function
 *
 */
int
get_code_index(int pc)
{
  return (pc - 4000) / 4;
}

static void
print_instruction(CPU_Stage* stage)
{
  if (strcmp(stage->opcode, "STORE") == 0) {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
  } else if (strcmp(stage->opcode, "MOVC") == 0) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->imm);
  }  else if (strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "SUB") == 0 || strcmp(stage->opcode, "MUL") == 0 || strcmp(stage->opcode, "LDR") == 0 || strcmp(stage->opcode, "AND") == 0 || strcmp(stage->opcode, "OR") == 0 || strcmp(stage->opcode, "EX-OR") == 0) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  } else if (strcmp(stage->opcode, "LOAD") == 0 || strcmp(stage->opcode, "ADDL") == 0 || strcmp(stage->opcode, "SUBL") == 0) {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rd, stage->rs1, stage->imm);
  } else if(strcmp(stage->opcode, "STR") == 0) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rs1, stage->rs2, stage->rs3);
  } else if(strcmp(stage->opcode, "BZ") == 0 || strcmp(stage->opcode, "BNZ") == 0) {
    printf("%s,#%d", stage->opcode, stage->imm);
  } else if(strcmp(stage->opcode, "JUMP") == 0) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rs1, stage->imm);
  } else if(strcmp(stage->opcode, "HALT") == 0){
    printf("%s", stage->opcode);
  }
}

/* Debug function which dumps the cpu stage
 * content
 *
 * Note : You are not supposed to edit this function
 *
 */
static void
print_stage_content(char* name, CPU_Stage* stage, int is_active)
{
  printf("%-15s: ", name);
  if(is_active) {
    printf("pc(%d) ", stage->pc);
    print_instruction(stage);
  } else {
    printf("EMPTY");
  }
  printf("\n");
}

/*
 *  Fetch Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
fetch(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[F];
  stage->is_empty = 0;
  if (!stage->busy && !stage->stalled) {  
    /* Store current PC in fetch latch */
    stage->pc = cpu->pc;

    /* Index into code memory using this pc and copy all instruction fields into
     * fetch latch
     */
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
    strcpy(stage->opcode, current_ins->opcode);
    stage->rd = current_ins->rd;
    stage->rs1 = current_ins->rs1;
    stage->rs2 = current_ins->rs2;
    stage->rs3 = current_ins->rs3;
    stage->imm = current_ins->imm;
    stage->rd = current_ins->rd;

    /* Update PC for next instruction */

    /* Copy data from fetch latch to decode latch*/
    if(!(&cpu->stage[DRF])->stalled) {
      cpu->stage[DRF] = cpu->stage[F];
      current_ins->stage_finished = F;
      cpu->pc += 4;
    } else {
      stage->stalled = 1;
    }
  }
  stage->is_empty = 1;
  if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Fetch", stage, get_code_index(stage->pc) < cpu->code_memory_size);
    }
  return 0;
}

/*
 *  Decode Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
decode(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[DRF];
  stage->stalled = 0;
  stage->is_empty = 0;
  if(stage->pc >= 4000) {
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(stage->pc)];
    if (!stage->busy && !stage->stalled) {
      int is_stage_stalled = 0;
      /* Read data from register file for store */
      if(strcmp(stage->opcode, "STR") == 0) {
        if(cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2] && cpu->regs_valid[stage->rs3]) {
          stage->rs1_value = cpu->regs[stage->rs1];
          stage->rs2_value = cpu->regs[stage->rs2];
          stage->rs3_value = cpu->regs[stage->rs3];
        } else {
          is_stage_stalled = 1;
        }
      } else if (strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "SUB") == 0 ||  strcmp(stage->opcode, "MUL") == 0 ||
   strcmp(stage->opcode, "AND") == 0 || strcmp(stage->opcode, "OR") == 0 || strcmp(stage->opcode, "EX-OR") == 0 || strcmp(stage->opcode, "LDR") == 0 || strcmp(stage->opcode, "STORE") == 0) {
        if(cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2]) {
          stage->rs1_value = cpu->regs[stage->rs1];
          stage->rs2_value = cpu->regs[stage->rs2];
        } else {
          is_stage_stalled = 1;
        }
      } else if (strcmp(stage->opcode, "ADDL") == 0 || strcmp(stage->opcode, "SUBL") == 0 || strcmp(stage->opcode, "LOAD") == 0 || strcmp(stage->opcode, "JUMP") == 0) {
        if(cpu->regs_valid[stage->rs1]) {
          stage->rs1_value = cpu->regs[stage->rs1];
        } else {
          is_stage_stalled = 1;
        }
      } else if (strcmp(stage->opcode, "BZ") == 0 || strcmp(stage->opcode, "BNZ") == 0) {
        CPU_Stage* ex1_stage = &cpu->stage[EX1];
        if(!ex1_stage->is_empty && (strcmp(ex1_stage->opcode, "ADD") == 0 || strcmp(ex1_stage->opcode, "ADDL") == 0 || strcmp(ex1_stage->opcode, "SUB") == 0 || strcmp(ex1_stage->opcode, "SUBL") == 0 || strcmp(ex1_stage->opcode, "MUL") == 0)) {
          is_stage_stalled = 1;
        }
        CPU_Stage* ex2_stage = &cpu->stage[EX2];
        if(!is_stage_stalled && !ex2_stage->is_empty && (strcmp(ex2_stage->opcode, "ADD") == 0 || strcmp(ex2_stage->opcode, "ADDL") == 0 || strcmp(ex2_stage->opcode, "SUB") == 0 || strcmp(ex2_stage->opcode, "SUBL") == 0 || strcmp(ex2_stage->opcode, "MUL") == 0)) {
            is_stage_stalled = 1;
        }
        CPU_Stage* mem1_stage = &cpu->stage[MEM1];
        if(!is_stage_stalled && !mem1_stage->is_empty && (strcmp(mem1_stage->opcode, "ADD") == 0 || strcmp(mem1_stage->opcode, "ADDL") == 0 || strcmp(mem1_stage->opcode, "SUB") == 0 || strcmp(mem1_stage->opcode, "SUBL") == 0 || strcmp(mem1_stage->opcode, "MUL") == 0)) {
            is_stage_stalled = 1;
        }
        CPU_Stage* mem2_stage = &cpu->stage[MEM2];
        if(!is_stage_stalled && !mem2_stage->is_empty && (strcmp(mem2_stage->opcode, "ADD") == 0 || strcmp(mem2_stage->opcode, "ADDL") == 0 || strcmp(mem2_stage->opcode, "SUB") == 0 || strcmp(mem2_stage->opcode, "SUBL") == 0 || strcmp(mem2_stage->opcode, "MUL") == 0)) {
            is_stage_stalled = 1;
        }
        CPU_Stage* wb_stage = &cpu->stage[WB];
        if(!is_stage_stalled && !wb_stage->is_empty && (strcmp(wb_stage->opcode, "ADD") == 0 || strcmp(wb_stage->opcode, "ADDL") == 0 || strcmp(wb_stage->opcode, "SUB") == 0 || strcmp(wb_stage->opcode, "SUBL") == 0 || strcmp(wb_stage->opcode, "MUL") == 0)) {
            is_stage_stalled = 1;
        }

      } else if (strcmp(stage->opcode, "HALT") == 0) {
        halt_and_flush = 1;
        CPU_Stage* ex1_stage = &cpu->stage[EX2];//Since the ex2 stage contents will be moved to mem1 as part ex1 is executed before drf
        if((strcmp(ex1_stage->opcode, "BZ") == 0 || strcmp(ex1_stage->opcode, "BNZ") == 0 || strcmp(ex1_stage->opcode, "JUMP") == 0)) {
          halt_and_flush = 0;
        }
        CPU_Stage* ex2_stage = &cpu->stage[MEM1];//Since the ex2 stage contents will be moved to mem1 as part ex2 is executed before ex1 and drf
        if(halt_and_flush && (strcmp(ex2_stage->opcode, "BZ") == 0 || strcmp(ex2_stage->opcode, "BNZ") == 0 || strcmp(ex2_stage->opcode, "JUMP") == 0)) {
          halt_and_flush = 0;
        }
      }

      /* No Register file read needed for MOVC */
      else if (strcmp(stage->opcode, "MOVC") == 0) {
      }

      /* Copy data from decode latch to execute latch*/
      //Stall here if any of the instructions in the subsequent stages is an arithmetic instruction, if no instruction is present there or no instruction is an arithmetic operation dont stall
      if(is_stage_stalled) {
        stage->stalled = 1;
      } else {
        cpu->stage[EX1] = cpu->stage[DRF];
        current_ins->stage_finished = DRF;
        (&cpu->stage[F])->stalled = 0;
      }
    }
    if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Decode/RF", stage, (current_ins->stage_finished <= DRF && get_code_index(stage->pc) < cpu->code_memory_size));
      }
  } else if (ENABLE_DEBUG_MESSAGES) {
    print_stage_content("Decode/RF", stage, 0);
  }
  stage->is_empty = 1;
  return 0;
}

/*
 *  Execute Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
execute1(APEX_CPU* cpu)
{
  //Set registered valid for the destination to 0 when entering
  CPU_Stage* stage = &cpu->stage[EX1];
  //stage->stalled = 0;
  stage->is_empty = 0;
  if(stage->pc >= 4000) {
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(stage->pc)];
    if (!stage->busy && !stage->stalled && current_ins->stage_finished  < EX1) {
      if(stage->rd && stage->rd < 16 && stage->rd >= 0) {
        cpu->regs_valid[stage->rd] = 0;
      }
      /* Store */
      if (strcmp(stage->opcode, "STORE") == 0) {
        stage->mem_address = stage->rs2_value + stage->imm;
      } else if(strcmp(stage->opcode, "STR") == 0) {
        stage->mem_address = stage->rs2_value + stage->rs3_value;
      } else if(strcmp(stage->opcode, "LOAD") == 0) {
        stage->mem_address = stage->rs1_value + stage->imm;
      } else if(strcmp(stage->opcode, "LDR") == 0) {
        stage->mem_address = stage->rs1_value + stage->rs2_value;
      } else if(strcmp(stage->opcode, "ADD") == 0) {
        stage->buffer = stage->rs1_value + stage->rs2_value;
      } else if(strcmp(stage->opcode, "SUB") == 0) {
        stage->buffer = stage->rs1_value - stage->rs2_value;
      } else if(strcmp(stage->opcode, "ADDL") == 0) {
        stage->buffer = stage->rs1_value + stage->imm;
      } else if(strcmp(stage->opcode, "SUBL") == 0) {
        stage->buffer = stage->rs1_value - stage->imm;
      } else if(strcmp(stage->opcode, "MUL") == 0) {
        stage->buffer = stage->rs1_value * stage->rs2_value;
      } else if(strcmp(stage->opcode, "AND") == 0) {
        stage->buffer = stage->rs1_value & stage->rs2_value;
      } else if(strcmp(stage->opcode, "OR") == 0) {
        stage->buffer = stage->rs1_value | stage->rs2_value;
      } else if(strcmp(stage->opcode, "EX-OR") == 0) {
        stage->buffer = stage->rs1_value ^ stage->rs2_value;
      } else if(strcmp(stage->opcode, "JUMP") == 0) {
        stage->buffer = stage->rs1_value + stage->imm;
        //TBD
      } else if(strcmp(stage->opcode, "HALT") == 0) {
        //No need to do anything here
      }

      /* MOVC */
      if (strcmp(stage->opcode, "MOVC") == 0) {
        stage->buffer = stage->imm + 0;
      }

      /* Copy data from Execute latch to Memory latch*/
      cpu->stage[EX2] = cpu->stage[EX1];
      current_ins->stage_finished = EX1;
    }
    if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Execute 1", stage, (current_ins->stage_finished <= EX1 && get_code_index(stage->pc) < cpu->code_memory_size));
    }
  } else if (ENABLE_DEBUG_MESSAGES) {
    print_stage_content("Execute 1", stage, 0);
  }
  stage->is_empty = 1;
  return 0;
}

int execute2(APEX_CPU* cpu) {
  CPU_Stage* stage = &cpu->stage[EX2];
  stage->is_empty = 0;
  if(stage->pc >= 4000) {
    APEX_Instruction* current_ins = (&cpu->code_memory[get_code_index(stage->pc)]);
    if(!stage->busy && !stage->stalled && current_ins->stage_finished  < EX2) {
      if((strcmp(stage->opcode, "BZ") == 0 && cpu->regs[CC] == 1) || (strcmp(stage->opcode, "BNZ") == 0 && cpu->regs[CC] == 0)) {
        //Flush out the contents of F, DRF and EX1 stages, calculate the new address to jump to using pc-relative addressing
        //new pc value to fetch = old pc value + stage->imm
        flush_and_reload_pc = stage->pc + stage->imm;
      } else if(strcmp(stage->opcode, "JUMP") == 0) {
        //Flush out the contents of F, DRF and EX1 stages, the new address to jump to is already calculated in the previous stage
        //new pc value to fetch = stage->buffer
        flush_and_reload_pc = stage->buffer;
      } else if(strcmp(stage->opcode, "HALT") == 0 && halt_and_flush == 0) {
        halt_and_flush = 2;
      }
      cpu->stage[MEM1] = cpu->stage[EX2];
      current_ins->stage_finished = EX2;
    }
    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Execute 2", stage, (current_ins->stage_finished <= EX2 && get_code_index(stage->pc) < cpu->code_memory_size));
    }
  } else if (ENABLE_DEBUG_MESSAGES) {
    print_stage_content("Execute 2", stage, 0);
  }
  stage->is_empty = 1;
  return 0;
}

/*
 *  Memory Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
memory1(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM1];
  stage->is_empty = 0;
  if(stage->pc >= 4000) {
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(stage->pc)];
    if (!stage->busy && !stage->stalled && current_ins->stage_finished  < MEM1) {

      /* Store */

      /* MOVC */
      /*if (strcmp(stage->opcode, "MOVC") == 0) {
      }*/

      /* Copy data from decode latch to execute latch*/
      cpu->stage[MEM2] = cpu->stage[MEM1];
      current_ins->stage_finished = MEM1;
    }
    if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Memory 1", stage, (current_ins->stage_finished <= MEM1 && get_code_index(stage->pc) < cpu->code_memory_size));
      }
  } else if (ENABLE_DEBUG_MESSAGES) {
    print_stage_content("Memory 1", stage, 0);
  }
  stage->is_empty = 1;
  stage->pc = 0;
  return 0;
}

int memory2(APEX_CPU* cpu) {
  CPU_Stage* stage = &cpu->stage[MEM2];
  stage->is_empty = 0;
  if(stage->pc >= 4000) {
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(stage->pc)];
    if(!stage->busy && !stage->stalled && current_ins->stage_finished  < MEM2) {
      if (strcmp(stage->opcode, "STORE") == 0 || strcmp(stage->opcode, "STR") == 0) {
        cpu->data_memory[stage->mem_address] = stage->rs1_value;
      }  else if(strcmp(stage->opcode, "LOAD") == 0 || strcmp(stage->opcode, "LDR") == 0) {
        stage->buffer = cpu->data_memory[stage->mem_address];
      }
      cpu->stage[WB] = cpu->stage[MEM2];
      current_ins->stage_finished = MEM2;
    }
    if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Memory2", stage, (current_ins->stage_finished <= MEM2 && get_code_index(stage->pc) < cpu->code_memory_size));
      }
  } else if (ENABLE_DEBUG_MESSAGES) {
    print_stage_content("Memory 2", stage, 0);
  }
  stage->is_empty = 1;
  stage->pc = 0;
  return 0;
}
/*
 *  Writeback Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
writeback(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[WB];
  int stage_executed = 0;
  stage->is_empty = 0;
  if(stage->pc >= 4000) {
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(stage->pc)];
    if (!stage->busy && !stage->stalled && current_ins->stage_finished  < WB) {
      if(stage->rd) {
        cpu->regs_valid[stage->rd] = 1;
      }
      /* Update register file */
      if (strcmp(stage->opcode, "MOVC") == 0 || strcmp(stage->opcode, "LOAD") == 0 || strcmp(stage->opcode, "LDR") == 0 || strcmp(stage->opcode, "AND") == 0 || strcmp(stage->opcode, "OR") == 0 || strcmp(stage->opcode, "EX-OR") == 0) {
        cpu->regs[stage->rd] = stage->buffer;
      } else if(strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "ADDL") == 0 || strcmp(stage->opcode, "SUB") == 0 || strcmp(stage->opcode, "SUBL") == 0 || strcmp(stage->opcode, "MUL") == 0) {
        cpu->regs[stage->rd] = stage->buffer;
        cpu->regs[CC] = (stage->buffer == 0);
      }
      else if(strcmp(stage->opcode, "HALT") == 0 && ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Writeback", stage, 1);
        return 1;
      }
      current_ins->stage_finished = WB;
      stage_executed = 1;
      cpu->ins_completed++;
    }
    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Writeback", stage, (stage_executed && get_code_index(stage->pc) < cpu->code_memory_size));
    }
    if(get_code_index(stage->pc) == cpu->code_memory_size) {
      return 1;
    }
  } else if (ENABLE_DEBUG_MESSAGES) {
    print_stage_content("Writeback", stage, 0);
  }
  stage->is_empty = 1;
  stage->pc = 0;
  return 0;
}

int print_register_state(APEX_CPU* cpu) {
  printf("=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\n");
  int index;
  int no_registers = (int) (sizeof(cpu->regs)/sizeof(cpu->regs[0]));
  for(index = 0; index < no_registers - 1; ++index) {//Assummin CC register is also part of the register file
    printf("| \t REG[%d] \t | \t Value=%d \t | \t STATUS=%s \t |\n", index, cpu->regs[index], (cpu->regs_valid[index] ? "VALID" : "INVALID"));
  }
  return 0;
}

int print_data_memory(APEX_CPU* cpu) {
  printf("============== STATE OF DATA MEMORY =============\n");
  int index;
  for(index = 0; index < 100; ++index) {
    printf("| \t MEM[%d] \t | \t Data Value=%d \t |\n", index, cpu->data_memory[index]);
  }
  return 0;
}
/*
 *  APEX CPU simulation loop
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
APEX_cpu_run(APEX_CPU* cpu, int no_of_cycles, int flag)
{
  ENABLE_DEBUG_MESSAGES = flag;
  while (cpu->clock <= no_of_cycles) {

    /* All the instructions committed, so exit */
    /*if (get_code_index(cpu->pc) > cpu->code_memory_size) {
      printf("(apex) >> Simulation Complete\n");
      break;
    }*/

    if (ENABLE_DEBUG_MESSAGES) {
      printf("--------------------------------\n");
      printf("Clock Cycle #: %d\n", cpu->clock);
      printf("--------------------------------\n");
    }

    int writeback_result = writeback(cpu);
    memory2(cpu);
    memory1(cpu);
    execute2(cpu);
    execute1(cpu);
    decode(cpu);
    fetch(cpu);
    if(flush_and_reload_pc) {
      cpu->pc = flush_and_reload_pc;
      (&cpu->stage[EX2])->pc = (&cpu->stage[EX1])->pc = (&cpu->stage[DRF])->pc = (&cpu->stage[F])->pc = 0;
      flush_and_reload_pc = 0;
    }
    if(halt_and_flush) {
      cpu->pc = cpu->code_memory_size * 4 + 4000;
      (&cpu->stage[DRF])->pc = (&cpu->stage[F])->pc = 0;
      if(halt_and_flush > 1) {
        (&cpu->stage[EX2])->pc = (&cpu->stage[EX1])->pc = 0;
      }
    }
    if(writeback_result) {
      break;
    }
    cpu->clock++;
  }
  printf("(apex) >> Simulation Complete\n");
  print_register_state(cpu);
  print_data_memory(cpu);
  return 0;
}