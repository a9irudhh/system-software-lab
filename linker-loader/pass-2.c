#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_BUF 256
#define ESTAB_SIZE 64
#define MEMORY_SIZE 65536
#define ERROR_VALUE -1

int memory_array[MEMORY_SIZE];
int memory_array_wom[MEMORY_SIZE];

typedef struct
{
    char symbol[MAX_BUF];
    int address;
} ESTAB;

int estab_length;
ESTAB estab[ESTAB_SIZE];

void show_estab();
void init_estab();
int ll_pass_two(FILE *object_programs, int PROGADDR);
void save_to_mem_array(FILE *object_programs, int start_address, int tr_length);
void modify_memory(int location, int half_bytes, char operator_symbol, int symbol_value);

void print_without_modifications(int PROGADDR, int total_length);
void print_with_modifications(int PROGADDR, int total_length);
void print_both(int PROGADDR, int total_length);

int search_symbol(char *symbol);

int main()
{
    init_estab();

    FILE *object_programs = fopen("object_programs.txt", "r");
    FILE *without_modifications = fopen("without_modifications.txt", "w");
    FILE *with_modifications = fopen("with_modifications.txt", "w");
    FILE *load_address = fopen("load_address.txt", "r");

    int PROGADDR;
    fscanf(load_address, "%x", &PROGADDR);

    ll_pass_two(object_programs, PROGADDR);

    fclose(object_programs);
    fclose(without_modifications);
    fclose(with_modifications);

    printf("Success!\n");
    return 0;
}

int ll_pass_two(FILE *object_programs, int PROGADDR)
{
    int CSADDR = PROGADDR;

    typedef struct
    {
        char record_type;
        char name[MAX_BUF];
        int start_address;
        int length;
    } header_record;

    typedef struct
    {
        int start_address;
        int length;
        int obj_program;
    } text_record;

    typedef struct
    {
        int start_address;
        int half_bytes;
        char operator_symbol;
        char symbol[MAX_BUF];
    } modification_record;

    header_record hr;
    text_record tr;
    modification_record mr;
    char current_record_type;
    int CSLTH;
    int total_length = 0;
    char input_record[MAX_BUF];

    while (fscanf(object_programs, "%c%6s%6x%6x\n", &hr.record_type, hr.name, &hr.start_address, &hr.length) > 1)
    {
        current_record_type = hr.record_type;
        CSLTH = hr.length;
        if (current_record_type == '.')
            continue;

        while (current_record_type != 'E')
        {
            fscanf(object_programs, "%c", &current_record_type);

            if (current_record_type == '.')
                fgets(input_record, MAX_BUF, object_programs);
            if (current_record_type == 'D')
                fgets(input_record, MAX_BUF, object_programs);
            if (current_record_type == 'R')
                fgets(input_record, MAX_BUF, object_programs);

            if (current_record_type == 'T')
            {
                fscanf(object_programs, "%6x%2x", &tr.start_address, &tr.length);
                save_to_mem_array(object_programs, CSADDR + tr.start_address, tr.length);
            }

            if (current_record_type == 'M')
            {
                fscanf(object_programs, "%6x%2x%c%s", &mr.start_address, &mr.half_bytes, &mr.operator_symbol, mr.symbol);
                int symbol_value = search_symbol(mr.symbol);

                if (symbol_value == ERROR_VALUE)
                {
                    printf("ERROR: Undefined external refernce (%s)\n", mr.symbol);
                    return ERROR_VALUE;
                }

                modify_memory(CSADDR + mr.start_address, mr.half_bytes, mr.operator_symbol, symbol_value);
            }
        }

        CSADDR += CSLTH;
        total_length += CSLTH;
    }

    print_without_modifications(PROGADDR, total_length);
    print_with_modifications(PROGADDR, total_length);
    print_both(PROGADDR, total_length);

    return 1;
}

void save_to_mem_array(FILE *object_programs, int start_address, int tr_length)
{
    int obj_instruction;
    int i;

    for (i = 0; i < tr_length; i++)
    {
        fscanf(object_programs, "%2x", &obj_instruction);
        memory_array[start_address + i] = obj_instruction;

        memory_array_wom[start_address + i] = obj_instruction;
    }

    return;
}

int search_symbol(char *symbol)
{
    int i;
    for (i = 0; i < estab_length; i++)
    {
        if (strcmp(symbol, estab[i].symbol) == 0)
        {
            return estab[i].address;
        }
    }

    return ERROR_VALUE;
}

void init_estab()
{
    FILE *estab_file = fopen("ESTAB.txt", "r");
    char symbol[MAX_BUF];
    char dummy[MAX_BUF];
    int address;
    char length[MAX_BUF];

    int i = 0;
    while (fscanf(estab_file, "%s\t%s\t%x\t%s", dummy, symbol, &address, &length) > 0)
    {
        if (strcmp(symbol, "****") != 0)
        {
            strcpy(estab[i].symbol, symbol);
            estab[i].address = address;
            i++;
        }
    }

    estab_length = i;
}

void show_estab()
{
    for (int i = 0; i < estab_length; i++)
    {
        printf("%8s%6x\n", estab[i].symbol, estab[i].address);
    }
    return;
}

void modify_memory(int location, int half_bytes, char operator_symbol, int symbol_value)
{
    int i;
    int modified_value = 0x00000000;

    for (i = 0; i < half_bytes / 2 + half_bytes % 2; i++)
    {
        modified_value <<= 8;
        modified_value += memory_array[location + i];
    }

    switch (operator_symbol)
    {
    case '+':
        modified_value += symbol_value;
        break;
    case '-':
        modified_value -= symbol_value;
        break;
    }

    for (; i >= 0; i--)
    {
        memory_array[location + i] = modified_value & 0x000000FF;
        modified_value >>= 8;
    }
}

void print_without_modifications(int PROGADDR, int total_length)
{
    FILE *without_modifications = fopen("without_modifications.txt", "a");

    for (int i = 0; i < total_length; i++)
    {
        if(i % 4 == 0)
            fprintf(without_modifications, " ");
        if (i % 16 == 0)
            fprintf(without_modifications, "\n%8x ", PROGADDR + i);
        fprintf(without_modifications, "%02x", memory_array_wom[PROGADDR + i]);
    }
}

void print_with_modifications(int PROGADDR, int total_length)
{
    FILE *with_modifications = fopen("with_modifications.txt", "a");

    for (int i = 0; i < total_length; i++)
    {
        if(i % 4 == 0)
            fprintf(with_modifications, " ");
        if (i % 16 == 0)
            fprintf(with_modifications, "\n%8x ", PROGADDR + i);
        fprintf(with_modifications, "%02x", memory_array[PROGADDR + i]);
    }
}

void print_both(int PROGADDR, int total_length)
{
    FILE *both = fopen("pass-2-output.txt", "w");
    fprintf(both, "%8s %8s %12s", "Address", "Modified", "Not-Modified");
    for (int i = 0; i < total_length; i += 4)
    {
        if (i % 4 == 0)
            fprintf(both, "\n%8x ", PROGADDR + i);
        fprintf(both, "%02x", memory_array[PROGADDR + i]);
        fprintf(both, "%02x", memory_array[PROGADDR + i + 1]);
        fprintf(both, "%02x", memory_array[PROGADDR + i + 2]);
        fprintf(both, "%02x ", memory_array[PROGADDR + i + 3]);

        fprintf(both, "%02x", memory_array_wom[PROGADDR + i]);
        fprintf(both, "%02x", memory_array_wom[PROGADDR + i + 1]);
        fprintf(both, "%02x", memory_array_wom[PROGADDR + i + 2]);
        fprintf(both, "%02x", memory_array_wom[PROGADDR + i + 3]);
    }
}
