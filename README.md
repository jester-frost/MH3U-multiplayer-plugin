# MH3U Multiplayer — plugin de 3DS (online sem Wii U)

Dá **multiplayer online** ao **Monster Hunter 3 Ultimate** de **Nintendo 3DS**,
que de fábrica só tinha co-op local (por Wi-Fi local/UDS) — o online era só no
Wii U. Este plugin faz o papel que o Wii U fazia: intercepta o Wi-Fi local do
jogo e o leva a um **servidor central** pela internet, para dois consoles em
redes diferentes se acharem e jogarem juntos.

> **Estado:** co-op provada (dois jogos numa sessão, entrar em porto, caçar
> junto, troca de host). O gameplay é **P2P direto** entre os bridges; o
> servidor central faz só **matchmaking** (o relay é opcional). Mesmo desenho
> dos servidores de MHXX / MH4U / Generations.

---

## O que você precisa

- Um **3DS com CFW** (Luma3DS) e o **plugin loader** do Luma ligado.
- MH3U (cartucho ou dump) — **título US `00040000000AE400`**. (Outras regiões
  precisam do plugin recompilado com os endereços da região.)
- Um **servidor** rodando a stack (`./mhxx jogo mh3u3ds on`) — pode ser um PC
  na sua rede ou uma VPS. Veja **O lado do servidor** abaixo.

---

## Instalar o plugin

1. Copie **`plugin/uds-espia.3gx`** para o cartão SD, em:

   ```
   sd:/luma/plugins/00040000000AE400/uds-espia.3gx
   ```

2. No **Luma3DS**: segure **Select** ao ligar (menu de config) e ligue
   **"Enable game plugin loader"**.

3. Pronto. O plugin sobe junto com o jogo.

---

## Configurar a rede (é por DNS, sem IP fixo)

O plugin **não aponta para um IP** — ele usa o **DNS configurado na conexão do
console**. Ele procura o servidor pelo nome **`mh3u3ds.pretendo.cc`**, e o seu
DNS resolve esse nome para o seu servidor.

No **3DS** → *Configurações da Internet* → sua conexão → *Alterar
configurações* → *DNS* → **Manual**:

| campo | valor |
|---|---|
| DNS primário | **o IP do seu servidor** (o mesmo que atende os outros jogos) |
| DNS secundário | `0.0.0.0` |

O servidor de DNS da stack resolve `mh3u3ds.pretendo.cc` para o endereço do
servidor de salas. Trocar de servidor depois é só mudar o DNS — **não precisa
recompilar o plugin**.

---

## Jogar

1. Abra o MH3U com o plugin carregado.
2. Entre no **modo em rede** (o online do jogo).
3. Um jogador **hospeda** um porto; o outro faz **Friend's Port Search**, acha
   o porto e **entra**.
4. Os dois caem no mesmo porto (Port Tanzia) e caçam junto.

---

## O lado do servidor

O plugin fala com o servidor de salas (`tools/uds_salas.py`), que faz parte da
stack `mhxx`. Ligue o serviço:

```bash
./mhxx jogo mh3u3ds on        # sobe o servidor de salas na porta 6101
```

E aponte o nome no DNS da stack para o servidor (uma linha no mapa do
`dns_server`): `mh3u3ds.pretendo.cc = <IP do servidor>`.

- **Matchmaking central, gameplay P2P direto.** O tráfego de jogo não passa
  pelo servidor por padrão (menos latência). O **relay** (pelo servidor) é
  **opcional** — ligue-o (o toggle *Relay P2P* do painel `mhxx-gui`, ou
  `run/relay-p2p`) só quando o furo de NAT direto não fecha (CGNAT / NAT
  simétrico).

---

## Limitações honestas

- **Dois consoles na MESMA casa** (atrás do mesmo roteador) chegam ao servidor
  com o **mesmo IP público** e colidem na identificação. Para esse caso, use um
  build por console com identidade distinta, **ou** teste com o amigo em
  **outra rede** (o caso normal, em que cada um tem IP próprio).
- **Latência**: o gargalo é o HTTP do próprio 3DS (~100 ms/ida). Em caçada
  ficou fluido nos testes; a melhora (socket fora do HTTP) é trabalho futuro.
- Só o **título US `00040000000AE400`** por enquanto.

---

## Como funciona (resumo técnico)

O 3DS não fala NEX (o protocolo online). Ele faz co-op por **UDS** (Wi-Fi
local). O plugin engancha as chamadas de UDS (`nwm`) e as leva por **HTTP** ao
servidor de salas, que:

- **fabrica o beacon** que o jogo espera (a busca do jogo aceita porta
  **id8=01**);
- lista as salas (Friend's Port Search) e roteia quem entra;
- liga os dois consoles num **barramento**, cada um vendo o outro como um nó
  local — o papel que o Wii U fazia de "Multiplayer Port".

Detalhe em `docs/mh3u-coop-funcionando.pt-BR.md`.
