#include <stdlib.h>
#include <stdio.h>


int main(){
    // Create program
    FILE* program_handle = fopen("kernel.cl", "r");
    FILE* x = program_handle;
    printf("Address of the file at first:%p\n",program_handle);

    fseek(program_handle, 0, SEEK_END);
    printf("Address of the file at the end:%p\n",program_handle);
    printf("number of bytes:%p\n",x - program_handle);

    long program_size = ftell(program_handle);
    printf("file tell: %d\n", program_size);
    rewind(program_handle);
    printf("Address of the file after rewind:%p\n",program_handle);

    char* program_buffer = (char*)malloc(program_size+1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle); 
    fclose(program_handle);
    int i = 0;
    while (i < program_size)
    {
        printf("%c",program_buffer[i]);
        i++;
    }
    printf("\n last character: %c", program_buffer[program_size]);
    
    
}