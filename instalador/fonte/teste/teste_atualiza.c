/*  teste_atualiza.c -- a verificacao RSA e o manifesto, no PC.
 *  gcc -O2 -I../source ../source/atualiza.c ../source/nucleo.c teste_atualiza.c -o teste_atualiza
 *  ./teste_atualiza <modulo.bin 256 B> <versao.txt> <versao.sig>  */
#include "atualiza.h"
#include "nucleo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *ler(const char *f, size_t *n)
{ FILE *a = fopen(f, "rb"); if (!a) { perror(f); exit(2); } fseek(a, 0, SEEK_END); *n = ftell(a); fseek(a, 0, SEEK_SET);
  uint8_t *b = malloc(*n + 1); if (fread(b, 1, *n, a) != *n) exit(2); fclose(a); b[*n] = 0; return b; }
int main(int argc, char **argv)
{
    size_t nm, nt, ns; uint8_t *mod = ler(argv[1], &nm), *txt = ler(argv[2], &nt), *sig = ler(argv[3], &ns);
    uint8_t h[32]; sha256(txt, nt, h);
    printf("assinatura: %s\n", rsa2048_verificar(mod, sig, h) == 0 ? "OK" : "FALHOU");
    txt[0] ^= 1; sha256(txt, nt, h); txt[0] ^= 1;
    printf("manifesto adulterado: %s\n", rsa2048_verificar(mod, sig, h) == 0 ? "ACEITO (ERRO!)" : "recusado");
    Manifesto m; int e = manifesto_ler((char *)txt, nt, &m);
    printf("manifesto: %d versao=%s inst=%s data=%s arquivos=%d mudancas=%d\n", e, m.versao, m.versao_inst, m.data, m.n_arq, m.n_mudancas[0]);
    for (int i = 0; i < m.n_arq; ++i) printf("  %s %u %02x%02x.. %s\n", m.arq[i].nome, m.arq[i].tam, m.arq[i].sha[0], m.arq[i].sha[1], m.arq[i].caminho);
    printf("versoes: beta7<beta8 %d, beta9<beta10 %d, beta10<1.0 %d\n",
           versao_numero("1.0-beta7") < versao_numero("1.0-beta8"), versao_numero("1.0-beta9") < versao_numero("1.0-beta10"),
           versao_numero("1.0-beta10") < versao_numero("1.0"));
    return 0;
}
