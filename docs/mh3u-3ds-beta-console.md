# MH3U 3DS online — beta no console

Monster Hunter 3 Ultimate de **3DS** jogando online (NEX + P2P direto, até 4
jogadores), sem Wii U. O multiplayer **local** do jogo vira online: o plugin
troca o Wi-Fi local (UDS) por salas no servidor (matchmaking NEX) e partida
direta console⇄console (P2P), com relay do servidor quando o P2P não abre.

> **Beta.** Testado no emulador (Azahar) e por testes automáticos contra o
> servidor de homologação. Esta é a primeira rodada em console de verdade.

## O que precisa

| | |
|---|---|
| Console | 3DS / 2DS / New 3DS com **Luma3DS v13 ou mais novo** (CFW) |
| Jogo | **MH3U norte-americano** (`00040000000AE400`), cartucho ou eShop. Outras regiões ainda **não** (os endereços dos ganchos são da versão US). |
| Rede | O **DNS do console apontando para o servidor** (o mesmo passo dos outros jogos). O servidor de homologação só aceita os IPs liberados no firewall (hoje: a casa do Marcos). |

## Os dois arquivos

Do repositório `MHXX-LOCAL` (tag `mh3u-nex-beta1`):

| Arquivo no repositório | Vai no cartão SD como |
|---|---|
| `plugin/plugin-beta.3gx` | `/luma/plugins/00040000000AE400/mh3u-online.3gx` |
| `patches/patchA-heap.ips` | `/luma/titles/00040000000AE400/code.ips` |

- O **plugin** é o multiplayer, e é o MESMO para qualquer servidor. Ao ligar a
  rede ele escolhe o servidor nesta ordem:
  1. `sd:/mh3u-online.cfg` com `servidor=IP-ou-nome` (teste; e o emulador);
  2. o **DNS do console** para `mh3u3ds.pretendo.cc` — o DNS da stack responde
     o IP do servidor MH3U dela (`hosts.conf`: `MH3U_IP`);
  3. o servidor oficial embutido (`24.199.103.160`) — console com DNS comum.
- O **patch de heap é obrigatório**: o NEX precisa de memória para a rede que o
  jogo original não reserva. Sem ele o plugin não consegue ligar a rede.
  (Reverso, para voltar ao jogo original: `patches/patchA-heap-reverse.ips`.)
- Deixe **só um** `.3gx` na pasta do jogo.

## Ligar no Luma

1. **Patch de jogo:** segure **SELECT** ao ligar o console → marque
   **"Enable game patching"** → salve (START).
2. **Plugin loader:** com o console ligado, abra o Rosalina (**L + ↓ + SELECT**)
   → **"Plugin Loader: Enabled"**.
3. **DNS:** Configurações de Internet → sua rede → DNS → Manual → primário =
   o IP do servidor (`24.199.103.160` na homologação), secundário `0.0.0.0`.
   O mesmo DNS dos outros jogos da stack.
4. Abra o MH3U.

Se algo der errado ao abrir, o plugin avisa em **vermelho** na tela e fica fora
do caminho (o jogo roda normal). Com tudo certo, não aparece nada.

## Jogar

Como no multiplayer local: **Ferry → Multiplayer** (Port Tanzia).

- **Hospedar:** entrar no Port já cria a sala no servidor.
- **Entrar:** Player Select / Search → a sala do outro aparece → tocar e entrar.
- Até **4 jogadores**. Se o dono sai, outro herda a sala e quem saiu consegue
  voltar.

Consoles na **mesma casa**: o P2P direto depende do roteador fazer "hairpin"; o
da casa de homologação não faz, então os dois caem no **relay** do servidor em
~6 s (automático, sem ação). Em casas diferentes o P2P direto deve fechar.

## Para o operador (servidor)

A stack da VPS serve o MH3U pelo `mh3u-revival` com o patch NEX+P2P:

```bash
./tools/mh3u-revival-setup.sh          # clona pinado + patch + .venv (idempotente)
# config.env (VPS de um IP):
MH3U_IP=24.199.103.160
MH3U_RELAY_MODO=auto
MH3U_NATCHECK=0                        # a 10025 e do nncs dos jogos de 3DS
MH3U_RELAY_ADVERTISE=24.199.103.160
./mhxx jogo mh3u on
```

O jogador aponta o DNS do console para a stack e usa o MESMO plugin: o nome
`mh3u3ds.pretendo.cc` resolve para o `MH3U_IP` desta stack. Emulador (Azahar,
que resolve pelo DNS do PC): `sdmc/mh3u-online.cfg` com `servidor=<IP>`.

Firewall: `1223,1224/udp` (NEX) e `27100:27387/udp` (relay), além da `10025/udp`
(NAT-check) que a stack já abre — liberados por IP de jogador.

Testes contra o servidor (daqui): `tools/nex_client/teste_{elenco,quatro,sala,
rede,troca}.py <ip>`.
