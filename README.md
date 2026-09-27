# MH3U Multiplayer — plugin de 3DS (online sem Wii U)

Dá **multiplayer online** ao **Monster Hunter 3 Ultimate** de **Nintendo 3DS**,
que de fábrica só tinha co-op local (Wi-Fi local / UDS) — o online era só no
Wii U. O plugin troca o Wi-Fi local do jogo por:

- **salas num servidor NEX** (o mesmo protocolo online dos outros Monster Hunter
  de 3DS), e
- **partida direta console ⇄ console (P2P)**, com relay do servidor só quando o
  P2P não abre.

Até **4 jogadores**, com troca de dono (quem sai deixa a sala com outro e
consegue voltar).

> **Estado: beta** (`mh3u-nex-beta1`). Testado no emulador (Azahar) e por
> testes automáticos contra o servidor de homologação: elenco, 4 jogadores,
> sala única, troca de dono, P2P → relay e rede com perda. **Primeira rodada em
> console de verdade agora.**

---

## O que você precisa

- **3DS / 2DS / New 3DS com Luma3DS v13+** (CFW).
- MH3U **norte-americano** — título **`00040000000AE400`** (cartucho ou eShop).
  Outras regiões ainda não: os ganchos são da versão US.
- Internet configurada no console.
- Acesso ao servidor: este beta aponta para o **servidor de homologação** do
  projeto, que só aceita os IPs liberados no firewall.

---

## Instalar

Dois arquivos no cartão SD:

| Arquivo deste repositório | Vai no SD como |
|---|---|
| `plugin/mh3u-online.3gx` | `sd:/luma/plugins/00040000000AE400/mh3u-online.3gx` |
| `patch/code.ips` | `sd:/luma/titles/00040000000AE400/code.ips` |

- O **plugin** é o multiplayer.
- O **patch** é obrigatório: aumenta o heap do jogo para caber a rede do NEX.
  Sem ele o plugin não liga a rede. Para voltar ao jogo original:
  `patch/code-reverse.ips`.
- Deixe **só um** `.3gx` na pasta do jogo (apague o `uds-espia.3gx` antigo).

No **Luma3DS**:

1. Segure **SELECT** ao ligar o console → marque **"Enable game patching"** →
   salve com START.
2. Com o console ligado, abra o Rosalina (**L + ↓ + SELECT**) → **"Plugin
   Loader: Enabled"**.

O plugin não tem menu nem texto na tela. Só aparece algo em **vermelho** se ele
não conseguir se instalar no jogo — aí ele fica fora do caminho e o jogo roda
normal.

---

## Jogar

Como no multiplayer local: **Ferry → Multiplayer** (Port Tanzia).

- **Hospedar:** entrar no Port já cria a sala no servidor.
- **Entrar:** *Player Select* → a sala do outro aparece → tocar e entrar.
- **Troca de dono:** se o dono sai, outro jogador herda a sala; quem saiu acha a
  sala de novo na busca e volta.

**Dois consoles na mesma casa:** o P2P direto depende do roteador fazer
"hairpin" (um aparelho alcançar o outro pelo IP público da casa). Muitos não
fazem; aí os dois passam para o **relay** do servidor em ~6 s, sozinhos. Em
casas diferentes o P2P direto deve fechar.

---

## Como funciona (resumo técnico)

O cartucho de 3DS **não tem NEX** — o co-op dele é por UDS (Wi-Fi local). O
plugin engancha as 17 chamadas de UDS do jogo (`nwm`) e responde por conta
própria:

- **Salas:** criar, buscar e entrar viram *matchmaking* NEX no
  [`mh3u-revival`](https://github.com/Matt-Wood-23/mh3u-revival) (com um patch do
  projeto). A busca do jogo recebe um beacon fabricado a partir das salas do
  servidor; a sala é identificada pelo **dono**, então atualizar o status não
  cria sala duplicada.
- **Partida:** os pacotes do jogo vão **direto** entre os consoles (UDP), pelo
  endereço que o NAT-check do servidor observou. O dono é o hub: dá o número de
  cada jogador, repassa quem está na sala e encaminha cada pacote ao destino.
- **Relay:** se o P2P não abre em 6 s, o console avisa o servidor
  (`ReportNATTraversalResult`) e aquele par passa a usar o relay.
- **Robustez:** pedidos ao servidor são reenviados se se perdem, e a conexão
  se refaz sozinha se o servidor a derrubar.

Para rodar o próprio servidor, a stack `mhxx` sobe o `mh3u-revival` com
`./mhxx jogo mh3u on` — ver `docs/mh3u-3ds-beta-console.md` (seção do operador).
Este beta vem compilado apontando para o servidor de homologação.

A versão anterior (bridge HTTP, servidor achado pelo DNS `mh3u3ds.pretendo.cc`)
está no histórico do git; o documento dela segue em
`docs/mh3u-coop-funcionando.pt-BR.md`.
