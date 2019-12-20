#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NUM_PACKETS 50

typedef struct
{
    uint16_t A0;
    uint16_t A1;
    uint16_t A2;
    uint16_t A3;
    uint16_t A4;
    uint16_t A5;
    uint16_t pulses_chan1;
    uint16_t pulses_chan2;
    uint32_t timestamp;
} packet;

int main()
{
    char error, first = 0;
    int RUN, part = 0;
    char filename[50];
    char foldername[30];
    FILE *f, *fp;

    printf("Insira o nome da pasta em que se encontram os dados: ");
    scanf(" %s", foldername);
    while(1)
    {
        error = 0;
        part = 0;
        printf("Insira o número da corrida a ser lida (negativo para sair): ");
        scanf(" %d", &RUN);

        if(RUN < 0)
            break;

        sprintf(filename, "%s/RUN%d.csv", foldername, RUN);
        f = fopen(filename, "wt");
        // printf("file = %ld\r\n", f);
        
        fprintf(f, "a0,a1,a2,a3,a4,a5,f1,f2,timestamp\n");
        while(!error)
        {
            char name[70];
            sprintf(name, "%s/%s%d/%s%d", foldername,"RUN", RUN, "part", part++);
            printf("filename = %s\n", name);
            fp = fopen(name, "r");
            packet x[NUM_PACKETS];
            
            if (fp == NULL)
            {
                error = 1;
                break;
            }   
        
            printf("~~~~~~~~PART %d ~~~~~~~~", part);
            
            fread((void *)x, sizeof(packet), NUM_PACKETS, fp);
            for(int i = 0; i<(ftell(fp)/sizeof(packet)); i++)
            {
                fprintf(f, "%d,%d,%d,%d,%d,%d,%d,%d,%i\n", x[i].A0, x[i].A1, x[i].A2, x[i].A3, x[i].A4, x[i].A5, x[i].pulses_chan1, x[i].pulses_chan2, x[i].timestamp);
            }
            fclose(fp);
            fp = NULL;
        }
        fclose(f);
        f = NULL;
    }
    
    return 0;
}
