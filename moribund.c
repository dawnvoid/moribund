#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define OP_NOP 0x00u
#define OP_HLT 0xFFu

#define OP_1SA 0x1Au
#define OP_1SB 0x1Bu
#define OP_1SC 0x1Cu
#define OP_1SD 0x1Du

#define OP_2SA 0x2Au
#define OP_2SB 0x2Bu
#define OP_2SC 0x2Cu
#define OP_2SD 0x2Du

#define OP_SS1 0x01u
#define OP_SS2 0x02u
#define OP_SS4 0x04u

#define OP_POP 0x50u
#define OP_PSH 0x51u
#define OP_DUP 0x52u

#define OP_SET 0x5Eu

#define OP_JMP 0x99u
#define OP_JCM 0x9Cu
#define OP_JEQ 0x9Eu
#define OP_JZR 0x90u

#define OP_ADD 0x80u
#define OP_SUB 0x81u
#define OP_MUL 0x82u
#define OP_DIV 0x83u
#define OP_MOD 0x84u

#define OP_NOT 0x40u
#define OP_AND 0x41u
#define OP_ORR 0x42u
#define OP_XOR 0x43u
#define OP_LSH 0x44u
#define OP_RSH 0x45u

typedef struct ByteStack {
    unsigned char data[1024];
    int_least16_t size;
} ByteStack;

void byte_stack_reset(ByteStack *stack) {
    int i;
    for (i = 0; i < 1024; i++) {
        stack->data[i] = 0x00u;
    }
    stack->size = 0;
}

void byte_stack_push(ByteStack *stack, unsigned char byte) {
    if (stack->size + 1 > 1024) {
        printf("CRASH! (stack overflow)\n");
        exit(-1);
    }
    stack->data[stack->size] = byte;
    stack->size++;
}

unsigned char byte_stack_pop(ByteStack *stack) {
    if (stack->size - 1 < 0) {
        printf("CRASH! (popped on empty stack)\n");
        exit(-1);
    }
    unsigned char result = stack->data[stack->size - 1];
    stack->size--;
    return result;
}

void byte_stack_print(ByteStack *stack, int max) {
    int i;
    for (i = 0; i < max && i < 1024 && i < stack->size; i++) {
        printf("%02x ", stack->data[i]);
    }
}

typedef struct MoribundCPU {
    
    /* registers */
    uint_least16_t pc; /* program counter */
    int ps; /* primary stack selection */
    int ss; /* secondary stack selection */
    int os; /* operating size */
    uint_least32_t tmp; /* temporary storage */
    uint_least16_t tar; /* jump target address */
    int sc; /* set counter (how many bytes left to read) */
    
    /* ram */
    unsigned char program[1024]; /* program bytecode */
    ByteStack stack_a; /* general purpose stack A */
    ByteStack stack_b; /* general purpose stack B */
    ByteStack stack_c; /* general purpose stack C */
    ByteStack stack_d; /* general purpose stack D */
    
} MoribundCPU;

void moribund_cpu_reset(MoribundCPU *cpu) {
    
    /* clear registers */
    cpu->pc = 0;
    cpu->ps = 0;
    cpu->ss = 0;
    cpu->os = 1;
    cpu->tmp = 0;
    cpu->tar = 0;
    cpu->sc = 0;
    
    /* clear ram */
    int i;
    for (i = 0; i < 1024; i++) {
        cpu->program[i] = 0x00u;
    }
    byte_stack_reset(&cpu->stack_a);
    byte_stack_reset(&cpu->stack_b);
    byte_stack_reset(&cpu->stack_c);
    byte_stack_reset(&cpu->stack_d);
}

int moribund_cpu_load(MoribundCPU *cpu, FILE *file) {
    return fread(cpu->program, 1024, 1, file);
}

ByteStack *moribund_cpu_get_stack(MoribundCPU *cpu, int selection) {
    if (selection == 0) {
        return &cpu->stack_a;
    } else if (selection == 1) {
        return &cpu->stack_b;
    } else if (selection == 2) {
        return &cpu->stack_c;
    } else if (selection == 3) {
        return &cpu->stack_d;
    } else {
        printf("CRASH! (bad stack selection)\n");
        exit(-1);
    }
}

void moribund_cpu_op_psh(MoribundCPU *cpu) {
    ByteStack *stack = moribund_cpu_get_stack(cpu, cpu->ps);
    if (cpu->os == 1) {
        unsigned char b0 = (unsigned char)((cpu->tmp << 24) >> 24);
        byte_stack_push(stack, b0);
    } else if (cpu->os == 2) {
        unsigned char b0 = (unsigned char)((cpu->tmp << 24) >> 24);
        unsigned char b1 = (unsigned char)((cpu->tmp << 16) >> 24);
        byte_stack_push(stack, b0);
        byte_stack_push(stack, b1);
    } else if (cpu->os == 4) {
        unsigned char b0 = (unsigned char)((cpu->tmp << 24) >> 24);
        unsigned char b1 = (unsigned char)((cpu->tmp << 16) >> 24);
        unsigned char b2 = (unsigned char)((cpu->tmp << 8) >> 24);
        unsigned char b3 = (unsigned char)(cpu->tmp >> 24);
        byte_stack_push(stack, b0);
        byte_stack_push(stack, b1);
        byte_stack_push(stack, b2);
        byte_stack_push(stack, b3);
    } else {
        printf("CRASH! (corrupt operating size)\n");
        exit(-1);
    }
}

void moribund_cpu_op_pop(MoribundCPU *cpu) {
    ByteStack *stack = moribund_cpu_get_stack(cpu, cpu->ps);
    if (cpu->os == 1) {
        unsigned char b0 = byte_stack_pop(stack);
        cpu->tmp = (uint32_t) b0;
    } else if (cpu->os == 2) {
        uint_least16_t bits_1 = (uint_least16_t) byte_stack_pop(stack);
        uint_least16_t bits_0 = (uint_least16_t) byte_stack_pop(stack);
        cpu->tmp = (bits_1 << 8) + bits_0;
    } else if (cpu->os == 4) {
        uint_least16_t bits_3 = (uint_least16_t) byte_stack_pop(stack);
        uint_least16_t bits_2 = (uint_least16_t) byte_stack_pop(stack);
        uint_least16_t bits_1 = (uint_least16_t) byte_stack_pop(stack);
        uint_least16_t bits_0 = (uint_least16_t) byte_stack_pop(stack);
        cpu->tmp = (bits_3 << 24) + (bits_3 << 16) + (bits_3 << 8) + bits_0;
    } else {
        printf("CRASH! (corrupt operating size)\n");
        exit(-1);
    }
}

void moribund_cpu_op_dup(MoribundCPU *cpu) {
    ByteStack *stack = moribund_cpu_get_stack(cpu, cpu->ps);
    if (cpu->os == 1) {
        unsigned char b0 = byte_stack_pop(stack);
        byte_stack_push(stack, b0);
        byte_stack_push(stack, b0);
    } else if (cpu->os == 2) {
        unsigned char b1 = byte_stack_pop(stack);
        unsigned char b0 = byte_stack_pop(stack);
        byte_stack_push(stack, b0);
        byte_stack_push(stack, b1);
        byte_stack_push(stack, b0);
        byte_stack_push(stack, b1);
    } else if (cpu->os == 4) {
        unsigned char b3 = byte_stack_pop(stack);
        unsigned char b2 = byte_stack_pop(stack);
        unsigned char b1 = byte_stack_pop(stack);
        unsigned char b0 = byte_stack_pop(stack);
        byte_stack_push(stack, b0);
        byte_stack_push(stack, b1);
        byte_stack_push(stack, b2);
        byte_stack_push(stack, b3);
        byte_stack_push(stack, b0);
        byte_stack_push(stack, b1);
        byte_stack_push(stack, b2);
        byte_stack_push(stack, b3);
    } else {
        printf("CRASH! (corrupt operating size)\n");
        exit(-1);
    }
}

void moribund_cpu_op_jcm(MoribundCPU *cpu) {
    ByteStack *primary = moribund_cpu_get_stack(cpu, cpu->ps);
    ByteStack *secondary = moribund_cpu_get_stack(cpu, cpu->ss);
    if (cpu->os == 1) {
        unsigned char pb0 = byte_stack_pop(primary);
        byte_stack_push(primary, pb0);
        unsigned char sb0 = byte_stack_pop(secondary);
        byte_stack_push(secondary, sb0);
        if (pb0 < sb0) {
            cpu->pc = cpu->tar;
        }
    } else if (cpu->os == 2) {
        unsigned char pb1 = byte_stack_pop(primary);
        unsigned char pb0 = byte_stack_pop(primary);
        byte_stack_push(primary, pb0);
        byte_stack_push(primary, pb1);
        unsigned char sb1 = byte_stack_pop(secondary);
        unsigned char sb0 = byte_stack_pop(secondary);
        byte_stack_push(secondary, sb0);
        byte_stack_push(secondary, sb1);
        
        uint_least16_t primary_value = ((uint_least16_t)(pb1) << 8) + (uint_least16_t)pb0;
        uint_least16_t secondary_value = ((uint_least16_t)(sb1) << 8) + (uint_least16_t)sb0;
        if (primary_value < secondary_value) {
            cpu->pc = cpu->tar;
        } 
    } else if (cpu->os == 4) {
        unsigned char pb3 = byte_stack_pop(primary);
        unsigned char pb2 = byte_stack_pop(primary);
        unsigned char pb1 = byte_stack_pop(primary);
        unsigned char pb0 = byte_stack_pop(primary);
        byte_stack_push(primary, pb0);
        byte_stack_push(primary, pb1);
        byte_stack_push(primary, pb2);
        byte_stack_push(primary, pb3);
        unsigned char sb3 = byte_stack_pop(secondary);
        unsigned char sb2 = byte_stack_pop(secondary);
        unsigned char sb1 = byte_stack_pop(secondary);
        unsigned char sb0 = byte_stack_pop(secondary);
        byte_stack_push(secondary, sb0);
        byte_stack_push(secondary, sb1);
        byte_stack_push(secondary, sb2);
        byte_stack_push(secondary, sb3);
        
        uint_least32_t primary_value = ((uint_least32_t)(pb3) << 24);
        primary_value += ((uint_least32_t)(pb2) << 16);
        primary_value += ((uint_least32_t)(pb1) << 8);
        primary_value += (uint_least32_t)pb0;
        uint_least32_t secondary_value = ((uint_least32_t)(sb3) << 24);
        secondary_value += ((uint_least32_t)(sb2) << 16);
        secondary_value += ((uint_least32_t)(sb1) << 8);
        secondary_value += (uint_least32_t)sb0;
        if (primary_value < secondary_value) {
            cpu->pc = cpu->tar;
        } 
    } else {
        printf("CRASH! (corrupt operating size)\n");
        exit(-1);
    }
}

void moribund_cpu_op_jeq(MoribundCPU *cpu) {
    ByteStack *primary = moribund_cpu_get_stack(cpu, cpu->ps);
    ByteStack *secondary = moribund_cpu_get_stack(cpu, cpu->ss);
    if (cpu->os == 1) {
        unsigned char pb0 = byte_stack_pop(primary);
        byte_stack_push(primary, pb0);
        unsigned char sb0 = byte_stack_pop(secondary);
        byte_stack_push(secondary, sb0);
        if (pb0 == sb0) {
            cpu->pc = cpu->tar;
        }
    } else if (cpu->os == 2) {
        unsigned char pb1 = byte_stack_pop(primary);
        unsigned char pb0 = byte_stack_pop(primary);
        byte_stack_push(primary, pb0);
        byte_stack_push(primary, pb1);
        unsigned char sb1 = byte_stack_pop(secondary);
        unsigned char sb0 = byte_stack_pop(secondary);
        byte_stack_push(secondary, sb0);
        byte_stack_push(secondary, sb1);
        if (pb1 == sb1 && pb0 == sb0) {
            cpu->pc = cpu->tar;
        }
    } else if (cpu->os == 4) {
        unsigned char pb3 = byte_stack_pop(primary);
        unsigned char pb2 = byte_stack_pop(primary);
        unsigned char pb1 = byte_stack_pop(primary);
        unsigned char pb0 = byte_stack_pop(primary);
        byte_stack_push(primary, pb0);
        byte_stack_push(primary, pb1);
        byte_stack_push(primary, pb2);
        byte_stack_push(primary, pb3);
        unsigned char sb3 = byte_stack_pop(secondary);
        unsigned char sb2 = byte_stack_pop(secondary);
        unsigned char sb1 = byte_stack_pop(secondary);
        unsigned char sb0 = byte_stack_pop(secondary);
        byte_stack_push(secondary, sb0);
        byte_stack_push(secondary, sb1);
        byte_stack_push(secondary, sb2);
        byte_stack_push(secondary, sb3);
        if (pb3 == sb3 && pb2 == sb2 && pb1 == sb1 && pb0 == sb0) {
            cpu->pc = cpu->tar;
        } 
    } else {
        printf("CRASH! (corrupt operating size)\n");
        exit(-1);
    }
}

int moribund_cpu_process(MoribundCPU *cpu) {
    unsigned char instruction = cpu->program[cpu->pc];
    
    if (cpu->sc > 0) {
        printf("%02x\n", instruction);
        cpu->sc--;
        ByteStack *stack = moribund_cpu_get_stack(cpu, cpu->ps);
        byte_stack_push(stack, instruction); /* binary literal */
    }
    
    else if (instruction == OP_HLT) {
        return 0;
    }
    
    else if (instruction == OP_1SA) {
        printf("1sa\n");
        cpu->ps = 0;
    } else if (instruction == OP_1SB) {
        printf("1sb\n");
        cpu->ps = 1;
    } else if (instruction == OP_1SC) {
        printf("1sc\n");
        cpu->ps = 2;
    } else if (instruction == OP_1SD) {
        printf("1sd\n");
        cpu->ps = 3;
    }
    
    else if (instruction == OP_2SA) {
        printf("2sa\n");
        cpu->ss = 0;
    } else if (instruction == OP_2SB) {
        printf("2sb\n");
        cpu->ss = 1;
    } else if (instruction == OP_2SC) {
        printf("2sc\n");
        cpu->ss = 2;
    } else if (instruction == OP_2SD) {
        printf("2sd\n");
        cpu->ss = 3;
    }
    
    else if (instruction == OP_SS1) {
        cpu->os = 1;
    } else if (instruction == OP_SS2) {
        cpu->os = 2;
    } else if (instruction == OP_SS4) {
        cpu->os = 4;
    }
    
    else if (instruction == OP_POP) {
        printf("pop\n");
        moribund_cpu_op_pop(cpu);
    } else if (instruction == OP_PSH) {
        printf("psh\n");
        moribund_cpu_op_psh(cpu);
    } else if (instruction == OP_DUP) {
        printf("dup\n");
        moribund_cpu_op_dup(cpu);
    }
    
    else if (instruction == OP_SET) {
        printf("set\n");
        cpu->sc = cpu->os;
    }
    
    else if (instruction == OP_JMP) {
        printf("jmp\n");
        cpu->pc = cpu->tar;
    } else if (instruction == OP_JCM) {
        printf("jcm\n");
        moribund_cpu_op_jcm(cpu);
    } else if (instruction == OP_JEQ) {
        printf("jeq\n");
        moribund_cpu_op_jeq(cpu);
    }
    
    cpu->pc++;
    if (cpu->pc >= 1024) {
        printf("CRASH! (program counter out of bounds)\n");
        exit(-1);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: moribund path/to/binary\n");
    }
    
    char *filepath = argv[1];
    FILE *file = fopen(filepath, "rb"); /* read as binary */
    
    if (file == NULL) {
        return -1;
    }
    
    MoribundCPU cpu;
    moribund_cpu_reset(&cpu);
    moribund_cpu_load(&cpu, file);
    
    int i = 0;
    while (i < 1024) {
        int w;
        for (w = 0; w < 16 && i < 1024; w++) {
            printf("%02x ", cpu.program[i]);
            i++;
        }
        printf("\n");
    }
    
    int status = 1;
    while (status) {
        status = moribund_cpu_process(&cpu);
    }
    
    printf("A: ");
    byte_stack_print(&cpu.stack_a, 16);
    printf("\n");
    printf("B: ");
    byte_stack_print(&cpu.stack_b, 16);
    printf("\n");
    printf("C: ");
    byte_stack_print(&cpu.stack_c, 16);
    printf("\n");
    printf("D: ");
    byte_stack_print(&cpu.stack_d, 16);
    printf("\n");
    
    fclose(file);
    
    return 0;
}

