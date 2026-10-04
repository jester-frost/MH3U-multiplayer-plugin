/*  textos.c -- a tabela de textos e a escolha do idioma (ver textos.h). */
#include "textos.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>

int g_idioma = IDIOMA_EN;

static const char *const TABELA[N_TEXTOS][N_IDIOMAS] = {
#define TX_LINHA(id, pt, en, es) [id] = { pt, en, es },
    TEXTOS(TX_LINHA)
#undef TX_LINHA
};

const char *T(int id)
{
    if (id < 0 || id >= N_TEXTOS) return "?";
    const char *t = TABELA[id][g_idioma];
    return t && t[0] ? t : TABELA[id][IDIOMA_EN];
}

void idioma_escolher(void)
{
    FILE *f = fopen("sdmc:/mh3u-online.cfg", "r");
    if (f) {
        char ln[64];
        while (fgets(ln, sizeof ln, f))
            if (!strncmp(ln, "idioma=", 7)) {
                const char *v = ln + 7;
                fclose(f);
                if (v[0] == 'p') { g_idioma = IDIOMA_PT; return; }
                if (v[0] == 'e' && v[1] == 's') { g_idioma = IDIOMA_ES; return; }
                if (v[0] == 'e') { g_idioma = IDIOMA_EN; return; }
                f = NULL;
                break;
            }
        if (f) fclose(f);
    }
    u8 lingua = CFG_LANGUAGE_EN;
    if (R_SUCCEEDED(cfguInit())) {
        CFGU_GetSystemLanguage(&lingua);
        cfguExit();
    }
    g_idioma = lingua == CFG_LANGUAGE_PT ? IDIOMA_PT : lingua == CFG_LANGUAGE_ES ? IDIOMA_ES : IDIOMA_EN;
}
