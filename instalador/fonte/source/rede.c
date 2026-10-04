/*  rede.c -- DNS proprio + TCP com prazo + mbedTLS + HTTP/1.0 (ver rede.h). */
#include "rede.h"
#include "textos.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/net_sockets.h>

#define REPO      "jester-frost/MH3U-multiplayer-plugin"
#define SOC_TAM   0x100000

enum { E_SOC = -1, E_DNS = -2, E_CONECTAR = -3, E_TLS = -4, E_HTTP = -5, E_GRANDE = -6,
       E_MEM = -7, E_CORTADO = -8, E_CAMINHO = -9 };

const char *rede_erro(int e)
{
    switch (e) {
    case E_SOC:      return T(T_E_SOC);
    case E_DNS:      return T(T_E_DNS);
    case E_CONECTAR: return T(T_E_CONECTAR);
    case E_TLS:      return T(T_E_TLS);
    case E_HTTP:     return T(T_E_HTTP);
    case E_GRANDE:   return T(T_E_GRANDE);
    case E_MEM:      return T(T_E_MEM);
    case E_CORTADO:  return T(T_E_CORTADO);
    case E_CAMINHO:  return T(T_E_CAMINHO);
    }
    return T(T_E_REDE);
}

static u32 *g_socbuf;
static mbedtls_entropy_context g_ent;
static mbedtls_ctr_drbg_context g_drbg;

static int entropia(void *d, unsigned char *out, size_t n, size_t *olen)
{ (void)d; if (R_FAILED(PS_GenerateRandomBytes(out, n))) return -1; *olen = n; return 0; }

int rede_iniciar(void)
{
    if (g_socbuf) return 0;
    g_socbuf = memalign(0x1000, SOC_TAM);
    if (!g_socbuf) return E_MEM;
    if (R_FAILED(socInit(g_socbuf, SOC_TAM))) { free(g_socbuf); g_socbuf = NULL; return E_SOC; }
    psInit();
    mbedtls_entropy_init(&g_ent); mbedtls_ctr_drbg_init(&g_drbg);
    if (mbedtls_entropy_add_source(&g_ent, entropia, NULL, 32, MBEDTLS_ENTROPY_SOURCE_STRONG) ||
        mbedtls_ctr_drbg_seed(&g_drbg, mbedtls_entropy_func, &g_ent, (const unsigned char *)"mh3u-online", 11)) {
        rede_fim(); return E_TLS;
    }
    return 0;
}

void rede_fim(void)
{
    if (!g_socbuf) return;
    mbedtls_ctr_drbg_free(&g_drbg); mbedtls_entropy_free(&g_ent);
    psExit(); socExit();
    free(g_socbuf); g_socbuf = NULL;
}

/* ------------------------------------------------------------------- DNS */
static int dns_a(const char *servidor, const char *nome, uint32_t *ips, int cap)
{
    unsigned char q[300]; int n = 0;
    uint16_t id = (uint16_t)osGetTime();
    q[n++] = id >> 8; q[n++] = id; q[n++] = 0x01; q[n++] = 0x00;
    q[n++] = 0; q[n++] = 1; memset(q + n, 0, 6); n += 6;
    for (const char *p = nome; *p; ) {
        const char *ponto = strchr(p, '.'); int k = ponto ? (int)(ponto - p) : (int)strlen(p);
        if (k <= 0 || k > 63 || n + k + 6 > (int)sizeof q) return -1;
        q[n++] = k; memcpy(q + n, p, k); n += k; p += k + (ponto ? 1 : 0);
        if (!ponto) break;
    }
    q[n++] = 0; q[n++] = 0; q[n++] = 1; q[n++] = 0; q[n++] = 1;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return -1;
    struct sockaddr_in a = { 0 }; a.sin_family = AF_INET; a.sin_port = htons(53);
    inet_aton(servidor, &a.sin_addr);
    if (sendto(s, q, n, 0, (struct sockaddr *)&a, sizeof a) != n) { close(s); return -1; }
    struct pollfd pf = { s, POLLIN, 0 };
    if (poll(&pf, 1, 2500) <= 0) { close(s); return -1; }
    unsigned char r[512]; int m = recv(s, r, sizeof r, 0); close(s);
    if (m < 12 || r[0] != q[0] || r[1] != q[1]) return -1;
    int an = r[6] << 8 | r[7], o = 12, achou = 0;
    while (o < m && r[o]) o += r[o] + 1;
    o += 5;
    for (int i = 0; i < an && o + 10 <= m && achou < cap; ++i) {
        if ((r[o] & 0xc0) == 0xc0) o += 2; else { while (o < m && r[o]) o += r[o] + 1; o++; }
        if (o + 10 > m) break;
        int tipo = r[o] << 8 | r[o + 1], tam = r[o + 8] << 8 | r[o + 9];
        if (tipo == 1 && tam == 4 && o + 14 <= m) memcpy(&ips[achou++], r + o + 10, 4);
        o += 10 + tam;
    }
    return achou ? achou : -1;
}

static int resolver(const char *nome, uint32_t *ips, int cap)
{
    static const char *DNS[] = { "8.8.8.8", "9.9.9.9", "1.1.1.1" };
    for (unsigned i = 0; i < 3; ++i) {
        int n = dns_a(DNS[i], nome, ips, cap);
        if (n > 0) return n;
    }
    return E_DNS;
}

/* ------------------------------------------------------------------- TCP */
/*  connect com prazo. No 3DS o SO_ERROR nao diz se completou (fica -26, "em
 *  andamento"): pergunta-se de novo com connect() ate vir EISCONN. */
static int conectar(uint32_t ip, int porta, int prazo_ms)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;
    struct sockaddr_in a = { 0 }; a.sin_family = AF_INET; a.sin_port = htons(porta); a.sin_addr.s_addr = ip;
    int fl = fcntl(s, F_GETFL, 0); fcntl(s, F_SETFL, fl | O_NONBLOCK);
    u64 t0 = osGetTime();
    for (;;) {
        int e = connect(s, (struct sockaddr *)&a, sizeof a) < 0 ? errno : EISCONN;
        if (e == EISCONN) break;
        if ((e != EINPROGRESS && e != EALREADY && e != EWOULDBLOCK) || osGetTime() - t0 > (u64)prazo_ms) {
            close(s); return -1;
        }
        svcSleepThread(50 * 1000000LL);
    }
    fcntl(s, F_SETFL, fl);
    return s;
}

static int env(void *ctx, const unsigned char *b, size_t n)
{ int r = send(*(int *)ctx, b, n, 0); return r < 0 ? (errno == EAGAIN ? MBEDTLS_ERR_SSL_WANT_WRITE : MBEDTLS_ERR_NET_SEND_FAILED) : r; }

static int rec(void *ctx, unsigned char *b, size_t n)
{
    struct pollfd pf = { *(int *)ctx, POLLIN, 0 };
    int p = poll(&pf, 1, 15000);                                 /* 15 s sem nada = desiste */
    if (p == 0) return MBEDTLS_ERR_SSL_TIMEOUT;
    if (p < 0) return MBEDTLS_ERR_NET_RECV_FAILED;
    int r = recv(*(int *)ctx, b, n, 0);
    return r < 0 ? (errno == EAGAIN ? MBEDTLS_ERR_SSL_WANT_READ : MBEDTLS_ERR_NET_RECV_FAILED) : r;
}

/* --------------------------------------------------------------- HTTPS GET */
static int get(const char *host, int porta, const char *alvo, uint32_t cap, uint8_t **out, uint32_t *n,
               RedeProgresso prog, void *ctx)
{
    uint32_t ips[8]; struct in_addr lit;
    if (prog) prog(ctx, T(T_R_SERVIDOR), 0, 0);
    int nips = inet_aton(host, &lit) ? (ips[0] = lit.s_addr, 1) : resolver(host, ips, 8);
    if (nips < 0) return nips;
    int s = -1;
    if (prog) prog(ctx, T(T_R_CONECTANDO), 0, 0);
    for (int i = 0; i < nips && s < 0; ++i) s = conectar(ips[i], porta, 5000);
    if (s < 0) return E_CONECTAR;

    mbedtls_ssl_context ssl; mbedtls_ssl_config cfg;
    mbedtls_ssl_init(&ssl); mbedtls_ssl_config_init(&cfg);
    int r = mbedtls_ssl_config_defaults(&cfg, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                        MBEDTLS_SSL_PRESET_DEFAULT);
    if (!r) {
        mbedtls_ssl_conf_authmode(&cfg, MBEDTLS_SSL_VERIFY_NONE);   /* garantia = assinatura do manifesto */
        mbedtls_ssl_conf_rng(&cfg, mbedtls_ctr_drbg_random, &g_drbg);
        r = mbedtls_ssl_setup(&ssl, &cfg);
    }
    if (!r) r = mbedtls_ssl_set_hostname(&ssl, host);
    if (!r) {
        mbedtls_ssl_set_bio(&ssl, &s, env, rec, NULL);
        while ((r = mbedtls_ssl_handshake(&ssl)) == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE) ;
    }
    int ret = r ? E_TLS : 0;
    if (!ret) {
        /* HTTP/1.0: sem chunked, o corpo vai ate o servidor fechar */
        char req[512];
        int k = snprintf(req, sizeof req, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: MH3U-Online\r\n"
                                          "Connection: close\r\n\r\n", alvo, host);
        if (mbedtls_ssl_write(&ssl, (unsigned char *)req, k) != k) ret = E_TLS;
    }
    uint8_t *corpo = NULL; uint32_t tam = 0, esperado = 0;
    if (!ret) {
        static char cab[4096]; uint32_t ncab = 0; bool no_corpo = false;
        static unsigned char buf[0x4000];
        if (prog) prog(ctx, T(T_R_BAIXANDO), 0, 0);
        for (;;) {
            int k = mbedtls_ssl_read(&ssl, buf, sizeof buf);
            if (k == MBEDTLS_ERR_SSL_WANT_READ || k == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
            if (k == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || k == 0) break;
            if (k < 0) { if (no_corpo && esperado && tam == esperado) break; ret = E_CORTADO; break; }
            unsigned char *p = buf; uint32_t q = (uint32_t)k;
            if (!no_corpo) {
                uint32_t cabe = q < sizeof cab - 1 - ncab ? q : sizeof cab - 1 - ncab;
                memcpy(cab + ncab, p, cabe); ncab += cabe; cab[ncab] = 0;
                char *fim = strstr(cab, "\r\n\r\n");
                if (!fim) { if (ncab >= sizeof cab - 1) { ret = E_HTTP; break; } continue; }
                int st = 0; sscanf(cab, "HTTP/1.%*d %d", &st);
                if (st != 200) { ret = E_HTTP; break; }
                for (char *l = cab; l && l < fim; l = strstr(l, "\r\n") ? strstr(l, "\r\n") + 2 : NULL)
                    if (!strncasecmp(l, "content-length:", 15)) esperado = (uint32_t)strtoul(l + 15, NULL, 10);
                if (esperado > cap) { ret = E_GRANDE; break; }
                corpo = malloc(esperado ? esperado : cap);
                if (!corpo) { ret = E_MEM; break; }
                no_corpo = true;
                uint32_t usado = (uint32_t)(fim + 4 - cab) - (ncab - cabe);   /* bytes deste bloco que eram cabecalho */
                p += usado; q -= usado;
            }
            if (tam + q > (esperado ? esperado : cap)) { ret = E_GRANDE; break; }
            memcpy(corpo + tam, p, q); tam += q;
            if (prog) prog(ctx, T(T_R_BAIXANDO), tam, esperado);
        }
        if (!ret && !no_corpo) ret = E_CORTADO;
        if (!ret && esperado && tam != esperado) ret = E_CORTADO;
    }
    mbedtls_ssl_close_notify(&ssl);
    mbedtls_ssl_free(&ssl); mbedtls_ssl_config_free(&cfg);
    close(s);
    if (ret) { free(corpo); return ret; }
    *out = corpo; *n = tam;
    return 0;
}

int rede_baixar(const char *caminho, uint32_t cap, uint8_t **out, uint32_t *n,
                RedeProgresso prog, void *ctx)
{
    const char *barra = strchr(caminho, '/');
    if (!barra || barra == caminho || strlen(caminho) > 200) return E_CAMINHO;
    int ref = (int)(barra - caminho);
    char alvo[300];
    int e = rede_iniciar();
    if (e) return e;
#ifdef REDE_TESTE_HOST
    /* build de teste (make TESTE_CFLAGS=...): um servidor HTTPS local no lugar do GitHub */
    snprintf(alvo, sizeof alvo, "/%s", caminho);
    (void)ref;
    return get(REDE_TESTE_HOST, REDE_TESTE_PORTA, alvo, cap, out, n, prog, ctx);
#endif
    /* 1) GitHub direto (atualiza em minutos); 2) jsDelivr (CDN, guarda em cache) */
    snprintf(alvo, sizeof alvo, "/" REPO "/%s", caminho);
    e = get("raw.githubusercontent.com", 443, alvo, cap, out, n, prog, ctx);
    if (e && e != E_GRANDE && e != E_MEM) {
        snprintf(alvo, sizeof alvo, "/gh/" REPO "@%.*s%s", ref, caminho, barra);
        int e2 = get("cdn.jsdelivr.net", 443, alvo, cap, out, n, prog, ctx);
        if (!e2 || e == E_DNS) e = e2;
    }
    return e;
}
