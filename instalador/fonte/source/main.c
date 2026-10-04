/*  MH3U Online -- instalador (3DS)
 *
 *  Instala o update do multiplayer online por cima do MH3U (EUA) do PROPRIO
 *  jogador. O app nao traz nada do jogo: le, do console, os arquivos do ExeFS
 *  do MH3U (.code, banner, icon, logo), confere os hashes, aplica o nosso
 *  code.bps e monta o update.cia seguindo a receita (romfs:/receita.bin, feita
 *  por tools/mh3u_receita.py). O resultado e conferido (SHA-256) antes de o AM
 *  concluir a instalacao.
 *
 *  Autoatualizacao (atualiza.h, rede.h): ao abrir, procura no repositorio
 *  publico um manifesto assinado; o patch novo vai p/ sd:/3ds/mh3u-online/ e
 *  passa a ser o instalado pelo A. Se o manifesto trouxer um instalador mais
 *  novo, o proprio app se atualiza.
 *
 *  Visual (citro2d) no desenho do Guild Hunter, com as cores do banner do
 *  app: tela de cima com o status e os comandos; a de baixo com as abas
 *  "Patch atual" e "Versoes", os botoes de toque e o painel de cada operacao.
 *  Textos em pt/en/es pelo idioma do console (textos.h).
 *
 *    A      instalar / atualizar o patch no jogo
 *    B      baixar a atualizacao (ou procurar de novo)
 *    X      convite do servidor (grava `convite=` no sd:/mh3u-online.cfg)
 *    Y      remover o patch (volta ao jogo original)
 *    L / R  trocar de aba      cima/baixo  rolar a tabela de versoes
 *    START  sair
 */
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "nucleo.h"
#include "atualiza.h"
#include "rede.h"
#include "textos.h"
#include "chave_atualizacao.h"

#ifndef COMMIT_INST
#define COMMIT_INST "?"
#endif
#ifndef VERSAO_INST
#define VERSAO_INST "?"
#endif
#define TID_BASE    0x00040000000AE400ULL
#define TID_UPDATE  0x0004000E000AE400ULL
#define PASTA       "sdmc:/3ds/mh3u-online"

#define LARG_TOPO 400
#define LARG_BASE 320
#define ALTURA    240

static void convite_ler(char *out, size_t cap);

/* ----------------------------------------------------------------- cores */
/* as do banner (arte/banner.png): noite, petroleo, laranja, lua, aco */
static u32 COR_NOITE, COR_FUNDO, COR_PAINEL, COR_PAINEL2, COR_TRILHO, COR_LARANJA,
           COR_LUA, COR_VERDE, COR_ERRO, COR_ACO, COR_CLARA;

static void cores(void)
{
    COR_NOITE   = C2D_Color32(0x0A, 0x15, 0x25, 0xFF);   /* ceu do banner */
    COR_FUNDO   = C2D_Color32(0x12, 0x31, 0x47, 0xFF);   /* petroleo */
    COR_PAINEL  = C2D_Color32(0x1B, 0x45, 0x60, 0xFF);
    COR_PAINEL2 = C2D_Color32(0x16, 0x3B, 0x53, 0xFF);
    COR_TRILHO  = C2D_Color32(0x2C, 0x58, 0x73, 0xFF);
    COR_LARANJA = C2D_Color32(0xFB, 0x8C, 0x24, 0xFF);   /* "ONLINE" */
    COR_LUA     = C2D_Color32(0xF0, 0xC4, 0x72, 0xFF);   /* lua e sinal */
    COR_VERDE   = C2D_Color32(0x5F, 0xBF, 0x77, 0xFF);
    COR_ERRO    = C2D_Color32(0xE5, 0x53, 0x4B, 0xFF);
    COR_ACO     = C2D_Color32(0xA0, 0xAC, 0xB6, 0xFF);   /* espada */
    COR_CLARA   = C2D_Color32(0xEE, 0xF1, 0xF2, 0xFF);   /* "MH3U" */
}

/* ---------------------------------------------------------------- estado */
/* o patch que o A instala: o embutido (romfs) ou o baixado (PASTA) */
typedef struct {
    Receita  rec;
    uint8_t *brec, *bps;
    uint32_t nrec, nbps;
    bool     baixado;
} Pacote;

static Pacote    g_pac;
static Manifesto g_man;            /* o manifesto mais novo que conhecemos (assinado) */
static bool      g_tem_man;
static enum { NET_NADA, NET_PROCURANDO, NET_OK, NET_ERRO } g_net;
static char      g_net_erro[48];
static bool      g_app_novo;       /* o instalador se atualizou: reabrir */
static int       g_aba, g_rolar;
static bool      g_tem_base;
static FS_MediaType g_mt = MEDIATYPE_SD;
static char      g_jogo[24];       /* versao do patch no jogo ("" = nao instalado) */
static char      g_recado[112];
static u32       g_cor_recado;
static char      g_ocupado[64];    /* trabalho sem painel (a busca ao abrir) */

/* painel de operacao (tela de baixo) */
#define LOG_MAX 8
static struct {
    bool  ativa;
    int   fim;                     /* 0 andando, 1 ok, -1 falhou */
    char  titulo[48];
    char  etapa[64];
    u64   feito, total;
    char  log[LOG_MAX][72];
    u32   cor[LOG_MAX];
    int   n;
    char  rodape[72];
} g_op;

static enum { MODAL_NADA, MODAL_REMOVER } g_modal;
static int g_modal_foco;

static C3D_RenderTarget *alvo_topo, *alvo_base;
static C2D_TextBuf buf_texto;

/* ------------------------------------------------------------------ util */
static uint8_t *ler_arquivo(const char *caminho, uint32_t *n)
{
    FILE *f = fopen(caminho, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); *n = (uint32_t)ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc(*n ? *n : 1);
    if (b && fread(b, 1, *n, f) != *n) { free(b); b = NULL; }
    fclose(f);
    return b;
}

/* grava em .tmp e troca: um desligamento no meio nao deixa arquivo pela metade */
static bool gravar_arquivo(const char *caminho, const uint8_t *p, uint32_t n)
{
    char tmp[128]; snprintf(tmp, sizeof tmp, "%s.tmp", caminho);
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = fwrite(p, 1, n, f) == n;
    ok = fclose(f) == 0 && ok;
    if (!ok) { remove(tmp); return false; }
    remove(caminho);
    return rename(tmp, caminho) == 0;
}

/*  Um arquivo do ExeFS de OUTRO titulo (o MH3U), pelo archive
 *  SaveDataAndContent (0x2345678A) -- o mesmo caminho que o FBI usa p/ o icone. */
static Result ler_exefs(FS_MediaType mt, const char *nome, uint8_t **buf, uint32_t *n)
{
    u32 arq[4] = { (u32)TID_BASE, (u32)(TID_BASE >> 32), mt, 0 };
    u32 cam[5] = { 0, 0, 2, 0, 0 };
    memcpy(&cam[3], nome, strlen(nome) < 8 ? strlen(nome) : 8);   /* 8 bytes, sem \0 */
    Handle h;
    Result r = FSUSER_OpenFileDirectly(&h, ARCHIVE_SAVEDATA_AND_CONTENT,
                   (FS_Path){ PATH_BINARY, sizeof arq, arq },
                   (FS_Path){ PATH_BINARY, sizeof cam, cam }, FS_OPEN_READ, 0);
    if (R_FAILED(r)) return r;
    u64 tam = 0; FSFILE_GetSize(h, &tam);
    *buf = malloc((size_t)tam); *n = (uint32_t)tam;
    u32 lidos = 0;
    if (!*buf) { FSFILE_Close(h); return -1; }
    r = FSFILE_Read(h, &lidos, 0, *buf, (u32)tam);
    FSFILE_Close(h);
    if (R_SUCCEEDED(r) && lidos != tam) r = -2;
    if (R_FAILED(r)) { free(*buf); *buf = NULL; }
    return r;
}

static bool titulo_em(FS_MediaType mt, u64 tid, u16 *versao)
{
    AM_TitleEntry e;
    if (R_FAILED(AM_GetTitleInfo(mt, 1, &tid, &e))) return false;
    if (versao) *versao = e.version;
    return true;
}

/* versao do titulo (a<<10 | b<<4 | beta, ver VER_TITULO no Makefile) -> "1.0-beta7" */
static void versao_titulo(u16 v, char *out, size_t cap)
{
    if (v & 0xF) snprintf(out, cap, "%u.%u-beta%u", v >> 10, (v >> 4) & 0x3F, v & 0xF);
    else         snprintf(out, cap, "%u.%u", v >> 10, (v >> 4) & 0x3F);
}

/*  Onde esta o MH3U base: instalado no SD ou no cartucho. */
static bool achar_base(FS_MediaType *mt)
{
    if (titulo_em(MEDIATYPE_SD, TID_BASE, NULL)) { *mt = MEDIATYPE_SD; return true; }
    bool tem = false;
    if (R_SUCCEEDED(FSUSER_CardSlotIsInserted(&tem)) && tem) {
        u32 n = 0; u64 ids[4];
        if (R_SUCCEEDED(AM_GetTitleList(&n, MEDIATYPE_GAME_CARD, 4, ids)))
            for (u32 i = 0; i < n; ++i)
                if (ids[i] == TID_BASE) { *mt = MEDIATYPE_GAME_CARD; return true; }
    }
    return false;
}

/* o que mudou no console: jogo, patch instalado */
static void reler_console(void)
{
    u16 v;
    g_tem_base = achar_base(&g_mt);
    g_jogo[0] = 0;
    if (titulo_em(MEDIATYPE_SD, TID_UPDATE, &v)) versao_titulo(v, g_jogo, sizeof g_jogo);
}

/*  Os arquivos do Luma que brigam com o update (o plugin e os patches de
 *  luma/titles): renomeados p/ .off -- nada e apagado. */
static int desligar_luma(void)
{
    static const char *arqs[] = {
        "sdmc:/luma/plugins/00040000000AE400/mh3u-online.3gx",
        "sdmc:/luma/plugins/00040000000AE400/uds-espia.3gx",
        "sdmc:/luma/titles/00040000000AE400/code.bps",
        "sdmc:/luma/titles/00040000000AE400/code.ips",
        "sdmc:/luma/titles/00040000000AE400/code.bin",
        "sdmc:/luma/titles/00040000000AE400/exheader.bin",
    };
    int n = 0;
    for (size_t i = 0; i < sizeof arqs / sizeof *arqs; ++i) {
        char off[128]; snprintf(off, sizeof off, "%s.off", arqs[i]);
        FILE *f = fopen(arqs[i], "rb");
        if (!f) continue;
        fclose(f);
        remove(off);
        if (rename(arqs[i], off) == 0) ++n;
    }
    return n;
}

/* "2026-10-04" -> "04/10/2026" (ingles: "10/04/2026") */
static const char *data_local(const char *d)
{
    static char b[2][16]; static int k;
    k ^= 1;
    if (strlen(d) == 10 && d[4] == '-') {
        if (g_idioma == IDIOMA_EN) snprintf(b[k], sizeof b[k], "%.2s/%.2s/%.4s", d + 5, d + 8, d);
        else                       snprintf(b[k], sizeof b[k], "%.2s/%.2s/%.4s", d + 8, d + 5, d);
    } else snprintf(b[k], sizeof b[k], "%.10s", d);
    return b[k];
}

static const AtuVersao *no_historico(const char *versao)
{
    for (int i = 0; g_tem_man && i < g_man.n_hist; ++i)
        if (!strcmp(g_man.hist[i].versao, versao)) return &g_man.hist[i];
    return NULL;
}

/* ha patch mais novo que o que o app tem? */
static bool tem_novidade(void)
{
    return g_tem_man && versao_numero(g_man.versao) > versao_numero(g_pac.rec.versao);
}

static bool jogo_desatualizado(void)
{
    return g_jogo[0] && versao_numero(g_jogo) < versao_numero(g_pac.rec.versao);
}

/* --------------------------------------------------------------- desenho */
typedef struct { float x, y, l, a; } Caixa;

static float g_z;                  /* profundidade extra (o modal fica por cima) */

static void escrever_(int alinha, float x, float y, float esc, u32 cor, const char *s)
{
    C2D_Text t;
    C2D_TextParse(&t, buf_texto, s);
    C2D_TextOptimize(&t);
    float l = 0, a = 0;
    if (alinha) C2D_TextGetDimensions(&t, esc, esc, &l, &a);
    C2D_DrawText(&t, C2D_WithColor, alinha == 1 ? x - l / 2 : alinha == 2 ? x - l : x, y, 0.5f + g_z, esc, esc, cor);
}

#define FMT(s, fmt) char s[192]; { va_list ap; va_start(ap, fmt); vsnprintf(s, sizeof s, fmt, ap); va_end(ap); }
static void escrever(float x, float y, float esc, u32 cor, const char *fmt, ...)
{ FMT(s, fmt); escrever_(0, x, y, esc, cor, s); }
static void escrever_centro(float x, float y, float esc, u32 cor, const char *fmt, ...)
{ FMT(s, fmt); escrever_(1, x, y, esc, cor, s); }
static void escrever_direita(float x, float y, float esc, u32 cor, const char *fmt, ...)
{ FMT(s, fmt); escrever_(2, x, y, esc, cor, s); }

static float largura(const char *s, float esc)
{
    C2D_Text t; float l = 0, a = 0;
    C2D_TextParse(&t, buf_texto, s);
    C2D_TextGetDimensions(&t, esc, esc, &l, &a);
    return l;
}

static void losango(float cx, float cy, float r, u32 cor)
{
    C2D_DrawTriangle(cx, cy - r, cor, cx - r, cy, cor, cx + r, cy, cor, 0.4f + g_z);
    C2D_DrawTriangle(cx, cy + r, cor, cx - r, cy, cor, cx + r, cy, cor, 0.4f + g_z);
}

static void moldura(Caixa c, u32 fundo, u32 borda, int grossa)
{
    C2D_DrawRectSolid(c.x, c.y, 0.1f + g_z, c.l, c.a, fundo);
    float g = grossa ? 2.5f : 1.0f;
    C2D_DrawLine(c.x, c.y, borda, c.x + c.l, c.y, borda, g, 0.2f + g_z);
    C2D_DrawLine(c.x + c.l, c.y, borda, c.x + c.l, c.y + c.a, borda, g, 0.2f + g_z);
    C2D_DrawLine(c.x + c.l, c.y + c.a, borda, c.x, c.y + c.a, borda, g, 0.2f + g_z);
    C2D_DrawLine(c.x, c.y + c.a, borda, c.x, c.y, borda, g, 0.2f + g_z);
}

/* emblema: o sinal do banner (arcos laranja) sobre o circulo da noite, com a lua */
static void emblema(float cx, float cy, float r)
{
    C2D_DrawCircleSolid(cx, cy, 0.3f, r * 1.15f, COR_FUNDO);
    for (int i = 0; i < 3; ++i) {
        float raio = r * (0.35f + 0.25f * i);
        for (int s = 0; s < 8; ++s) {             /* arco de 90 graus, de cima p/ a direita */
            float a0 = -1.5708f + s * 0.19635f, a1 = a0 + 0.19635f;
            C2D_DrawLine(cx - r * 0.35f + raio * cosf(a0), cy + r * 0.35f + raio * sinf(a0), COR_LARANJA,
                         cx - r * 0.35f + raio * cosf(a1), cy + r * 0.35f + raio * sinf(a1), COR_LARANJA, 2.2f, 0.35f);
        }
    }
    C2D_DrawCircleSolid(cx - r * 0.35f, cy + r * 0.35f, 0.36f, 2.5f, COR_LARANJA);
    C2D_DrawCircleSolid(cx + r * 0.55f, cy - r * 0.55f, 0.36f, r * 0.22f, COR_LUA);
}

/* losango que pulsa: o app esta vivo, quem demora e a rede */
static void pulso(float cx, float cy, u32 cor)
{
    float f = (float)(osGetTime() % 900) / 900.0f;
    losango(cx, cy, 3.0f + 3.0f * (f < 0.5f ? f * 2 : (1 - f) * 2), cor);
}

static void barra_progresso(Caixa c, u64 feito, u64 total)
{
    moldura(c, COR_NOITE, COR_TRILHO, 0);
    if (total) {
        float l = (c.l - 4) * (float)(feito > total ? total : feito) / (float)total;
        C2D_DrawRectSolid(c.x + 2, c.y + 2, 0.3f, l, c.a - 4, COR_LARANJA);
    } else {
        /* ainda sem tamanho: um bloco vai e volta */
        float f = (float)(osGetTime() % 1400) / 1400.0f, w = c.l * 0.25f;
        float x = (c.l - 4 - w) * (f < 0.5f ? f * 2 : (1 - f) * 2);
        C2D_DrawRectSolid(c.x + 2 + x, c.y + 2, 0.3f, w, c.a - 4, COR_LUA);
    }
}

/* uma linha de status: rotulo, losango colorido e valor */
static void status(float y, const char *rotulo, u32 cor, const char *fmt, ...)
{
    FMT(s, fmt);
    escrever(18, y, 0.46f, COR_ACO, "%s", rotulo);
    losango(150, y + 7, 4, cor);
    escrever(162, y, 0.5f, cor == COR_ACO ? COR_CLARA : cor, "%s", s);
}

static void comando(float x, float y, const char *bt, const char *txt, bool vivo)
{
    escrever(x, y - 1, 0.6f, vivo ? COR_LARANJA : COR_TRILHO, "%s", bt);
    escrever(x + 22, y + 1, 0.46f, vivo ? COR_CLARA : COR_TRILHO, "%s", txt);
}

static void desenhar_topo(void)
{
    C2D_TargetClear(alvo_topo, COR_NOITE);
    C2D_SceneBegin(alvo_topo);

    emblema(32, 30, 20);
    escrever(62, 7, 0.85f, COR_CLARA, "MH3U");
    escrever(62 + largura("MH3U ", 0.85f), 7, 0.85f, COR_LARANJA, "ONLINE");
    escrever_direita(LARG_TOPO - 10, 12, 0.44f, COR_LUA, "app %s", VERSAO_INST);
    escrever_direita(LARG_TOPO - 10, 27, 0.36f, COR_ACO, "build %.7s", COMMIT_INST);
    escrever(62, 35, 0.42f, COR_ACO, "%s", T(T_SUBTITULO));
    C2D_DrawLine(12, 56, COR_LARANJA, LARG_TOPO - 12, 56, COR_LARANJA, 2.0f, 0.2f);

    /* status */
    if (g_tem_base) status(62, T(T_JOGO), COR_VERDE, T(T_ENCONTRADO), T(g_mt == MEDIATYPE_SD ? T_CARTAO_SD : T_CARTUCHO));
    else            status(62, T(T_JOGO), COR_ERRO, "%s", T(T_NAO_ENCONTRADO));
    if (!g_jogo[0])                status(78, T(T_PATCH_JOGO), COR_LUA, "%s", T(T_NAO_INSTALADO));
    else if (jogo_desatualizado()) status(78, T(T_PATCH_JOGO), COR_LUA, T(T_DESATUALIZADO), g_jogo);
    else                           status(78, T(T_PATCH_JOGO), COR_VERDE, "%s", g_jogo);
    status(94, T(T_PATCH_APP), COR_ACO, "%s  %s", g_pac.rec.versao, T(g_pac.baixado ? T_BAIXADO : T_VEIO_APP));
    switch (g_net) {
    case NET_NADA:       status(110, T(T_MAIS_NOVA), COR_ACO, "%s", T(T_NAO_PROCURADA)); break;
    case NET_PROCURANDO: status(110, T(T_MAIS_NOVA), COR_ACO, "%s", T(T_PROCURANDO)); break;
    case NET_ERRO:       status(110, T(T_MAIS_NOVA), COR_ERRO, "%s", g_net_erro); break;
    case NET_OK:
        if (tem_novidade()) status(110, T(T_MAIS_NOVA), COR_LUA, T(T_NOVA_DATA), g_man.versao, data_local(g_man.data));
        else                status(110, T(T_MAIS_NOVA), COR_VERDE, T(T_EM_DIA), g_man.versao);
    }
    if (g_app_novo) status(126, T(T_APP), COR_LUA, T(T_APP_REABRA), g_man.versao_inst);
    else if (g_tem_man && versao_numero(g_man.versao_inst) > versao_numero(VERSAO_INST))
        status(126, T(T_APP), COR_LUA, T(T_APP_NOVA), VERSAO_INST, g_man.versao_inst);
    else status(126, T(T_APP), COR_VERDE, "%s", VERSAO_INST);
    char conv[32]; convite_ler(conv, sizeof conv);
    if (conv[0]) status(142, T(T_CONVITE), COR_VERDE, "%.5s****", conv);
    else         status(142, T(T_CONVITE), COR_ACO, "%s", T(T_NENHUM));

    C2D_DrawLine(12, 162, COR_TRILHO, LARG_TOPO - 12, 162, COR_TRILHO, 1.0f, 0.2f);

    /* comandos */
    bool livre = !g_op.ativa && !g_modal;
    int a = !g_jogo[0] ? T_CMD_INSTALAR : jogo_desatualizado() ? T_CMD_ATUALIZAR : T_CMD_REINSTALAR;
    comando(18, 168, BT_A, T(a), livre && g_tem_base);
    comando(18, 187, BT_B, T(tem_novidade() ? T_CMD_BAIXAR : T_CMD_PROCURAR), livre);
    comando(214, 168, BT_X, T(T_CMD_CONVITE), livre);
    comando(214, 187, BT_Y, T(T_CMD_REMOVER), livre && g_jogo[0]);

    C2D_DrawLine(12, 208, COR_TRILHO, LARG_TOPO - 12, 208, COR_TRILHO, 1.0f, 0.2f);
    const char *ocupado = g_op.ativa && !g_op.fim ? (g_op.etapa[0] ? g_op.etapa : g_op.titulo)
                        : g_ocupado[0] ? g_ocupado : NULL;
    if (ocupado) {
        pulso(21, 223, COR_LARANJA);
        escrever(32, 214, 0.48f, COR_LUA, "%s", ocupado);
    } else if (g_recado[0]) {
        escrever(14, 214, 0.44f, g_cor_recado, "%s", g_recado);
    } else {
        escrever(14, 214, 0.42f, COR_ACO, "%s", T(T_RODAPE));
        escrever_direita(LARG_TOPO - 12, 214, 0.4f, COR_TRILHO, "%s", T(T_NADA_JOGO));
    }
}

/* ---- tela de baixo: abas, botoes, painel de operacao, modal ---- */
static const Caixa ABAS[2]     = { { 6, 4, 118, 24 }, { 128, 4, 104, 24 } };
static const Caixa BOTOES[4]   = { { 8, 178, 148, 26 }, { 164, 178, 148, 26 },
                                   { 8, 208, 148, 26 }, { 164, 208, 148, 26 } };
static const Caixa MODAL       = { 36, 64, 248, 112 };
static const Caixa MODAL_BT[2] = { { 52, 132, 100, 28 }, { 168, 132, 100, 28 } };
static const Caixa VOLTAR      = { 70, 200, 180, 30 };

static bool dentro(Caixa c, touchPosition t)
{
    return t.px >= c.x && t.px <= c.x + c.l && t.py >= c.y && t.py <= c.y + c.a;
}

static void aviso(u32 cor, const char *esq, const char *dir)
{
    Caixa c = { 8, 144, LARG_BASE - 16, 28 };
    moldura(c, COR_PAINEL, cor, 1);
    losango(22, 158, 5, cor);
    escrever(32, 150, 0.46f, COR_CLARA, "%s", esq);
    escrever_direita(LARG_BASE - 16, 150, 0.46f, cor, "%s", dir);
}

static void aba_patch(void)
{
    if (!g_jogo[0]) {
        losango(LARG_BASE / 2, 56, 10, COR_TRILHO);
        escrever_centro(LARG_BASE / 2, 74, 0.6f, COR_CLARA, "%s", T(T_SEM_PATCH));
        escrever_centro(LARG_BASE / 2, 98, 0.46f, COR_ACO, "%s", T(T_SEM_PATCH2));
        escrever_centro(LARG_BASE / 2, 114, 0.46f, COR_ACO, T(T_SEM_PATCH3), g_pac.rec.versao);
    } else {
        const AtuVersao *h = no_historico(g_jogo);
        escrever(14, 34, 0.4f, COR_ACO, "%s", T(T_INSTALADO_JOGO));
        escrever(14, 46, 0.8f, COR_CLARA, "%s", g_jogo);
        if (h) escrever_direita(LARG_BASE - 14, 54, 0.46f, COR_ACO, "%s", data_local(h->data));
        C2D_DrawLine(14, 76, COR_TRILHO, LARG_BASE - 14, 76, COR_TRILHO, 1.0f, 0.2f);
        escrever(14, 80, 0.4f, COR_ACO, "%s", T(T_O_QUE_FAZ));
        float y = 94;
        if (g_tem_man && !strcmp(g_man.versao, g_jogo)) {
            int li = manifesto_idioma_mudancas(&g_man, g_idioma);
            for (int i = 0; i < g_man.n_mudancas[li] && y < 140; ++i, y += 15) {
                losango(20, y + 7, 3, COR_LARANJA);
                escrever(28, y, 0.42f, COR_CLARA, "%s", g_man.mudancas[li][i]);
            }
        } else if (h) {
            losango(20, y + 7, 3, COR_LARANJA);
            escrever(28, y, 0.42f, COR_CLARA, "%s", manifesto_resumo(h, g_idioma));
        } else escrever(14, y, 0.42f, COR_ACO, "%s", T(T_CONECTE));
    }
    char t[64];
    if (tem_novidade()) {
        snprintf(t, sizeof t, T(T_NOVA_CAIXA), g_man.versao, data_local(g_man.data));
        aviso(COR_LARANJA, t, T(T_BAIXAR_BT));
    } else if (jogo_desatualizado()) {
        snprintf(t, sizeof t, T(T_APP_TEM), g_pac.rec.versao);
        aviso(COR_LUA, t, T(T_ATUALIZAR_BT));
    }
}

static void aba_versoes(void)
{
    if (!g_tem_man || !g_man.n_hist) {
        losango(LARG_BASE / 2, 56, 10, COR_TRILHO);
        escrever_centro(LARG_BASE / 2, 74, 0.55f, COR_CLARA, "%s", T(T_SEM_HIST));
        escrever_centro(LARG_BASE / 2, 98, 0.44f, COR_ACO, "%s", T(T_SEM_HIST2));
        escrever_centro(LARG_BASE / 2, 112, 0.44f, COR_ACO, "%s", T(T_SEM_HIST3));
        return;
    }
    C2D_DrawRectSolid(8, 32, 0.1f, LARG_BASE - 16, 16, COR_NOITE);
    escrever(28, 33, 0.42f, COR_ACO, "%s", T(T_COL_VERSAO));
    escrever(130, 33, 0.42f, COR_ACO, "%s", T(T_COL_DATA));
    escrever(214, 33, 0.42f, COR_ACO, "%s", T(T_COL_SITUACAO));
    const int VIS = 4, ALT = 31;
    if (g_rolar > g_man.n_hist - VIS) g_rolar = g_man.n_hist - VIS;
    if (g_rolar < 0) g_rolar = 0;
    for (int v = 0; v < VIS && g_rolar + v < g_man.n_hist; ++v) {
        int i = g_rolar + v;
        const AtuVersao *h = &g_man.hist[i];
        bool no_jogo = g_jogo[0] && !strcmp(h->versao, g_jogo);
        bool no_app = !strcmp(h->versao, g_pac.rec.versao);
        float y = 49 + v * ALT;
        C2D_DrawRectSolid(8, y, 0.1f, LARG_BASE - 16, ALT - 1, (i & 1) ? COR_PAINEL2 : COR_PAINEL);
        u32 cor = no_jogo ? COR_VERDE : i == 0 ? COR_LARANJA : COR_TRILHO;
        losango(18, y + 9, 4, cor);
        escrever(28, y + 1, 0.5f, COR_CLARA, "%s", h->versao);
        escrever(130, y + 2, 0.44f, COR_CLARA, "%s", data_local(h->data));
        escrever(214, y + 2, 0.42f, no_jogo ? COR_VERDE : i == 0 ? COR_LARANJA : COR_ACO, "%s",
                 T(no_jogo ? T_SIT_JOGO : i == 0 ? T_SIT_NOVA : no_app ? T_SIT_APP : T_SIT_ANTERIOR));
        escrever(28, y + 16, 0.38f, COR_ACO, "%s", manifesto_resumo(h, g_idioma));
    }
    if (g_man.n_hist > VIS)
        escrever_direita(LARG_BASE - 10, 160, 0.38f, COR_ACO, T(T_ROLAR),
                         g_rolar + 1, g_rolar + VIS > g_man.n_hist ? g_man.n_hist : g_rolar + VIS, g_man.n_hist);
}

static void painel_operacao(void)
{
    u32 faixa = g_op.fim > 0 ? COR_VERDE : g_op.fim < 0 ? COR_ERRO : COR_NOITE;
    C2D_DrawRectSolid(0, 0, 0.1f, LARG_BASE, 28, faixa);
    escrever(10, 5, 0.55f, g_op.fim ? COR_NOITE : COR_CLARA, "%s", g_op.titulo);
    if (!g_op.fim) pulso(LARG_BASE - 16, 14, COR_LARANJA);

    for (int i = 0; i < g_op.n; ++i)
        escrever(12, 34 + i * 15, 0.43f, g_op.cor[i], "%s", g_op.log[i]);

    if (!g_op.fim) {
        escrever(12, 170, 0.44f, COR_LUA, "%s", g_op.etapa);
        barra_progresso((Caixa){ 10, 188, LARG_BASE - 20, 18 }, g_op.feito, g_op.total);
        if (g_op.total)
            escrever_direita(LARG_BASE - 10, 212, 0.42f, COR_ACO, "%d%%", (int)(g_op.feito * 100 / g_op.total));
        if (g_op.total >= 4096)
            escrever(12, 212, 0.42f, COR_ACO, T(T_KB), (unsigned long)(g_op.feito / 1024), (unsigned long)(g_op.total / 1024));
    } else {
        if (g_op.rodape[0]) escrever_centro(LARG_BASE / 2, 176, 0.46f, COR_CLARA, "%s", g_op.rodape);
        moldura(VOLTAR, COR_LARANJA, COR_LUA, 1);
        escrever_centro(LARG_BASE / 2, VOLTAR.y + 7, 0.55f, COR_NOITE, "%s", T(T_VOLTAR));
    }
}

static void desenhar_modal(void)
{
    g_z = 0.3f;
    C2D_DrawRectSolid(0, 0, 0.3f, LARG_BASE, ALTURA, C2D_Color32(0x05, 0x0B, 0x14, 0xD8));
    C2D_DrawRectSolid(MODAL.x + 4, MODAL.y + 5, 0.32f, MODAL.l, MODAL.a, C2D_Color32(0, 0, 0, 0x80));
    moldura(MODAL, COR_FUNDO, COR_LARANJA, 1);
    escrever_centro(LARG_BASE / 2, MODAL.y + 16, 0.6f, COR_CLARA, "%s", T(T_Q_REMOVER));
    escrever_centro(LARG_BASE / 2, MODAL.y + 42, 0.44f, COR_ACO, "%s", T(T_Q_REMOVER2));
    for (int b = 0; b < 2; ++b) {
        bool m = g_modal_foco == b;
        moldura(MODAL_BT[b], m ? COR_LARANJA : COR_PAINEL, m ? COR_LUA : COR_TRILHO, m);
        escrever_centro(MODAL_BT[b].x + MODAL_BT[b].l / 2, MODAL_BT[b].y + 7, 0.5f, m ? COR_NOITE : COR_CLARA,
                        "%s", T(b == 0 ? T_REMOVER_BT : T_CANCELAR_BT));
    }
    g_z = 0;
}

static void desenhar_base(void)
{
    C2D_TargetClear(alvo_base, COR_FUNDO);
    C2D_SceneBegin(alvo_base);
    if (g_op.ativa) { painel_operacao(); return; }

    C2D_DrawRectSolid(0, 0, 0.1f, LARG_BASE, 28, COR_NOITE);
    for (int i = 0; i < 2; ++i) {
        bool m = g_aba == i;
        if (m) {
            C2D_DrawRectSolid(ABAS[i].x, ABAS[i].y, 0.15f, ABAS[i].l, ABAS[i].a, COR_FUNDO);
            C2D_DrawRectSolid(ABAS[i].x, ABAS[i].y, 0.16f, ABAS[i].l, 2, COR_LARANJA);
        }
        escrever_centro(ABAS[i].x + ABAS[i].l / 2, ABAS[i].y + 5, 0.5f, m ? COR_CLARA : COR_ACO,
                        "%s", T(i == 0 ? T_ABA_PATCH : T_ABA_VERSOES));
    }
    escrever_direita(LARG_BASE - 8, 8, 0.44f, COR_ACO, BT_L " " BT_R);

    if (g_aba == 0) aba_patch(); else aba_versoes();

    int rot[4] = { !g_jogo[0] ? T_BT_INSTALAR : jogo_desatualizado() ? T_BT_ATUALIZAR : T_BT_REINSTALAR,
                   tem_novidade() ? T_BT_BAIXAR : T_BT_PROCURAR, T_BT_CONVITE, T_BT_REMOVER };
    bool vivo[4] = { g_tem_base, true, true, g_jogo[0] != 0 };
    bool destaque[4] = { g_tem_base && (!g_jogo[0] || jogo_desatualizado()), tem_novidade(), false, false };
    for (int b = 0; b < 4; ++b) {
        moldura(BOTOES[b], destaque[b] ? COR_LARANJA : COR_PAINEL, destaque[b] ? COR_LUA : COR_TRILHO, destaque[b]);
        escrever_centro(BOTOES[b].x + BOTOES[b].l / 2, BOTOES[b].y + 6, 0.5f,
                        !vivo[b] ? COR_TRILHO : destaque[b] ? COR_NOITE : COR_CLARA, "%s", T(rot[b]));
    }
    if (g_modal) desenhar_modal();
}

static void desenhar(void)
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(buf_texto);
    desenhar_topo();
    desenhar_base();
    C3D_FrameEnd(0);
}

/* ---- painel de operacao: quem trabalha so chama estas ---- */
static void op_inicio(const char *titulo)
{
    memset(&g_op, 0, sizeof g_op);
    g_op.ativa = true;
    snprintf(g_op.titulo, sizeof g_op.titulo, "%s", titulo);
    g_recado[0] = 0;
    desenhar();
}

static void op_linha(u32 cor, const char *fmt, ...)
{
    FMT(s, fmt);
    if (g_op.n == LOG_MAX) {
        memmove(g_op.log, g_op.log + 1, sizeof g_op.log[0] * (LOG_MAX - 1));
        memmove(g_op.cor, g_op.cor + 1, sizeof g_op.cor[0] * (LOG_MAX - 1));
        g_op.n--;
    }
    snprintf(g_op.log[g_op.n], sizeof g_op.log[0], "%.71s", s);
    g_op.cor[g_op.n++] = cor;
    desenhar();
}

static void op_etapa(const char *etapa, u64 feito, u64 total)
{
    snprintf(g_op.etapa, sizeof g_op.etapa, "%s", etapa);
    g_op.feito = feito; g_op.total = total;
    desenhar();
}

static void op_fim(bool ok, const char *titulo, const char *rodape)
{
    g_op.fim = ok ? 1 : -1;
    snprintf(g_op.titulo, sizeof g_op.titulo, "%s", titulo);
    snprintf(g_op.rodape, sizeof g_op.rodape, "%s", rodape ? rodape : "");
    g_op.etapa[0] = 0;
    reler_console();
    desenhar();
}

static void prog_rede(void *ctx, const char *etapa, uint32_t feito, uint32_t total)
{
    char t[64]; snprintf(t, sizeof t, "%s: %s", (const char *)ctx, etapa);
    if (g_op.ativa) op_etapa(t, feito, total);
    else { snprintf(g_ocupado, sizeof g_ocupado, "%s", t); desenhar(); }
}

/* ------------------------------------------------------ autoatualizacao */
static void pacote_livre(Pacote *p) { free(p->brec); free(p->bps); memset(p, 0, sizeof *p); }

static bool pacote_embutido(Pacote *p)
{
    memset(p, 0, sizeof *p);
    p->brec = ler_arquivo("romfs:/receita.bin", &p->nrec);
    p->bps = ler_arquivo("romfs:/mh3u.bps", &p->nbps);
    if (!p->brec || !p->bps || receita_ler(p->brec, p->nrec, &p->rec)) { pacote_livre(p); return false; }
    return true;
}

static bool sha_ok(const uint8_t *p, uint32_t n, const AtuArquivo *a)
{
    uint8_t h[32];
    if (!a || n != a->tam) return false;
    sha256(p, n, h);
    return memcmp(h, a->sha, 32) == 0;
}

/* confere assinatura e le o manifesto */
static bool manifesto_valido(const uint8_t *txt, uint32_t nt, const uint8_t *sig, uint32_t ns, Manifesto *m)
{
    uint8_t h[32];
    if (!txt || !sig || ns != 256) return false;
    sha256(txt, nt, h);
    return rsa2048_verificar(CHAVE_ATUALIZACAO, sig, h) == 0 && manifesto_ler((const char *)txt, nt, m) == 0;
}

/*  O que ficou no cartao de vezes anteriores:
 *    versao.txt/.sig   o ultimo manifesto visto (historico, mesmo sem baixar)
 *    pacote.txt/.sig   o manifesto do patch baixado + receita.bin + mh3u.bps  */
static void carregar_cache(void)
{
    uint32_t nt = 0, ns = 0;
    uint8_t *txt = ler_arquivo(PASTA "/versao.txt", &nt), *sig = ler_arquivo(PASTA "/versao.sig", &ns);
    Manifesto m;
    if (manifesto_valido(txt, nt, sig, ns, &m)) { g_man = m; g_tem_man = true; }
    free(txt); free(sig);

    txt = ler_arquivo(PASTA "/pacote.txt", &nt); sig = ler_arquivo(PASTA "/pacote.sig", &ns);
    bool ok = manifesto_valido(txt, nt, sig, ns, &m);
    free(txt); free(sig);
    if (!ok || versao_numero(m.versao) <= versao_numero(g_pac.rec.versao)) return;
    Pacote p; memset(&p, 0, sizeof p);
    p.brec = ler_arquivo(PASTA "/receita.bin", &p.nrec);
    p.bps = ler_arquivo(PASTA "/mh3u.bps", &p.nbps);
    if (p.brec && p.bps && sha_ok(p.brec, p.nrec, manifesto_arquivo(&m, "receita.bin")) &&
        sha_ok(p.bps, p.nbps, manifesto_arquivo(&m, "mh3u.bps")) && !receita_ler(p.brec, p.nrec, &p.rec)) {
        p.baixado = true;
        pacote_livre(&g_pac); g_pac = p;
        if (!g_tem_man || versao_numero(m.versao) > versao_numero(g_man.versao)) { g_man = m; g_tem_man = true; }
    } else pacote_livre(&p);
}

static void procurar(void)
{
    g_net = NET_PROCURANDO;
    snprintf(g_ocupado, sizeof g_ocupado, "%s", T(T_PROCURANDO_ATU));
    desenhar();
    uint8_t *txt = NULL, *sig = NULL; uint32_t nt = 0, ns = 0;
    const char *rot = T(T_ATUALIZACAO);
    int e = rede_baixar("main/atualizacao/versao.txt", 16 * 1024, &txt, &nt, prog_rede, (void *)rot);
    if (!e) e = rede_baixar("main/atualizacao/versao.sig", 256, &sig, &ns, prog_rede, (void *)rot);
    Manifesto m;
    if (e) { g_net = NET_ERRO; snprintf(g_net_erro, sizeof g_net_erro, "%s", rede_erro(e)); }
    else if (!manifesto_valido(txt, nt, sig, ns, &m)) { g_net = NET_ERRO; snprintf(g_net_erro, sizeof g_net_erro, "%s", T(T_MANIF_INVALIDO)); }
    else {
        g_net = NET_OK;
        /* nunca volta atras: um manifesto mais velho (cache de CDN) nao substitui o que ja vimos */
        if (!g_tem_man || versao_numero(m.versao) >= versao_numero(g_man.versao)) {
            g_man = m; g_tem_man = true;
            mkdir("sdmc:/3ds", 0777); mkdir(PASTA, 0777);
            gravar_arquivo(PASTA "/versao.txt", txt, nt);
            gravar_arquivo(PASTA "/versao.sig", sig, ns);
        }
    }
    free(txt); free(sig);
    g_ocupado[0] = 0;
    if (g_net == NET_OK && tem_novidade()) {
        snprintf(g_recado, sizeof g_recado, T(T_RECADO_NOVA), g_man.versao);
        g_cor_recado = COR_LUA;
    }
}

static int baixar_conferindo(const AtuArquivo *a, const char *rotulo, uint8_t **out)
{
    uint32_t n = 0; *out = NULL;
    int e = rede_baixar(a->caminho, a->tam, out, &n, prog_rede, (void *)rotulo);
    if (e) { op_linha(COR_ERRO, "%s: %s", rotulo, rede_erro(e)); return e; }
    op_etapa(T(T_CONFERINDO_SHA), n, a->tam);
    if (!sha_ok(*out, n, a)) { op_linha(COR_ERRO, T(T_DIFERENTE_ASSINADO), rotulo); free(*out); *out = NULL; return -1; }
    op_linha(COR_VERDE, T(T_OK_KB), rotulo, (unsigned long)(n + 1023) / 1024);
    return 0;
}

/* o proprio instalador, pelo AM (como o FBI se atualiza) */
static bool instalar_app(const uint8_t *cia, uint32_t n)
{
    Handle h;
    if (R_FAILED(AM_StartCiaInstall(MEDIATYPE_SD, &h))) return false;
    for (uint32_t off = 0; off < n; ) {
        u32 k = n - off > 0x10000 ? 0x10000 : n - off, w = 0;
        if (R_FAILED(FSFILE_Write(h, &w, off, cia + off, k, 0)) || w != k) { AM_CancelCIAInstall(h); return false; }
        off += k;
        op_etapa(T(T_INSTALANDO_APP), off, n);
    }
    return R_SUCCEEDED(AM_FinishCiaInstall(h));
}

static void baixar_atualizacao(void)
{
    Manifesto m = g_man;
    char t[48]; snprintf(t, sizeof t, T(T_BAIXANDO_PATCH), m.versao);
    op_inicio(t);
    op_linha(COR_ACO, "%s", T(T_DO_REPO));
    op_linha(COR_ACO, "%s", T(T_TUDO_CONFERIDO));

    uint8_t *rec = NULL, *bps = NULL, *cia = NULL;
    const AtuArquivo *arec = manifesto_arquivo(&m, "receita.bin"), *abps = manifesto_arquivo(&m, "mh3u.bps");
    bool ok = arec && abps && !baixar_conferindo(arec, T(T_RECEITA), &rec) && !baixar_conferindo(abps, T(T_PATCH), &bps);
    Pacote p; memset(&p, 0, sizeof p);
    if (ok) {
        p.brec = rec; p.nrec = arec->tam; p.bps = bps; p.nbps = abps->tam; p.baixado = true;
        rec = bps = NULL;
        if (receita_ler(p.brec, p.nrec, &p.rec)) { op_linha(COR_ERRO, "%s", T(T_RECEITA_INVALIDA)); ok = false; }
    }
    if (ok) {
        /* guarda no cartao: o pacote vale tambem nas proximas vezes, sem internet */
        op_etapa(T(T_GRAVANDO_SD), 0, 0);
        uint32_t nt = 0, ns = 0;
        uint8_t *txt = ler_arquivo(PASTA "/versao.txt", &nt), *sig = ler_arquivo(PASTA "/versao.sig", &ns);
        bool gravou = txt && sig && gravar_arquivo(PASTA "/receita.bin", p.brec, p.nrec) &&
                      gravar_arquivo(PASTA "/mh3u.bps", p.bps, p.nbps) &&
                      gravar_arquivo(PASTA "/pacote.txt", txt, nt) && gravar_arquivo(PASTA "/pacote.sig", sig, ns);
        free(txt); free(sig);
        if (gravou) op_linha(COR_VERDE, "%s", T(T_GUARDADO));
        else op_linha(COR_LUA, "%s", T(T_NAO_GRAVEI));
        pacote_livre(&g_pac); g_pac = p; memset(&p, 0, sizeof p);
    }
    pacote_livre(&p); free(rec); free(bps);

    const AtuArquivo *acia = manifesto_arquivo(&m, "instalador.cia");
    if (ok && acia && versao_numero(m.versao_inst) > versao_numero(VERSAO_INST)) {
        if (!baixar_conferindo(acia, T(T_APP_NOVO), &cia)) {
            if (instalar_app(cia, acia->tam)) { g_app_novo = true; op_linha(COR_VERDE, T(T_APP_ATUALIZADO), m.versao_inst); }
            else op_linha(COR_ERRO, "%s", T(T_APP_FALHOU));
        }
        free(cia);
    }
    if (!ok) { op_fim(false, T(T_FALHOU_TROCADO), T(T_TENTE)); return; }
    op_fim(true, T(T_PATCH_BAIXADO), T(g_app_novo ? T_FECHE_ABRA : T_AGORA_INSTALE));
}

/* ------------------------------------------------------------ instalar */
typedef struct { Handle cia; u64 off; } Destino;

static int escrever_cia(void *ctx, const uint8_t *p, uint32_t n)
{
    Destino *d = ctx; u32 escritos = 0;
    Result r = FSFILE_Write(d->cia, &escritos, d->off, p, n, 0);
    if (R_FAILED(r) || escritos != n) return -1;
    d->off += n;
    return 0;
}

static void progresso(void *ctx, uint64_t feito, uint64_t total)
{
    (void)ctx;
    static u64 ultimo;
    if (osGetTime() - ultimo < 50 && feito < total) return;   /* desenhar a cada bloco atrasa a gravacao */
    ultimo = osGetTime();
    op_etapa(T(T_GRAVANDO_UPDATE), feito, total);
}

/* arq: nome curto (ASCII) do arquivo de diagnostico, se nao bater */
static bool conferir(const char *nome, const char *arq, const uint8_t *p, uint32_t n, const uint8_t *esperado)
{
    uint8_t h[32]; sha256(p, n, h);
    if (!memcmp(h, esperado, 32)) { op_linha(COR_VERDE, T(T_ITEM_OK), nome); return true; }
    /* p/ diagnostico: o que foi lido vai p/ o cartao */
    char cam[64]; snprintf(cam, sizeof cam, "sdmc:/mh3u-online-%s.bin", arq);
    FILE *f = fopen(cam, "wb");
    if (f) { fwrite(p, 1, n, f); fclose(f); }
    op_linha(COR_ERRO, T(T_ITEM_DIFERENTE), nome, cam + 5);
    return false;
}

static void instalar(void)
{
    const Receita *rec = &g_pac.rec;
    uint8_t *comp = NULL, *ban = NULL, *logo = NULL, *ic = NULL, *code = NULL;
    uint32_t nc = 0, nban = 0, nlogo = 0, nic = 0;
    Result r;
    int e;
    char t[48]; snprintf(t, sizeof t, T(T_INSTALANDO), rec->versao);
    op_inicio(t);
    op_linha(COR_LUA, "%s", T(T_NAO_DESLIGUE));

    op_etapa(T(g_mt == MEDIATYPE_SD ? T_LENDO_SD : T_LENDO_CART), 1, 6);
    if (R_FAILED(r = ler_exefs(g_mt, ".code", &comp, &nc)) ||
        R_FAILED(r = ler_exefs(g_mt, "banner", &ban, &nban)) ||
        R_FAILED(r = ler_exefs(g_mt, "icon", &ic, &nic))) {
        op_linha(COR_ERRO, T(T_NAO_LI), r);
        goto falhou;
    }

    uint32_t td = blz_tamanho(comp, nc);
    if (td != rec->tam_code_base) {
        op_linha(COR_ERRO, "%s", T(T_NAO_SUPORTADA));
        op_linha(COR_ERRO, "(EUA/USA, 00040000000AE400, v1.0).");
        goto falhou;
    }
    code = malloc(rec->tam_code_alvo > td ? rec->tam_code_alvo : td);
    if (!code) { op_linha(COR_ERRO, "%s", T(T_SEM_MEMORIA)); goto falhou; }
    op_etapa(T(T_DESCOMPRIMINDO), 2, 6);
    if ((e = blz_descomprimir(comp, nc, code, td))) { op_linha(COR_ERRO, "%s", nucleo_erro(e)); goto falhou; }
    free(comp); comp = NULL;

    /*  SMDH 0x2018 = bloqueio de regiao. O Azahar entrega 0x7FFFFFFF (ele
     *  libera a regiao na leitura); o console, o original do MH3U EUA (2).
     *  Normaliza p/ o original -- e o que vai no update. */
    if (nic >= 0x201C) { static const uint8_t eua[4] = { 2, 0, 0, 0 }; memcpy(ic + 0x2018, eua, 4); }

    op_etapa(T(T_CONFERINDO_JOGO), 3, 6);
    if (!conferir(T(T_COD_JOGO), "codigo_do_jogo", code, td, rec->sha_code_base) ||
        !conferir(T(T_BANNER), "banner", ban, nban, rec->sha_banner) ||
        !conferir(T(T_ICONE), "icone", ic, nic, rec->sha_icone)) {
        op_linha(COR_ERRO, "%s", T(T_JOGO_ERRADO));
        goto falhou;
    }

    op_etapa(T(T_APLICANDO), 4, 6);
    uint32_t ta = 0;
    if ((e = bps_aplicar_no_lugar(g_pac.bps, g_pac.nbps, code, td, rec->tam_code_alvo, &ta))) {
        op_linha(COR_ERRO, "%s", nucleo_erro(e)); goto falhou;
    }
    if (!conferir(T(T_COD_PATCH), "codigo_com_o_patch", code, ta, rec->sha_code_alvo)) goto falhou;
    receita_icone(rec, ic, nic);

    Destino d = { 0, 0 };
    if (R_FAILED(r = AM_StartCiaInstall(MEDIATYPE_SD, &d.cia))) {
        op_linha(COR_ERRO, "AM_StartCiaInstall: 0x%08lX", r); goto falhou;
    }
    Pedacos pc = { code, ta, ban, nban, logo, nlogo, ic, nic };
    e = receita_montar(rec, &pc, escrever_cia, progresso, &d);
    if (e) {
        AM_CancelCIAInstall(d.cia);
        op_linha(COR_ERRO, T(T_CANCELADA), nucleo_erro(e));
        goto falhou;
    }
    if (R_FAILED(r = AM_FinishCiaInstall(d.cia))) {
        op_linha(COR_ERRO, "AM_FinishCiaInstall: 0x%08lX", r); goto falhou;
    }
    free(ban); free(ic); free(code);
    op_linha(COR_VERDE, T(T_INSTALADO), rec->versao);
    int n = desligar_luma();
    if (n) op_linha(COR_ACO, T(T_LUMA_OFF), n);
    op_linha(COR_CLARA, "%s", T(T_ABRA));
    op_linha(COR_ACO, "%s", T(T_OLD3DS));
    { char c[32]; convite_ler(c, sizeof c);
      if (!c[0]) op_linha(COR_LUA, "%s", T(T_SEM_CONVITE)); }
    op_fim(true, T(T_CONCLUIDO), NULL);
    return;

falhou:
    free(comp); free(ban); free(logo); free(ic); free(code);
    op_fim(false, T(T_FALHOU_INST), NULL);
}

static void desinstalar(void)
{
    op_inicio(T(T_REMOVENDO));
    Result r = AM_DeleteTitle(MEDIATYPE_SD, TID_UPDATE);
    if (R_SUCCEEDED(r)) { op_linha(COR_VERDE, "%s", T(T_VOLTOU_ORIGINAL)); op_fim(true, T(T_REMOVIDO), NULL); }
    else { op_linha(COR_ERRO, T(T_NAO_REMOVIDO_COD), r); op_fim(false, T(T_NAO_REMOVIDO), NULL); }
}

/* --------------------------------------------------------------- convite */
/*  O servidor do MH3U e trancado por IP. O convite (gerado pelo dono do
 *  servidor: ./mhxx convite novo APELIDO) fica no sd:/mh3u-online.cfg; o jogo
 *  le e manda a "batida" que libera o IP do console (tools/mh3u_convite.py). */
#define CFG "sdmc:/mh3u-online.cfg"
static const char ALFABETO[] = "23456789ABCDEFGHJKMNPQRSTUVWXYZ";

static void convite_ler(char *out, size_t cap)
{
    out[0] = 0;
    FILE *f = fopen(CFG, "r");
    if (!f) return;
    char ln[128];
    while (fgets(ln, sizeof ln, f))
        if (!strncmp(ln, "convite=", 8)) {
            size_t n = 0;
            for (const char *p = ln + 8; *p && *p != '\r' && *p != '\n' && n < cap - 1; p++) out[n++] = *p;
            out[n] = 0;
        }
    fclose(f);
}

/* troca (ou acrescenta) a linha convite=, sem tocar nas outras (servidor= etc.) */
static bool convite_gravar(const char *cod)
{
    char resto[1024]; size_t nr = 0;
    FILE *f = fopen(CFG, "r");
    if (f) {
        char ln[128];
        while (fgets(ln, sizeof ln, f))
            if (strncmp(ln, "convite=", 8)) {
                size_t k = strlen(ln);
                if (nr + k + 1 < sizeof resto) { memcpy(resto + nr, ln, k); nr += k; }
            }
        fclose(f);
    }
    if (nr && resto[nr - 1] != '\n') resto[nr++] = '\n';
    f = fopen(CFG, "w");
    if (!f) return false;
    fwrite(resto, 1, nr, f);
    fprintf(f, "convite=%s\n", cod);
    fclose(f);
    return true;
}

/* so letras/numeros, maiusculo; 8 do alfabeto do convite -> "XXXX-XXXX" */
static bool convite_normalizar(const char *in, char out[10])
{
    char c8[9]; int n = 0;
    for (; *in; in++) {
        char c = *in;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c == '-' || c == ' ') continue;
        if (n >= 8 || !strchr(ALFABETO, c)) return false;
        c8[n++] = c;
    }
    if (n != 8) return false;
    memcpy(out, c8, 4); out[4] = '-'; memcpy(out + 5, c8 + 4, 4); out[9] = 0;
    return true;
}

static void convite_digitar(void)
{
    char atual[32]; convite_ler(atual, sizeof atual);
    SwkbdState kb;
    swkbdInit(&kb, SWKBD_TYPE_QWERTY, 2, 12);
    swkbdSetHintText(&kb, T(T_CONVITE_DICA));
    swkbdSetValidation(&kb, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    if (atual[0]) swkbdSetInitialText(&kb, atual);
    char buf[32] = {0};
    if (swkbdInputText(&kb, buf, sizeof buf) != SWKBD_BUTTON_CONFIRM) return;
    char cod[10];
    if (!convite_normalizar(buf, cod)) {
        snprintf(g_recado, sizeof g_recado, "%s", T(T_CONVITE_INVALIDO)); g_cor_recado = COR_ERRO;
    } else if (convite_gravar(cod)) {
        snprintf(g_recado, sizeof g_recado, "%s", T(T_CONVITE_GRAVADO)); g_cor_recado = COR_VERDE;
    } else {
        snprintf(g_recado, sizeof g_recado, "%s", T(T_CONVITE_NAO_GRAVOU)); g_cor_recado = COR_ERRO;
    }
}

/* ------------------------------------------------------------------ main */
enum { ACAO_NADA, ACAO_INSTALAR, ACAO_BAIXAR, ACAO_CONVITE, ACAO_REMOVER };

static void executar(int acao)
{
    switch (acao) {
    case ACAO_INSTALAR:
        if (g_tem_base) instalar();
        else { snprintf(g_recado, sizeof g_recado, "%s", T(T_SEM_JOGO)); g_cor_recado = COR_ERRO; }
        break;
    case ACAO_BAIXAR:
        if (tem_novidade()) baixar_atualizacao();
        else {
            g_recado[0] = 0; procurar();
            if (g_net == NET_OK && !tem_novidade()) {
                snprintf(g_recado, sizeof g_recado, T(T_EM_DIA_RECADO), g_pac.rec.versao);
                g_cor_recado = COR_VERDE;
            }
        }
        break;
    case ACAO_CONVITE: convite_digitar(); break;
    case ACAO_REMOVER: if (g_jogo[0]) { g_modal = MODAL_REMOVER; g_modal_foco = 1; } break;
    }
}

#ifdef PILOTO_AUTOMATICO
static void esperar_desenhando(u64 ms)
{
    for (u64 t = osGetTime(); osGetTime() - t < ms && aptMainLoop(); ) desenhar();
}
#endif

int main(void)
{
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    alvo_topo = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    alvo_base = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    buf_texto = C2D_TextBufNew(4096);
    cores();
    idioma_escolher();
    amInit();
    romfsInit();

    bool ok = pacote_embutido(&g_pac);
    reler_console();
    if (ok) {
        carregar_cache();
        desenhar();
        procurar();
#ifdef PILOTO_AUTOMATICO
        /* teste no Azahar: baixar, voltar, aba Versoes */
        esperar_desenhando(4000);
        if (tem_novidade()) baixar_atualizacao();
        esperar_desenhando(6000);
        g_op.ativa = false;
        esperar_desenhando(4000);
        g_aba = 1;
#endif
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 k = hidKeysDown();
        touchPosition toque; hidTouchRead(&toque);
        bool tocou = (k & KEY_TOUCH) != 0;
        if (k & KEY_START) break;

        if (!ok) {
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            C2D_TextBufClear(buf_texto);
            C2D_TargetClear(alvo_topo, COR_NOITE); C2D_SceneBegin(alvo_topo);
            escrever(16, 100, 0.6f, COR_ERRO, "%s", T(T_CORROMPIDO));
            escrever(16, 124, 0.5f, COR_ACO, "%s", T(T_CORROMPIDO2));
            C2D_TargetClear(alvo_base, COR_FUNDO); C2D_SceneBegin(alvo_base);
            C3D_FrameEnd(0);
            continue;
        }

        if (g_op.ativa) {
            if (g_op.fim && ((k & (KEY_B | KEY_A)) || (tocou && dentro(VOLTAR, toque)))) g_op.ativa = false;
        } else if (g_modal) {
            if (k & (KEY_DLEFT | KEY_DRIGHT)) g_modal_foco ^= 1;
            int esc = -1;
            if (k & KEY_A) esc = g_modal_foco;
            if (k & KEY_B) esc = 1;
            if (tocou) for (int b = 0; b < 2; ++b) if (dentro(MODAL_BT[b], toque)) esc = b;
            if (esc >= 0) { g_modal = MODAL_NADA; if (esc == 0) desinstalar(); }
        } else {
            int acao = ACAO_NADA;
            if (k & KEY_A) acao = ACAO_INSTALAR;
            else if (k & KEY_B) acao = ACAO_BAIXAR;
            else if (k & KEY_X) acao = ACAO_CONVITE;
            else if (k & KEY_Y) acao = ACAO_REMOVER;
            else if (k & (KEY_L | KEY_R | KEY_DLEFT | KEY_DRIGHT)) g_aba ^= 1;
            else if (g_aba == 1 && (k & (KEY_DUP | KEY_DDOWN))) g_rolar += (k & KEY_DDOWN) ? 1 : -1;
            if (tocou) {
                for (int i = 0; i < 2; ++i) if (dentro(ABAS[i], toque)) g_aba = i;
                static const int ACAO_BT[4] = { ACAO_INSTALAR, ACAO_BAIXAR, ACAO_CONVITE, ACAO_REMOVER };
                for (int b = 0; b < 4; ++b) if (dentro(BOTOES[b], toque)) acao = ACAO_BT[b];
            }
            executar(acao);
        }
        desenhar();
    }

    rede_fim();
    pacote_livre(&g_pac);
    C2D_TextBufDelete(buf_texto);
    C2D_Fini(); C3D_Fini();
    romfsExit(); amExit(); gfxExit();
    return 0;
}
