/*  rede.h -- baixar do repositorio publico (GitHub, espelho jsDelivr) no 3DS.
 *
 *  Independe da "guilda" (o DNS do console): DNS proprio direto em servidores
 *  publicos (8.8.8.8, 9.9.9.9, 1.1.1.1) e TLS do mbedTLS com o nome no SNI.
 *  O certificado nao e verificado -- a garantia e a assinatura do manifesto e
 *  o SHA-256 de cada arquivo (atualiza.h).
 *
 *  Medido no .85 (Old 3DS) em 03/10/2026: um dos IPs do GitHub nao responde
 *  naquela rede e o connect bloqueante do soc:U nunca volta (travava o
 *  console). Por isso: todos os IPs, connect com prazo e poll em cada leitura.
 */
#pragma once
#include <stdint.h>

/* etapa: texto curto p/ a tela; total = 0 se ainda nao se sabe */
typedef void (*RedeProgresso)(void *ctx, const char *etapa, uint32_t feito, uint32_t total);

int  rede_iniciar(void);
void rede_fim(void);

/*  caminho = "<ref>/<arquivo no repositorio>", ex. "main/atualizacao/versao.txt"
 *  ou "v1.0-beta8/nativo/code.bps". Tenta o GitHub e depois o jsDelivr.
 *  cap = tamanho maximo aceito. *out vem do malloc (o chamador libera). */
int  rede_baixar(const char *caminho, uint32_t cap, uint8_t **out, uint32_t *n,
                 RedeProgresso prog, void *ctx);

const char *rede_erro(int e);
