/*  atualiza.c -- assinatura, manifesto e versoes (ver atualiza.h). */
#include "atualiza.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ------------------------------------------------------------ RSA-2048 */
/*  So a verificacao, com e = 65537: 16 quadrados + 1 produto, em Montgomery
 *  com palavras de 32 bits (CIOS). Sem dependencia nenhuma -- o devkitPro nao
 *  traz mbedTLS e para verificar isto basta. */
#define NL 64                            /* 2048 / 32 */
typedef uint32_t Bn[NL];

static void bn_de_be(Bn r, const uint8_t *b)
{
    for (int i = 0; i < NL; ++i) {
        const uint8_t *p = b + 256 - 4 * (i + 1);
        r[i] = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
    }
}

static void bn_para_be(uint8_t *b, const Bn a)
{
    for (int i = 0; i < NL; ++i) {
        uint8_t *p = b + 256 - 4 * (i + 1);
        p[0] = a[i] >> 24; p[1] = a[i] >> 16; p[2] = a[i] >> 8; p[3] = a[i];
    }
}

static int bn_cmp(const uint32_t *a, const uint32_t *b)
{
    for (int i = NL - 1; i >= 0; --i)
        if (a[i] != b[i]) return a[i] > b[i] ? 1 : -1;
    return 0;
}

static void bn_sub(uint32_t *a, const uint32_t *b)          /* a -= b */
{
    uint64_t emp = 0;
    for (int i = 0; i < NL; ++i) {
        uint64_t d = (uint64_t)a[i] - b[i] - emp;
        a[i] = (uint32_t)d; emp = (d >> 63) & 1;
    }
}

static void mont(Bn r, const Bn a, const Bn b, const Bn n, uint32_t n0)
{
    uint32_t t[NL + 2]; memset(t, 0, sizeof t);
    for (int i = 0; i < NL; ++i) {
        uint64_t c = 0, s;
        for (int j = 0; j < NL; ++j) { s = (uint64_t)t[j] + (uint64_t)a[j] * b[i] + c; t[j] = (uint32_t)s; c = s >> 32; }
        s = (uint64_t)t[NL] + c; t[NL] = (uint32_t)s; t[NL + 1] = (uint32_t)(s >> 32);
        uint32_t m = t[0] * n0;
        s = (uint64_t)t[0] + (uint64_t)m * n[0]; c = s >> 32;
        for (int j = 1; j < NL; ++j) { s = (uint64_t)t[j] + (uint64_t)m * n[j] + c; t[j - 1] = (uint32_t)s; c = s >> 32; }
        s = (uint64_t)t[NL] + c; t[NL - 1] = (uint32_t)s; c = s >> 32;
        t[NL] = t[NL + 1] + (uint32_t)c; t[NL + 1] = 0;
    }
    if (t[NL] || bn_cmp(t, n) >= 0) bn_sub(t, n);
    memcpy(r, t, sizeof(Bn));
}

int rsa2048_verificar(const uint8_t n_be[256], const uint8_t sig[256], const uint8_t sha256[32])
{
    Bn n, s, r2, x, y, um;
    bn_de_be(n, n_be); bn_de_be(s, sig);
    if (!(n[0] & 1) || bn_cmp(s, n) >= 0) return -1;
    uint32_t inv = 1;                                     /* n[0]^-1 mod 2^32 (Newton) */
    for (int i = 0; i < 5; ++i) inv *= 2 - n[0] * inv;
    uint32_t n0 = (uint32_t)-inv;
    memset(r2, 0, sizeof r2); r2[0] = 1;                  /* R^2 mod n = 2^4096 mod n */
    for (int i = 0; i < 2 * NL * 32; ++i) {
        uint32_t vai = r2[NL - 1] >> 31;
        for (int j = NL - 1; j > 0; --j) r2[j] = r2[j] << 1 | r2[j - 1] >> 31;
        r2[0] <<= 1;
        if (vai || bn_cmp(r2, n) >= 0) bn_sub(r2, n);
    }
    mont(x, s, r2, n, n0);                                /* s em Montgomery */
    memcpy(y, x, sizeof y);
    for (int i = 0; i < 16; ++i) mont(y, y, y, n, n0);    /* s^65536 */
    mont(y, y, x, n, n0);                                 /* s^65537 */
    memset(um, 0, sizeof um); um[0] = 1;
    mont(y, y, um, n, n0);                                /* fora de Montgomery */
    uint8_t em[256], esperado[256];
    bn_para_be(em, y);
    static const uint8_t di[19] = { 0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
                                    0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20 };
    esperado[0] = 0; esperado[1] = 1;
    memset(esperado + 2, 0xff, 256 - 3 - 19 - 32);
    esperado[256 - 1 - 19 - 32] = 0;
    memcpy(esperado + 256 - 19 - 32, di, 19);
    memcpy(esperado + 256 - 32, sha256, 32);
    return memcmp(em, esperado, 256) ? -2 : 0;
}

/* ------------------------------------------------------------ manifesto */
static int hexbyte(const char *p)
{
    int v = 0;
    for (int k = 0; k < 2; ++k) {
        char c = p[k]; v <<= 4;
        if (c >= '0' && c <= '9') v |= c - '0';
        else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
        else return -1;
    }
    return v;
}

static void copia(char *dst, size_t cap, const char *src, size_t n)
{
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n); dst[n] = 0;
}

/*  o formato esta em atualiza.h */
/* "mudanca" -> 0, "mudanca.en" -> 1, "mudanca.es" -> 2, outra chave -> -1 */
static int idioma_chave(const char *k, size_t nk, const char *base)
{
    size_t nb = strlen(base);
    if (nk < nb || memcmp(k, base, nb)) return -1;
    if (nk == nb) return 0;
    if (nk == nb + 3 && k[nb] == '.' && !memcmp(k + nb + 1, "en", 2)) return 1;
    if (nk == nb + 3 && k[nb] == '.' && !memcmp(k + nb + 1, "es", 2)) return 2;
    return -1;
}

int manifesto_ler(const char *txt, size_t n, Manifesto *m)
{
    int li;
    memset(m, 0, sizeof *m);
    const char *p = txt, *fim = txt + n;
    while (p < fim) {
        const char *q = memchr(p, '\n', fim - p); if (!q) q = fim;
        const char *e = q; if (e > p && e[-1] == '\r') --e;
        const char *ig = memchr(p, '=', e - p);
        if (ig) {
            size_t nk = ig - p; const char *v = ig + 1; size_t nv = e - v;
            if (nk == 6 && !memcmp(p, "versao", 6)) copia(m->versao, sizeof m->versao, v, nv);
            else if (nk == 10 && !memcmp(p, "instalador", 10)) copia(m->versao_inst, sizeof m->versao_inst, v, nv);
            else if (nk == 4 && !memcmp(p, "data", 4)) copia(m->data, sizeof m->data, v, nv);
            else if ((li = idioma_chave(p, nk, "mudanca")) >= 0) {
                if (m->n_mudancas[li] < ATU_MAX_MUDANCAS) copia(m->mudancas[li][m->n_mudancas[li]++], 64, v, nv);
            }
            else if (nk == 7 && !memcmp(p, "arquivo", 7) && m->n_arq < ATU_MAX_ARQ) {
                char ln[256]; copia(ln, sizeof ln, v, nv);
                char nome[24], sha[65], cam[128]; unsigned long tam;
                if (sscanf(ln, "%23s %lu %64s %127s", nome, &tam, sha, cam) != 4 || strlen(sha) != 64) return -2;
                AtuArquivo *a = &m->arq[m->n_arq++];
                strcpy(a->nome, nome); a->tam = (uint32_t)tam; strcpy(a->caminho, cam);
                for (int k = 0; k < 32; ++k) { int b = hexbyte(sha + 2 * k); if (b < 0) return -3; a->sha[k] = (uint8_t)b; }
            }
            else if ((li = idioma_chave(p, nk, "historico")) >= 0) {
                char ln[128], ver[16], data[12]; int off = 0;
                copia(ln, sizeof ln, v, nv);
                if (sscanf(ln, "%15s %11s %n", ver, data, &off) < 2) return -4;
                AtuVersao *h = NULL;
                for (int i = 0; i < m->n_hist; ++i) if (!strcmp(m->hist[i].versao, ver)) h = &m->hist[i];
                if (!h && m->n_hist < ATU_MAX_HIST) {
                    h = &m->hist[m->n_hist++];
                    strcpy(h->versao, ver); strcpy(h->data, data);
                }
                if (h) copia(h->resumo[li], sizeof h->resumo[li], ln + off, strlen(ln + off));
            }
        }
        p = q + 1;
    }
    return m->versao[0] && m->n_arq ? 0 : -1;
}

int manifesto_idioma_mudancas(const Manifesto *m, int idioma)
{
    if (idioma >= 0 && idioma < ATU_IDIOMAS && m->n_mudancas[idioma]) return idioma;
    return m->n_mudancas[1] ? 1 : 0;
}

const char *manifesto_resumo(const AtuVersao *h, int idioma)
{
    if (idioma >= 0 && idioma < ATU_IDIOMAS && h->resumo[idioma][0]) return h->resumo[idioma];
    return h->resumo[1][0] ? h->resumo[1] : h->resumo[0];
}

const AtuArquivo *manifesto_arquivo(const Manifesto *m, const char *nome)
{
    for (int i = 0; i < m->n_arq; ++i)
        if (!strcmp(m->arq[i].nome, nome)) return &m->arq[i];
    return NULL;
}

uint32_t versao_numero(const char *v)
{
    unsigned a = 0, b = 0, beta = 0;
    if (sscanf(v, "%u.%u", &a, &b) < 1) return 0;
    const char *s = strstr(v, "-beta");
    if (s) beta = (unsigned)atoi(s + 5); else beta = 1023;   /* a final vem depois das betas */
    return (a & 0x3ff) << 20 | (b & 0x3ff) << 10 | (beta & 0x3ff);
}
