/*  nucleo.c -- ver nucleo.h. Sem nada do 3DS: compila no PC (teste/teste_nucleo.c). */
#include "nucleo.h"
#include <string.h>

enum {
    OK = 0, E_BLZ = -1, E_BPS_MAGIC = -2, E_BPS_TAM = -3, E_BPS_CMD = -4, E_BPS_CRC = -5,
    E_REC_MAGIC = -6, E_REC_CURTA = -7, E_REC_SEG = -8, E_ESCRITA = -9, E_CIA_TAM = -10,
    E_CIA_SHA = -11,
};

const char *nucleo_erro(int e)
{
    switch (e) {
    case E_BLZ:       return "descompressao do .code falhou";
    case E_BPS_MAGIC: return "patch (BPS) invalido";
    case E_BPS_TAM:   return "patch (BPS) de outro tamanho de jogo";
    case E_BPS_CMD:   return "patch (BPS) com comando inesperado";
    case E_BPS_CRC:   return "patch (BPS) nao confere (CRC)";
    case E_REC_MAGIC: return "receita invalida";
    case E_REC_CURTA: return "receita cortada";
    case E_REC_SEG:   return "receita com segmento desconhecido";
    case E_ESCRITA:   return "falha ao gravar o update";
    case E_CIA_TAM:   return "update montado com tamanho errado";
    case E_CIA_SHA:   return "update montado nao confere (SHA-256)";
    default:          return "erro";
    }
}

/* ================= SHA-256 ================= */
static const uint32_t K[64] = {
 0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
 0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
 0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
 0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
 0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
#define ROR(x,n) (((x)>>(n))|((x)<<(32-(n))))
static void sha_bloco(Sha256 *s, const uint8_t *b)
{
    uint32_t w[64], a,bb,c,d,e,f,g,h,t1,t2; int i;
    for (i = 0; i < 16; ++i) w[i] = (uint32_t)b[i*4]<<24 | (uint32_t)b[i*4+1]<<16 | (uint32_t)b[i*4+2]<<8 | b[i*4+3];
    for (; i < 64; ++i) {
        uint32_t s0 = ROR(w[i-15],7) ^ ROR(w[i-15],18) ^ (w[i-15]>>3);
        uint32_t s1 = ROR(w[i-2],17) ^ ROR(w[i-2],19) ^ (w[i-2]>>10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    a=s->h[0]; bb=s->h[1]; c=s->h[2]; d=s->h[3]; e=s->h[4]; f=s->h[5]; g=s->h[6]; h=s->h[7];
    for (i = 0; i < 64; ++i) {
        t1 = h + (ROR(e,6)^ROR(e,11)^ROR(e,25)) + ((e&f)^(~e&g)) + K[i] + w[i];
        t2 = (ROR(a,2)^ROR(a,13)^ROR(a,22)) + ((a&bb)^(a&c)^(bb&c));
        h=g; g=f; f=e; e=d+t1; d=c; c=bb; bb=a; a=t1+t2;
    }
    s->h[0]+=a; s->h[1]+=bb; s->h[2]+=c; s->h[3]+=d; s->h[4]+=e; s->h[5]+=f; s->h[6]+=g; s->h[7]+=h;
}
void sha256_ini(Sha256 *s)
{
    static const uint32_t H0[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    memcpy(s->h, H0, sizeof H0); s->n = 0; s->k = 0;
}
void sha256_add(Sha256 *s, const void *pv, size_t n)
{
    const uint8_t *p = pv;
    s->n += n;
    if (s->k) {
        size_t t = 64 - s->k; if (t > n) t = n;
        memcpy(s->buf + s->k, p, t); s->k += t; p += t; n -= t;
        if (s->k == 64) { sha_bloco(s, s->buf); s->k = 0; }
    }
    while (n >= 64) { sha_bloco(s, p); p += 64; n -= 64; }
    if (n) { memcpy(s->buf, p, n); s->k = n; }
}
void sha256_fim(Sha256 *s, uint8_t out[32])
{
    uint64_t bits = s->n * 8; uint8_t pad = 0x80, z = 0; int i;
    sha256_add(s, &pad, 1);
    while (s->k != 56) sha256_add(s, &z, 1);
    for (i = 7; i >= 0; --i) { uint8_t b = (uint8_t)(bits >> (i*8)); sha256_add(s, &b, 1); }
    for (i = 0; i < 8; ++i) { out[i*4]=s->h[i]>>24; out[i*4+1]=s->h[i]>>16; out[i*4+2]=s->h[i]>>8; out[i*4+3]=s->h[i]; }
}
void sha256(const void *p, size_t n, uint8_t out[32]) { Sha256 s; sha256_ini(&s); sha256_add(&s, p, n); sha256_fim(&s, out); }

/* ================= BLZ (.code do ExeFS) ================= */
static uint32_t le32(const uint8_t *p) { return p[0] | p[1]<<8 | p[2]<<16 | (uint32_t)p[3]<<24; }

uint32_t blz_tamanho(const uint8_t *comp, uint32_t n)
{
    return n < 8 ? 0 : n + le32(comp + n - 4);
}

/*  O mesmo algoritmo do ctrtool/3dstool: le de tras para frente. */
int blz_descomprimir(const uint8_t *comp, uint32_t n, uint8_t *out, uint32_t tam)
{
    if (n < 8 || tam < n) return E_BLZ;
    uint32_t tb = le32(comp + n - 8);
    uint32_t idx = n - ((tb >> 24) & 0xFF);
    uint32_t para = n - (tb & 0xFFFFFF);
    uint32_t o = tam;
    memset(out + n, 0, tam - n);
    memmove(out, comp, n);
    while (idx > para) {
        uint8_t ctl = out[--idx];
        for (int i = 0; i < 8; ++i) {
            if (idx <= para || o == 0) break;
            if (ctl & 0x80) {
                if (idx < 2) return E_BLZ;
                idx -= 2;
                uint32_t so = out[idx] | out[idx + 1] << 8;
                uint32_t st = ((so >> 12) & 15) + 3;
                so = (so & 0x0FFF) + 2;
                if (o < st) return E_BLZ;
                for (uint32_t j = 0; j < st; ++j) {
                    if (o + so >= tam) return E_BLZ;
                    uint8_t b = out[o + so];
                    out[--o] = b;
                }
            } else {
                if (o < 1) return E_BLZ;
                out[--o] = out[--idx];
            }
            ctl <<= 1;
        }
    }
    return OK;
}

/* ================= BPS ================= */
static uint32_t crc32(const uint8_t *p, uint32_t n)
{
    /* com tabela: bit a bit, 2 x 13 MB levava muito no 3DS (e no Azahar) */
    static uint32_t t[256]; static int pronta = 0;
    if (!pronta) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320 & -(c & 1));
            t[i] = c;
        }
        pronta = 1;
    }
    uint32_t c = 0xFFFFFFFF;
    while (n--) c = t[(c ^ *p++) & 0xFF] ^ (c >> 8);
    return ~c;
}
static int bps_num(const uint8_t *p, uint32_t n, uint32_t *pos, uint64_t *v)
{
    uint64_t d = 0, sh = 1;
    for (;;) {
        if (*pos >= n) return -1;
        uint8_t x = p[(*pos)++];
        d += (x & 0x7F) * sh;
        if (x & 0x80) { *v = d; return 0; }
        sh <<= 7; d += sh;
    }
}
/*  No lugar: so funciona com SourceRead (mesma posicao), TargetRead e
 *  TargetCopy -- que e o que o tools/mh3u_nativo.py gera. SourceCopy recusa. */
int bps_aplicar_no_lugar(const uint8_t *p, uint32_t n, uint8_t *buf, uint32_t tam_origem,
                         uint32_t cap, uint32_t *tam_alvo)
{
    uint32_t pos = 4; uint64_t ss, ts, ms;
    if (n < 16 || memcmp(p, "BPS1", 4)) return E_BPS_MAGIC;
    if (bps_num(p, n, &pos, &ss) || bps_num(p, n, &pos, &ts) || bps_num(p, n, &pos, &ms)) return E_BPS_MAGIC;
    if (ss != tam_origem || ts > cap) return E_BPS_TAM;
    if (crc32(buf, tam_origem) != le32(p + n - 12)) return E_BPS_CRC;
    pos += (uint32_t)ms;
    if (ts > tam_origem) memset(buf + tam_origem, 0, (size_t)(ts - tam_origem));
    uint32_t o = 0; int64_t tco = 0;
    while (pos < n - 12) {
        uint64_t d; if (bps_num(p, n, &pos, &d)) return E_BPS_CMD;
        uint32_t cmd = d & 3, len = (uint32_t)(d >> 2) + 1;
        if (o + len > ts) return E_BPS_CMD;
        if (cmd == 0) {                       /* SourceRead: ja esta no lugar */
            if (o + len > tam_origem) return E_BPS_CMD;
        } else if (cmd == 1) {                /* TargetRead */
            if (pos + len > n - 12) return E_BPS_CMD;
            memcpy(buf + o, p + pos, len); pos += len;
        } else if (cmd == 3) {                /* TargetCopy */
            uint64_t r; if (bps_num(p, n, &pos, &r)) return E_BPS_CMD;
            tco += (r & 1) ? -(int64_t)(r >> 1) : (int64_t)(r >> 1);
            if (tco < 0) return E_BPS_CMD;
            for (uint32_t k = 0; k < len; ++k) buf[o + k] = buf[tco++];
        } else return E_BPS_CMD;              /* SourceCopy: nao gerado por nos */
        o += len;
    }
    if (o != ts || crc32(buf, (uint32_t)ts) != le32(p + n - 8)) return E_BPS_CRC;
    *tam_alvo = (uint32_t)ts;
    return OK;
}

/* ================= receita ================= */
int receita_ler(const uint8_t *p, uint32_t n, Receita *r)
{
    uint32_t o = 0;
#define PRECISA(k) do { if (o + (k) > n) return E_REC_CURTA; } while (0)
    PRECISA(8 + 4 + 16 + 8 + 32 * 6 + 8 + 4);
    if (memcmp(p, "MH3UREC1", 8) || le32(p + 8) != 1) return E_REC_MAGIC;
    o = 12;
    memcpy(r->versao, p + o, 16); r->versao[15] = 0; o += 16;
    r->tam_code_base = le32(p + o); r->tam_code_alvo = le32(p + o + 4); o += 8;
    memcpy(r->sha_code_base, p + o, 32); o += 32;
    memcpy(r->sha_code_alvo, p + o, 32); o += 32;
    memcpy(r->sha_banner, p + o, 32); o += 32;
    memcpy(r->sha_logo, p + o, 32); o += 32;
    memcpy(r->sha_icone, p + o, 32); o += 32;
    memcpy(r->sha_cia, p + o, 32); o += 32;
    r->tam_cia = le32(p + o) | (uint64_t)le32(p + o + 4) << 32; o += 8;
    r->n_patch_icone = le32(p + o); o += 4;
    r->patch_icone = p + o;
    for (uint32_t i = 0; i < r->n_patch_icone; ++i) {
        PRECISA(6); uint32_t l = p[o + 4] | p[o + 5] << 8; o += 6; PRECISA(l); o += l;
    }
    PRECISA(4); r->n_segs = le32(p + o); o += 4;
    r->segs = p + o;
    for (uint32_t i = 0; i < r->n_segs; ++i) {
        PRECISA(5); uint8_t t = p[o]; uint32_t l = le32(p + o + 1); o += 5;
        if (t == 'L') { PRECISA(l); o += l; }
        else if (t != 'C' && t != 'B' && t != 'G' && t != 'I') return E_REC_SEG;
    }
    return OK;
#undef PRECISA
}

void receita_icone(const Receita *r, uint8_t *icone, uint32_t tam)
{
    const uint8_t *q = r->patch_icone;
    for (uint32_t i = 0; i < r->n_patch_icone; ++i) {
        uint32_t off = le32(q), l = q[4] | q[5] << 8;
        if (off + l <= tam) memcpy(icone + off, q + 6, l);
        q += 6 + l;
    }
}

int receita_montar(const Receita *r, const Pedacos *pc, Escritor esc, Progresso prog, void *ctx)
{
    Sha256 s; sha256_ini(&s);
    uint64_t feito = 0;
    const uint8_t *q = r->segs;
    for (uint32_t i = 0; i < r->n_segs; ++i) {
        uint8_t t = q[0]; uint32_t l = le32(q + 1); q += 5;
        const uint8_t *d = NULL;
        if (t == 'L') { d = q; q += l; }
        else if (t == 'C' && l == pc->tam_code)   d = pc->code;
        else if (t == 'B' && l == pc->tam_banner) d = pc->banner;
        else if (t == 'G' && l == pc->tam_logo)   d = pc->logo;
        else if (t == 'I' && l == pc->tam_icone)  d = pc->icone;
        if (!d) return E_REC_SEG;
        for (uint32_t k = 0; k < l; ) {            /* blocos de 256 KB: progresso */
            uint32_t b = l - k > 0x40000 ? 0x40000 : l - k;
            sha256_add(&s, d + k, b);
            if (esc(ctx, d + k, b)) return E_ESCRITA;
            k += b; feito += b;
            if (prog) prog(ctx, feito, r->tam_cia);
        }
    }
    uint8_t h[32]; sha256_fim(&s, h);
    if (feito != r->tam_cia) return E_CIA_TAM;
    if (memcmp(h, r->sha_cia, 32)) return E_CIA_SHA;
    return OK;
}
