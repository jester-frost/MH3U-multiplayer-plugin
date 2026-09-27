/*  teste_nucleo.c -- o nucleo do instalador no PC, com os arquivos do jogo
 *  original, reproduz o update.cia byte a byte?
 *
 *  gcc -O2 -I../source ../source/nucleo.c teste_nucleo.c -o teste_nucleo
 *  ./teste_nucleo <receita.bin> <mh3u.bps> <code_comprimido> <banner> <logo> <icone> <saida.cia>
 */
#include "nucleo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *ler(const char *f, uint32_t *n)
{
    FILE *a = fopen(f, "rb"); if (!a) { perror(f); exit(2); }
    fseek(a, 0, SEEK_END); *n = (uint32_t)ftell(a); fseek(a, 0, SEEK_SET);
    uint8_t *b = malloc(*n); if (fread(b, 1, *n, a) != *n) exit(2); fclose(a); return b;
}
static int esc(void *ctx, const uint8_t *p, uint32_t n) { return fwrite(p, 1, n, (FILE *)ctx) == n ? 0 : -1; }
static void confere(const char *nome, const uint8_t *p, uint32_t n, const uint8_t *esperado)
{
    uint8_t h[32]; sha256(p, n, h);
    printf("%-12s %s\n", nome, memcmp(h, esperado, 32) ? "DIFERENTE" : "ok");
    if (memcmp(h, esperado, 32)) exit(1);
}

int main(int argc, char **argv)
{
    if (argc != 8) { fprintf(stderr, "uso: ver o comentario\n"); return 2; }
    uint32_t nr, nb, nc, nban, nlogo, nic; Receita r;
    uint8_t *rec = ler(argv[1], &nr), *bps = ler(argv[2], &nb), *comp = ler(argv[3], &nc);
    uint8_t *ban = ler(argv[4], &nban), *logo = ler(argv[5], &nlogo), *ic = ler(argv[6], &nic);
    int e = receita_ler(rec, nr, &r);
    if (e) { printf("receita: %s\n", nucleo_erro(e)); return 1; }
    printf("receita %s: code %u -> %u, cia %llu B\n", r.versao, r.tam_code_base, r.tam_code_alvo, (unsigned long long)r.tam_cia);

    uint32_t td = blz_tamanho(comp, nc);
    if (td != r.tam_code_base) { printf("tamanho descomprimido %u != %u\n", td, r.tam_code_base); return 1; }
    uint8_t *code = malloc(r.tam_code_alvo > td ? r.tam_code_alvo : td);
    e = blz_descomprimir(comp, nc, code, td); if (e) { printf("%s\n", nucleo_erro(e)); return 1; }
    confere("code base", code, td, r.sha_code_base);
    uint32_t ta; e = bps_aplicar_no_lugar(bps, nb, code, td, r.tam_code_alvo, &ta);
    if (e) { printf("bps: %s\n", nucleo_erro(e)); return 1; }
    confere("code alvo", code, ta, r.sha_code_alvo);
    confere("banner", ban, nban, r.sha_banner);
    confere("logo", logo, nlogo, r.sha_logo);
    confere("icone base", ic, nic, r.sha_icone);
    receita_icone(&r, ic, nic);

    Pedacos pc = { code, ta, ban, nban, logo, nlogo, ic, nic };
    FILE *s = fopen(argv[7], "wb");
    e = receita_montar(&r, &pc, esc, NULL, s); fclose(s);
    printf("montar: %s\n", e ? nucleo_erro(e) : "ok (SHA-256 do update.cia confere)");
    return e ? 1 : 0;
}
