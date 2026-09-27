/*  MH3U Online -- instalador (3DS)
 *
 *  Instala o update do multiplayer online por cima do MH3U (EUA) do PROPRIO
 *  jogador. O app nao traz nada do jogo: le, do console, os arquivos do ExeFS
 *  do MH3U (.code, banner, icon, logo), confere os hashes, aplica o nosso
 *  code.bps e monta o update.cia seguindo a receita (romfs:/receita.bin, feita
 *  por tools/mh3u_receita.py). O resultado e conferido (SHA-256) antes de o AM
 *  concluir a instalacao.
 *
 *    A      instalar / atualizar
 *    Y      desinstalar o update (volta ao jogo original)
 *    START  sair
 */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nucleo.h"

#define TID_BASE    0x00040000000AE400ULL
#define TID_UPDATE  0x0004000E000AE400ULL

static PrintConsole cima, baixo;

/* ------------------------------------------------------------------ util */
static uint8_t *ler_romfs(const char *caminho, uint32_t *n)
{
    FILE *f = fopen(caminho, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); *n = (uint32_t)ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc(*n);
    if (b && fread(b, 1, *n, f) != *n) { free(b); b = NULL; }
    fclose(f);
    return b;
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
        if (rename(arqs[i], off) == 0) { printf("  %s -> .off\n", arqs[i] + 5); ++n; }
    }
    return n;
}

/* ------------------------------------------------------------ instalar */
typedef struct { Handle cia; u64 off; } Destino;

static int escrever(void *ctx, const uint8_t *p, uint32_t n)
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
    int pct = (int)(feito * 100 / (total ? total : 1));
    printf("\x1b[20;2H  gravando o update: %3d%%  ", pct);
    gfxFlushBuffers(); gfxSwapBuffers();
}

static bool conferir(const char *nome, const uint8_t *p, uint32_t n, const uint8_t *esperado)
{
    uint8_t h[32]; sha256(p, n, h);
    bool ok = memcmp(h, esperado, 32) == 0;
    printf("  %-24s %s\n", nome, ok ? "\x1b[32mok\x1b[0m" : "\x1b[31mDIFERENTE\x1b[0m");
    return ok;
}

static void instalar(const Receita *rec, const uint8_t *bps, uint32_t nbps, FS_MediaType mt)
{
    uint8_t *comp = NULL, *ban = NULL, *logo = NULL, *ic = NULL, *code = NULL;
    uint32_t nc = 0, nban = 0, nlogo = 0, nic = 0;
    Result r;
    int e;

    consoleSelect(&baixo); consoleClear();
    printf("Lendo o MH3U do %s...\n", mt == MEDIATYPE_SD ? "cartao SD" : "cartucho");
    if (R_FAILED(r = ler_exefs(mt, ".code", &comp, &nc)) ||
        R_FAILED(r = ler_exefs(mt, "banner", &ban, &nban)) ||
        R_FAILED(r = ler_exefs(mt, "icon", &ic, &nic)) ||
        R_FAILED(r = ler_exefs(mt, "logo", &logo, &nlogo))) {
        printf("\x1b[31mNao consegui ler o jogo (0x%08lX).\x1b[0m\n", r);
        goto fim;
    }

    uint32_t td = blz_tamanho(comp, nc);
    if (td != rec->tam_code_base) {
        printf("\x1b[31mEste MH3U nao e a versao suportada\n(EUA, 00040000000AE400, v1.0).\x1b[0m\n");
        goto fim;
    }
    code = malloc(rec->tam_code_alvo > td ? rec->tam_code_alvo : td);
    if (!code) { printf("\x1b[31mSem memoria.\x1b[0m\n"); goto fim; }
    printf("Descomprimindo o codigo do jogo...\n");
    if ((e = blz_descomprimir(comp, nc, code, td))) { printf("\x1b[31m%s\x1b[0m\n", nucleo_erro(e)); goto fim; }
    free(comp); comp = NULL;

    printf("Conferindo:\n");
    if (!conferir("codigo do jogo", code, td, rec->sha_code_base) ||
        !conferir("banner", ban, nban, rec->sha_banner) ||
        !conferir("logo", logo, nlogo, rec->sha_logo) ||
        !conferir("icone", ic, nic, rec->sha_icone)) {
        printf("\x1b[31mO jogo nao e o esperado (regiao/versao).\x1b[0m\n");
        goto fim;
    }

    printf("Aplicando o patch do online...\n");
    uint32_t ta = 0;
    if ((e = bps_aplicar_no_lugar(bps, nbps, code, td, rec->tam_code_alvo, &ta))) {
        printf("\x1b[31m%s\x1b[0m\n", nucleo_erro(e)); goto fim;
    }
    if (!conferir("codigo com o patch", code, ta, rec->sha_code_alvo)) goto fim;
    receita_icone(rec, ic, nic);

    printf("Instalando o update %s...\n", rec->versao);
    Destino d = { 0, 0 };
    if (R_FAILED(r = AM_StartCiaInstall(MEDIATYPE_SD, &d.cia))) {
        printf("\x1b[31mAM_StartCiaInstall: 0x%08lX\x1b[0m\n", r); goto fim;
    }
    Pedacos pc = { code, ta, ban, nban, logo, nlogo, ic, nic };
    e = receita_montar(rec, &pc, escrever, progresso, &d);
    if (e) {
        AM_CancelCIAInstall(d.cia);
        printf("\n\x1b[31m%s -- instalacao cancelada.\x1b[0m\n", nucleo_erro(e));
        goto fim;
    }
    if (R_FAILED(r = AM_FinishCiaInstall(d.cia))) {
        printf("\n\x1b[31mAM_FinishCiaInstall: 0x%08lX\x1b[0m\n", r); goto fim;
    }
    printf("\n\x1b[32mUpdate instalado!\x1b[0m\n");
    int n = desligar_luma();
    if (n) printf("(%d arquivo(s) do plugin/Luma desligados)\n", n);
    printf("\nAbra o MH3U: Ferry -> Multiplayer.\n");
    printf("Old 3DS: a tela preta ao abrir demora\num pouco mais -- e normal.\n");

fim:
    free(comp); free(ban); free(logo); free(ic); free(code);
    printf("\nAperte B.\n");
}

static void desinstalar(void)
{
    consoleSelect(&baixo); consoleClear();
    Result r = AM_DeleteTitle(MEDIATYPE_SD, TID_UPDATE);
    if (R_SUCCEEDED(r)) printf("\x1b[32mUpdate removido.\x1b[0m\nO MH3U volta ao original.\n");
    else printf("\x1b[31mNao removi (0x%08lX).\x1b[0m\n", r);
    printf("\nAperte B.\n");
}

/* ------------------------------------------------------------------ tela */
static void tela(const Receita *rec, bool tem_base, FS_MediaType mt)
{
    consoleSelect(&cima); consoleClear();
    printf("\x1b[1;2H\x1b[33mMH3U Online\x1b[0m  --  instalador %s\n", rec->versao);
    printf("\x1b[3;2HMultiplayer online p/ o Monster Hunter 3\n");
    printf("\x1b[4;2HUltimate de 3DS: salas, P2P e ate 4.\n");

    printf("\x1b[7;2HJogo (EUA): ");
    if (tem_base) printf("\x1b[32mencontrado\x1b[0m (%s)\n", mt == MEDIATYPE_SD ? "SD" : "cartucho");
    else          printf("\x1b[31mnao encontrado\x1b[0m\n");
    u16 v = 0;
    printf("\x1b[8;2HUpdate online: ");
    if (titulo_em(MEDIATYPE_SD, TID_UPDATE, &v))
        printf("\x1b[32minstalado\x1b[0m (v%u.%u.%u)\n", v >> 10, (v >> 4) & 0x3F, v & 0xF);
    else printf("nao instalado\n");

    printf("\x1b[11;2H A      instalar / atualizar\n");
    printf("\x1b[12;2H Y      desinstalar o update\n");
    printf("\x1b[13;2H START  sair\n");
    printf("\x1b[16;2H\x1b[90mO app le o MH3U do SEU console e aplica o\n");
    printf("\x1b[17;2Hpatch; nada do jogo vem dentro dele.\x1b[0m\n");
    consoleSelect(&baixo);
}

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, &cima);
    consoleInit(GFX_BOTTOM, &baixo);
    amInit();
    romfsInit();

    uint32_t nrec = 0, nbps = 0;
    uint8_t *brec = ler_romfs("romfs:/receita.bin", &nrec);
    uint8_t *bps = ler_romfs("romfs:/mh3u.bps", &nbps);
    Receita rec; memset(&rec, 0, sizeof rec);
    int e = brec ? receita_ler(brec, nrec, &rec) : -1;

    FS_MediaType mt = MEDIATYPE_SD;
    bool tem_base = achar_base(&mt);
    if (e || !bps) {
        consoleSelect(&cima);
        printf("\x1b[31mInstalador corrompido (receita/patch).\x1b[0m\nAperte START.\n");
    } else tela(&rec, tem_base, mt);

    bool esperando_b = false;
    while (aptMainLoop()) {
        hidScanInput();
        u32 k = hidKeysDown();
        if (k & KEY_START) break;
        if (esperando_b) {
            if (k & KEY_B) { esperando_b = false; consoleSelect(&baixo); consoleClear();
                             tem_base = achar_base(&mt); tela(&rec, tem_base, mt); }
        } else if (!e && bps) {
            if (k & KEY_A) {
                if (!tem_base) { consoleSelect(&baixo); consoleClear();
                                 printf("\x1b[31mMH3U (EUA) nao encontrado no SD nem no cartucho.\x1b[0m\n\nAperte B.\n"); }
                else instalar(&rec, bps, nbps, mt);
                esperando_b = true;
            } else if (k & KEY_Y) { desinstalar(); esperando_b = true; }
        }
        gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();
    }

    free(brec); free(bps);
    romfsExit(); amExit(); gfxExit();
    return 0;
}
