#include <stdio.h>
#include <stdlib.h>

#define TAMANHO 400

void comparar_arquivos(const char* nome_res, const char* nome_ref) 
{
    float vetor1[TAMANHO], vetor2[TAMANHO];
    FILE* arq_res, * arq_ref;
    int i;

    arq_res = fopen(nome_res, "r");
    arq_ref = fopen(nome_ref, "r");

    if (arq_res == NULL || arq_ref == NULL) 
    {
        printf("\n[AVISO] Falha ao abrir par: %s ou %s (Pode nao existir)\n", nome_res, nome_ref);
        if (arq_res) fclose(arq_res);
        if (arq_ref) fclose(arq_ref);
        return;
    }

    printf("\n--- Comparando %s vs %s ---\n", nome_res, nome_ref);

    int divergencias = 0;
    for (i = 0; i < TAMANHO; i++) 
    {
        if (fscanf(arq_res, "%f", &vetor1[i]) == EOF) vetor1[i] = 0;
        if (fscanf(arq_ref, "%f", &vetor2[i]) == EOF) vetor2[i] = 0;

        if (vetor1[i] != vetor2[i]) 
        {
            printf("  Indice [%d]: %.4f != %.4f\n", i, vetor1[i], vetor2[i]);
            divergencias++;
        }
    }

    if (divergencias == 0) 
    {
        printf("  Arquivos identicos!\n");
    }
    else 
    {
        printf("  Total de %d divergencias encontradas.\n", divergencias);
    }

    fclose(arq_res);
    fclose(arq_ref);
}

int main() 
{
    int n = 3;
    char nome_res[255];
    char nome_ref[255];

    for (int i = 1; i <= 3; i++) 
    {
        sprintf(nome_res, "res_scenario_%d.txt", i);
        sprintf(nome_ref, "ref_scenario_%d.txt", i);

        comparar_arquivos(nome_res, nome_ref);
    }

    return 0;
}