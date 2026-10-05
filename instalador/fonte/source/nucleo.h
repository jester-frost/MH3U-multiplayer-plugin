/*  nucleo.h -- a parte do instalador que nao depende do 3DS (testavel no PC).
 *
 *  - blz_*     : descompressao do .code (LZ "reverso" do ExeFS do 3DS)
 *  - bps_*     : aplica o code.bps (so SourceRead/TargetRead/TargetCopy), no lugar
 *  - sha256_*  : SHA-256
 *  - receita_* : le a receita (tools/mh3u_receita.py) e monta o update.cia
 *
 *  Receita formato 2 (1.4+): traz o mh3u.bps dentro e manda buscar o conteudo
 *  novo em PECAS -- um arquivo por peca, nomeado pelo SHA-256 (<pasta>/<sha>.bin).
 *  O app baixa so as pecas que ainda nao estao no cartao (fila de atualizacao).
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

/* ---- SHA-256 ---- */
typedef struct { uint32_t h[8]; uint64_t n; uint8_t buf[64]; size_t k; } Sha256;
void sha256_ini(Sha256 *s);
void sha256_add(Sha256 *s, const void *p, size_t n);
void sha256_fim(Sha256 *s, uint8_t out[32]);
void sha256(const void *p, size_t n, uint8_t out[32]);

/* ---- .code comprimido (BLZ) ---- */
uint32_t blz_tamanho(const uint8_t *comp, uint32_t n);          /* tamanho descomprimido */
int      blz_descomprimir(const uint8_t *comp, uint32_t n, uint8_t *out, uint32_t tam_out);

/* ---- BPS, no lugar: buf tem a ORIGEM (tam_origem bytes) e capacidade p/ o alvo ---- */
int bps_aplicar_no_lugar(const uint8_t *patch, uint32_t n, uint8_t *buf, uint32_t tam_origem,
                         uint32_t cap, uint32_t *tam_alvo);

/* ---- receita ---- */
typedef struct {
    char     versao[16];
    uint32_t tam_code_base, tam_code_alvo;
    uint8_t  sha_code_base[32], sha_code_alvo[32];
    uint8_t  sha_banner[32], sha_logo[32], sha_icone[32];
    uint8_t  sha_cia[32];
    uint64_t tam_cia;
    uint32_t formato;                              /* 1 ou 2 */
    const uint8_t *bps;          uint32_t n_bps;   /* so no formato 2 */
    const uint8_t *patch_icone;  uint32_t n_patch_icone;
    const uint8_t *segs;         uint32_t n_segs;
} Receita;

int receita_ler(const uint8_t *p, uint32_t n, Receita *r);
/* aplica os nossos textos sobre o icone do jogo base (in place) */
void receita_icone(const Receita *r, uint8_t *icone, uint32_t tam);

/* as pecas que a receita usa, na ordem (pode repetir); devolve quantas */
typedef void (*VerPeca)(void *ctx, const uint8_t sha[32], uint32_t tam);
uint32_t receita_pecas(const Receita *r, VerPeca ver, void *ctx);
/* "<pasta>/<sha hex>.bin" */
void peca_caminho(const char *pasta, const uint8_t sha[32], char *out, size_t cap);

/* Os pedacos do jogo, ja prontos: */
typedef struct {
    const uint8_t *code;   uint32_t tam_code;     /* base descomprimido + bps */
    const uint8_t *banner; uint32_t tam_banner;
    const uint8_t *logo;   uint32_t tam_logo;
    const uint8_t *icone;  uint32_t tam_icone;    /* ja com receita_icone */
    const char *pasta_pecas;                      /* onde estao as pecas (formato 2) */
} Pedacos;

/* Emite o CIA em blocos: escrever(ctx, dados, n) devolve 0 se ok. Calcula o
 * SHA-256 no caminho e devolve 0 so se bater com a receita (tamanho e hash).
 * progresso(ctx, feito, total) pode ser NULL. */
typedef int  (*Escritor)(void *ctx, const uint8_t *p, uint32_t n);
typedef void (*Progresso)(void *ctx, uint64_t feito, uint64_t total);
int receita_montar(const Receita *r, const Pedacos *pc, Escritor esc, Progresso prog, void *ctx);

const char *nucleo_erro(int e);
