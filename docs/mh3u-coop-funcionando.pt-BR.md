# MH3U co-op online funcionando — dois 3DS numa sessão, sem Wii U

**Data: 2026-09-25.** Provado ao vivo com dois Azahar (BiguysDikus + CLIENT),
cada um com o plugin HTTP, contra um `uds_salas.py` único. Entraram na mesma
sala, viraram nós 1 e 2, trocaram sessão **sem perda**, e aguentaram **troca de
host**. É a co-op de rede do MH3U rodando pela ponte — o papel que era do Wii U.

> **O que isto prova, e o projeto marcava como suposição:** *o jogo ACEITA a
> sessão entregue pela ponte.* O doc `mh3u-o-que-esta-provado.md` listava isso
> como "só o hardware diz". Agora o hardware (emulado) disse: aceita.

---

## O que rodou

Um `uds_salas.py` **único** escutando em **duas portas** (6101 e 6102), com uma
`Central` só — logo, **um barramento só**. Os dois consoles falam com a mesma
ponte e caem no mesmo barramento; um entra na sala do outro e viram uma sessão.

```
   Azahar A (plugin -> :6101) \
                               >--- uds_salas.py (Central única) --- barramento
   Azahar B (plugin -> :6102) /         6101 + 6102
```

### Comando

```bash
python3 tools/uds_salas.py --porta 6101 --porta2 6102
```

- `--porta2` (novo): sobe um **segundo servidor HTTP** na **mesma** `Mao.central`.
  Dois Azahar com plugins de portas diferentes caem no mesmo barramento.
- **Sem** `--peer`, **sem** `--nex`. É barramento local puro — o caminho que o
  `tools/uds_console_falso.py` já validava contra a captura real.
- Config **padrão** (porta publicada, `ENTRAR_NA_PONTE=True`).

Os dois Azahar são distinguidos pelo cabeçalho **`X-Dono`** (o build de teste do
plugin manda; um vira `192.168.18.38#az1`, o outro fica no IP puro). Sem isso os
dois teriam o mesmo dono e um console "co-oparia consigo mesmo".

### Prova (do `/membros`, ao vivo)

```
sala 192.168.18.38#az1  membros { #az1: 1,  192.168.18.38: 2 }   <- B entrou como nó 2
  #az1           n=476  perdidos=0  na_fila=0
  192.168.18.38  n=462  perdidos=0  na_fila=0
```

- **Nó 1 = host, nó 2 = convidado** — a numeração real do UDS.
- Contadores subindo (293→476 em ~30s), **perdidos=0**: sessão viva e limpa.
- Sem `/sair` durante a sessão: a conexão **não caiu**.
- **Troca de host**: o convidado saiu e re-entrou; os papéis inverteram
  (`{ 192.168.18.38: 1, #az1: 2 }`) e a sessão subiu de novo.

---

## Por que funciona (o barramento)

Cada console que anuncia sua rede entra no **barramento** (`Central.caixas`).
Quando um entra na sala do outro (`GET /entrar?rede=<rede do host>`), o jogo
vira **nó 2** naquela sala. Daí em diante:

- O envio de um cai na caixa do outro com nó de origem = o **nó da porta** na
  sala de quem recebe (`enviar_no_barramento` / `no_da_porta_para`), nunca o nó
  1 — entregar como nó 1 faria o jogo achar que a mensagem veio dele mesmo e
  descartá-la (despachante `0x0094E2C4`).
- O `/receber` (drenado pelo plugin no `PullPacket`) devolve o pacote já no
  formato do UDS: `u16 tamanho, u16 nó, bytes`.

O formato do transporte é o mesmo do `/lote` do plugin: `u32 quantos | por
pacote: u16 tamanho, u16 destino, bytes`. Nada foi reinventado — é o caminho
HTTP que o plugin já usava.

---

## O caminho até aqui (o que destravou)

Três defeitos travavam a co-op cross-console antes disto:

1. **Descoberta (a lista vinha vazia).** O *Friend's Port Search* só lista
   portas **id8=01** (medido: o jogo recusa 23/23 redes id8=02, aceita 10/10
   id8=01). As salas remotas eram publicadas como id8=02 → o jogo as ignorava.
   Corrigido em `sincronizar_remotas`: `rede[0x14] = ID8_DA_PORTA[0]`.

2. **Salas clonadas.** Sessões NEX velhas (de pontes reiniciadas) ficavam
   penduradas com o mesmo appdata → o jogo via os próprios espelhos-fantasma.
   Corrigido com dedup por identidade do jogador (`appdata[1:9]`, mantendo o gid
   mais novo e excluindo o que casa com uma sala local).

3. **A sessão não fundia (cross-bridge).** Com duas pontes + relay UDP direto
   (`--peer`), os bytes cruzavam mas o jogo não engatava — o relay não
   reproduzia exatamente a entrega do barramento. **A bridge única resolveu**:
   um barramento só, e o jogo aceitou na hora.

---

## O que já está resolvido, e o que falta

**Resolvido (o cerne):**
- Descoberta: um console acha a porta do outro (id8=01, lista limpa).
- Sessão: um entra na sala do outro, viram nós 1/2, trocam dados sem perda.
- **O jogo aceita a sessão da ponte** — antes era suposição.
- Troca de host.

**Falta (engenharia, não incógnita):**
- **Redes de fato diferentes.** O teste acima é uma máquina, uma ponte. Para
  dois consoles em redes diferentes, o transporte entre as duas pontes precisa
  se comportar **idêntico** ao barramento local (mesma origem de nó, mesma
  ordem). O `--peer` e o `--nex` (descoberta) já existem; falta fazer o relay
  peer copiar exatamente o `enviar_no_barramento` — agora com a bridge única
  como referência do que "certo" significa.

## Reproduzir

1. Dois Azahar, cada um com o plugin (um mira 6101, o outro 6102) e um
   **`X-Dono` distinto** por instância.
2. `python3 tools/uds_salas.py --porta 6101 --porta2 6102`
3. Nos dois jogos: modo em rede. Um hospeda o hub; o outro faz *Friend's Port
   Search*, acha a porta e entra.
4. Conferir: `curl -s localhost:6101/membros` deve mostrar uma sala com **dois
   nós** e `perdidos=0`.

---

## Infra: no mhxx-gui, com toggle de relay (25/09)

O servidor MH3U 3DS entra no painel `mhxx`/`mhxx-gui` como os outros jogos, com
o **mesmo modelo de MHG/MH4U**: matchmaking central + P2P direto + relay opcional.

- **Ligar/desligar:** `./mhxx jogo mh3u3ds on|off` (ou o liga/desliga no painel).
  Sobe o `tools/uds_salas.py` na porta `$UDS_PORT` (6101) com a **config de
  co-op** (`--nex $MH3U_IP` p/ matchmaking; porta padrão) — NÃO mais a de debug
  (`--sem-porta --sem-entrar-ponte`, que não fecha a co-op).
- **Toggle de relay:** o botão **Relay P2P** do `mhxx-gui` escreve
  `run/relay-p2p`. O `nex_secure` (MHG/MH4U) o relê ao vivo; e agora o
  `start_mh3u` (revival) o lê no arranque → `MH3U_RELAY=force` quando ligado,
  `off` (P2P direto) quando desligado. Trocar o relay do MH3U pede
  `./mhxx restart mh3u`.
- **Gameplay = P2P direto por padrão** (o relay é peso morto onde o furo de NAT
  fecha sozinho); o central só faz descoberta. Provado ao vivo: com o revival
  reiniciando, o tráfego de jogo não piscou.

**Deploy na VPS:** `./mhxx setup && ./mhxx start` (a VPS já existe). O mesmo
liga/desliga e o toggle de relay valem lá. Ligar o relay quando alguém estiver
atrás de CGNAT / NAT simétrico (o furo direto não fecha) — ver `docs/vps.pt-BR.md`.

## Próxima etapa (roadmap)

Depois de estável, **assar o plugin num patch do binário** do jogo
(`code.ips`, com reverse — ver a política em memory), **como um update de
atualização** — pra rodar sem o `.3gx` externo.
