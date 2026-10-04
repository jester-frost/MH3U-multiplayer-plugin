/*  atualiza.h -- a parte da autoatualizacao que nao depende do 3DS (testavel no PC).
 *
 *  O instalador baixa do repositorio publico no GitHub (espelho: jsDelivr),
 *  pela rede.c (DNS e TLS proprios, medidos no .85 em 03/10/2026):
 *
 *    atualizacao/versao.txt   o manifesto (texto, chave=valor)
 *    atualizacao/versao.sig   RSA-2048 PKCS#1 v1.5 SHA-256 do versao.txt
 *
 *  A chave publica vai embutida (chave_atualizacao.h, gerada por
 *  tools/mh3u_atualizacao.py); a privada fica so no PC de quem publica. Cada
 *  arquivo do manifesto tem tamanho e SHA-256: o que vier diferente e recusado.
 *
 *    versao=1.0-beta8
 *    instalador=1.0-beta8
 *    data=2026-10-04
 *    mudanca=<o que esta versao faz>                         (ate 8)
 *    arquivo=<nome> <tam> <sha256 hex> <ref>/<caminho>       (ate 4)
 *    historico=<versao> <data> <resumo>                      (ate 16)
 *
 *  mudanca= e historico= sao em portugues; mudanca.en=, mudanca.es=,
 *  historico.en= e historico.es= trazem os outros idiomas (o app cai no
 *  ingles e depois no portugues quando falta o do console).
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

#define ATU_MAX_ARQ      4
#define ATU_MAX_MUDANCAS 8
#define ATU_MAX_HIST     16
#define ATU_IDIOMAS      3      /* pt, en, es -- a ordem do textos.h */

typedef struct {
    char     nome[24];          /* receita.bin, mh3u.bps, instalador.cia */
    uint32_t tam;
    uint8_t  sha[32];
    char     caminho[128];      /* caminho no repositorio, com a ref: v1.0-beta8/nativo/code.bps */
} AtuArquivo;

typedef struct {
    char versao[16], data[12], resumo[ATU_IDIOMAS][40];
} AtuVersao;

typedef struct {
    char       versao[24];      /* do patch (a receita): 1.0-beta8 */
    char       versao_inst[24]; /* do instalador.cia, se houver */
    char       data[16];
    char       mudancas[ATU_IDIOMAS][ATU_MAX_MUDANCAS][64];
    int        n_mudancas[ATU_IDIOMAS];
    AtuArquivo arq[ATU_MAX_ARQ];
    int        n_arq;
    AtuVersao  hist[ATU_MAX_HIST]; /* da mais nova p/ a mais velha (aba Versoes) */
    int        n_hist;
} Manifesto;

/* 0 se a assinatura confere (n = modulo big-endian de 256 bytes, e = 65537) */
int  rsa2048_verificar(const uint8_t n_be[256], const uint8_t sig[256], const uint8_t sha256[32]);

int  manifesto_ler(const char *txt, size_t n, Manifesto *m);
const AtuArquivo *manifesto_arquivo(const Manifesto *m, const char *nome);

/* o idioma que tem texto: o pedido, senao ingles, senao portugues */
int manifesto_idioma_mudancas(const Manifesto *m, int idioma);
const char *manifesto_resumo(const AtuVersao *h, int idioma);

/* "1.0-beta8" -> numero comparavel; versao final (sem -beta) vem depois das betas */
uint32_t versao_numero(const char *v);
