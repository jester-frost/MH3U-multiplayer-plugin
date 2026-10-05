# MH3U Multiplayer — online no 3DS (sem Wii U)

Dá **multiplayer online** ao **Monster Hunter 3 Ultimate** de **Nintendo 3DS**,
que de fábrica só tinha co-op local (Wi-Fi local) — o online era só no Wii U.
O multiplayer local do jogo vira online:

- **salas num servidor NEX** (o mesmo protocolo online dos outros Monster Hunter
  de 3DS);
- **partida direta console ⇄ console (P2P)**, com relay do servidor quando o
  P2P não abre;
- até **4 jogadores**; se o dono sai, outro herda a sala e quem saiu volta.

> **Versão 1.6.** O online foi testado em Old 3DS, New 3DS e no emulador
> Azahar, os três juntos na mesma sala e na mesma quest (até a 1.2). A
> instalação foi testada no Old 3DS (1.5); o visual novo da 1.6, no Azahar.
>
> **Novidades da 1.6 — armaduras Snake e The Boss revistas:**
> - **Sem buracos:** as laterais e as costas fecharam (o conversor lia mal parte
>   da malha do P3rd) e a textura não pega mais a borda errada.
> - **Cor da armadura:** as áreas tingíveis do P3rd agora são as do 3U — mudam
>   com o *pigmento* do jogo (High Rank; paleta livre depois da Dire Miralis).
>   Sem tingir, o cinza original.
> - **Cabelo:** só some com a cabeça do Snake ou da The Boss; com qualquer outro
>   elmo (inclusive com o resto do set) o penteado aparece normal.
> - **The Boss:** pescoço completo, presilhas vazadas como no P3rd, sem reflexo
>   colorido no cabelo, tom mais próximo do original.
>
> **Da 1.5:**
> - **A instalação funciona no console.** A 1.3 parava em cerca de 13 MB com
>   "falha ao gravar o update" (o AM do 3DS só aceita gravações em blocos
>   alinhados; o emulador aceita qualquer tamanho). Agora o update é gravado em
>   blocos de 1 MB.
> - **A quest MGS aparece no console.** O patch não informava a versão ao
>   servidor de DLC quando o servidor vinha pelo DNS do console.
> - **Atualização em peças:** o instalador ficou pequeno (cerca de 1,5 MB) e
>   baixa o conteúdo novo em **peças** (um arquivo cada, a maior com cerca de
>   4 MB), conferidas pela assinatura. Ele só baixa as peças que ainda não estão
>   no cartão: numa versão nova, o que não mudou não é baixado de novo, e uma
>   queda no meio continua de onde parou.
> - **START** pergunta antes de sair.
>
> **Da 1.3 — colaboração Metal Gear do MHP3rd:** as armaduras
> **Snake** (homem) e **The Boss** (mulher), lâmina e atirador, com o modelo,
> a textura e a cabeça do P3rd; o item **Big Boss Title**; a quest de evento
> **MGS - Hunter Eater Mission** (Nibelsnarf, no menu de DLC) que dá o título; e
> a forja no ferreiro (aparece ao obter o título). Com o set completo, os sons
> mudam como no P3rd: menus, caixas, roleta de itens, partida para a quest,
> vozes, o alerta quando o monstro te vê e a música de batalha. O conteúdo novo
> vai **dentro do update** (por isso ele passou a ter ~44 MB).
>
> **Da 1.2:** **Atualizar** remove o patch antigo antes de instalar o
> novo, e a versão do título sai certa no FBI.
>
> **Da 1.1:** o instalador se **atualiza sozinho** (baixa as versões
> novas deste repositório, conferidas por assinatura), tem abas **Patch atual**
> e **Versões**, barra de progresso, toque na tela e textos em português, inglês
> e espanhol; e o **menu de DLC funciona no emulador** sem mexer no `hosts` do PC
> (basta o `servidor=` no `mh3u-online.cfg`).

---

## Três jeitos de instalar — escolha UM

| | O que é | Precisa de | Recomendado p/ |
|---|---|---|---|
| **1. Instalador** | um app que instala o **update** do online por cima do **seu** MH3U | só o FBI p/ instalar o app | **quem quer o jeito mais simples** |
| **2. Patch nativo** | os mesmos arquivos do update, aplicados pelo Luma | "game patching" do Luma | quem não quer instalar título novo |
| **3. Plugin** | o online como plugin do Luma (o jeito antigo) | "game patching" **e** plugin loader do Luma | quem já usa (**sem o menu de DLC**) |

Em todos: **MH3U norte-americano** (título `00040000000AE400`, cartucho ou
eShop), **Luma3DS v13+** e o **DNS do console apontando para o servidor**
(Configurações de Internet → sua rede → DNS → Manual → primário = IP do
servidor, secundário `0.0.0.0`). Outras regiões ainda não.

> **Old 3DS / 2DS:** nos jeitos 1 e 2, **a tela preta ao abrir o jogo demora
> alguns segundos a mais**. É o console reservando memória extra para o online
> (80 MB, o mesmo que o Monster Hunter 4 Ultimate faz). É normal — não desligue.

### 1. Instalador (recomendado)

> **Validado** em Old 3DS, New 3DS e Azahar: o update instalado por ele entra
> no online **mesmo com o "Enable game patching" do Luma desligado** — não
> precisa de plugin loader nem de patch do Luma. (O Luma continua sendo o CFW:
> é por ele que o FBI instala o `.cia`.)

1. Copie `instalador/mh3u-online-instalador.cia` para o cartão SD e instale pelo
   **FBI**. Aparece o ícone **MH3U Online** no HOME menu.
2. Abra o **MH3U Online**. Ele mostra a versão (ex.: `1.6`, e no FBI o título
   aparece como `v1.6.0`), se achou o seu MH3U (no SD ou no cartucho) e procura
   sozinho uma versão mais nova (aba **Versões**: o que mudou em cada uma).
3. **Convite:** se o servidor pede convite (o do projeto pede), aperte **X** e
   digite o código que o dono do servidor te passou (ex.: `K7M4-2QXZ`). É uma
   vez só: fica gravado no cartão. Veja [Convite](#convite).
4. Aperte **A**. O app lê o jogo **do seu console**, confere que é a versão
   certa, aplica o patch e instala o **update** (~44 MB, com o conteúdo novo). Na primeira vez
   ele baixa o conteúdo (~30 MB, em peças; precisa de internet). A tela mostra cada
   passo e a porcentagem; no fim aparece **CONCLUIDO**. Não desligue no meio.
5. Abra o MH3U → **Ferry → Multiplayer**.

- **Versão nova:** o app avisa ao abrir; **B** baixa (do GitHub, conferida pela
  assinatura) e **A** instala no jogo. Se vier um app novo, ele mesmo se
  atualiza: feche e abra de novo.
- **Voltar ao jogo original:** abra o MH3U Online e aperte **Y** (remove o
  update). O save não é tocado.
- O instalador desliga sozinho (renomeia para `.off`) o plugin e os patches
  antigos do Luma para este jogo, que brigariam com o update.
- Também há `instalador/mh3u-online-instalador.3dsx` para o Homebrew Launcher.

### 2. Patch nativo (pelo Luma)

Copie para o cartão SD:

| Arquivo deste repositório | Vai no SD como |
|---|---|
| `nativo/code.bps` | `sd:/luma/titles/00040000000AE400/code.bps` |
| `nativo/exheader.bin` | `sd:/luma/titles/00040000000AE400/exheader.bin` |

e a pasta **`nativo/romfs/`** inteira como `sd:/luma/titles/00040000000AE400/romfs/`
(as armaduras, os textos e os sons novos da 1.3; sem ela as peças novas ficam
invisíveis).

Ligue o **"Enable game patching"** no Luma (segure SELECT ao ligar). Não deixe
`code.ips` nem `.3gx` deste jogo junto.

**No Azahar** (inclusive com o jogo em `.3ds`/`.cci`, que o instalador não
enxerga): botão direito no jogo → *Open Mods Location* e deixe

```
load/mods/00040000000AE400/exheader.bin
load/mods/00040000000AE400/exefs/code.bps
load/mods/00040000000AE400/romfs/...      (o conteúdo de nativo/romfs/)
```

mais o `mh3u-online.cfg` na **raiz** do `sdmc` (ao lado da pasta `Nintendo 3DS`)
com `servidor=` e `convite=` — o emulador não tem o DNS do console.

### 3. Plugin (o jeito antigo)

| Arquivo | Vai no SD como |
|---|---|
| `plugin/mh3u-online.3gx` | `sd:/luma/plugins/00040000000AE400/mh3u-online.3gx` |
| `patch/code.ips` | `sd:/luma/titles/00040000000AE400/code.ips` |

"Enable game patching" + Rosalina (L + ↓ + SELECT) → "Plugin Loader: Enabled".
Para voltar ao original: `patch/code-reverse.ips`. No Azahar: *Emulation →
Configure → System → Enable 3GX plugin loader*.

> O plugin faz o online, mas **não** o menu de DLC nem o conteúdo da 1.3
> (armaduras Snake/The Boss): para isso, use o instalador ou o patch nativo.

---

## Convite

O servidor fica **fechado** para a internet: só entra quem tem convite. O dono
do servidor gera um código para você (ex.: `K7M4-2QXZ`); você digita **uma vez**
e nunca mais pensa nisso.

- **Instalador:** tecla **X** (a tela mostra `Convite: K7M4-****` quando tem).
- **Patch nativo / plugin / emulador:** crie `sd:/mh3u-online.cfg` com a linha
  `convite=K7M4-2QXZ` (no Azahar, dentro da pasta `sdmc`).

Toda vez que o MH3U abre, ele avisa o servidor com o convite e o servidor libera
**a rede de onde você está jogando** por 24 h — em casa, no 4G ou na casa de um
amigo, sem pedir nada a ninguém. O código não viaja pela rede (vai só uma
assinatura dele). Se o dono revogar o seu convite, o acesso some em até 1 min.

Sem convite, ou com convite recusado, o jogo abre normal, mas não acha o
servidor (o online não conecta).

---

## Jogar

**Ferry → Multiplayer** (Port Tanzia). Entrar no Port já cria a sua sala no
servidor; para entrar na de alguém: *Player Select* → a sala → entrar.

**Friend Search** (Ferry → Hunter Search → *Choose a Port* → **Friend Search**,
desde a `1.0-beta6`): as salas de quem é seu **amigo na lista do 3DS** aparecem
**primeiro**, e as dos outros jogadores logo depois. O *Player Select* continua
mostrando todas. Para valer, os dois precisam estar na `1.0-beta6` (o jogo entra
no servidor com o código de amigo do console).

**Downloads / DLC** (tela inicial → *DLC*, desde a `1.0-beta7`; no emulador, desde a `1.1`): o menu de
download volta a funcionar com o servidor do projeto. Dá para ler os
**Content Previews** e baixar **Event Quests**, incluindo quests trazidas do
**Monster Hunter Portable 3rd** que nunca saíram no MH3U (a primeira é *White
Rabbit Beast!*, um Lagombi na Tundra, HR 6+). Como no jogo original, cada
quest só aparece no balcão quando o seu HR alcança o rank dela.

**Dois aparelhos na mesma casa:** muitos roteadores não deixam um aparelho
alcançar o outro pelo IP público da casa; aí a partida passa pelo **relay** do
servidor (automático) e fica com um pouco mais de latência.

## Problemas comuns

- **Fora de casa, a conexão cai a cada minuto** (sozinho parece online; cai quando
  alguém entra). O teste de conexão do 3DS (HTTP na porta 80 do servidor) não
  está passando. Quem roda o servidor: a liberação do jogador precisa de
  **TCP 80/443**, além das portas UDP do jogo. O convite já libera as duas
  (corrigido no servidor do projeto em 29/09/2026).
- **A busca não mostra sala nenhuma / não conecta:** falta o convite, ou o DNS do
  console não aponta para o servidor.

## Nada do jogo é distribuído aqui

- O **instalador** não traz nada da Capcom: ele lê o executável, o ícone e o
  banner do **MH3U do próprio console**, confere os hashes (só aceita a versão
  certa), aplica o `code.bps` (~126 KB, só as nossas mudanças) e monta o update
  seguindo uma *receita* (`romfs/receita2.bin`). O conteúdo novo (os nossos
  modelos, textos e sons) vem em peças (`pecas/<sha256>.bin`), cada uma
  conferida pelo SHA-256 da receita assinada e guardada em
  `sd:/3ds/mh3u-online/pecas/`. O resultado é conferido por SHA-256 antes de
  instalar. Código-fonte em `instalador/fonte/`.
- O **patch nativo** e o **plugin** são só as nossas mudanças / o nosso código.

## Como funciona (resumo técnico)

O cartucho de 3DS não tem NEX — o co-op dele é por UDS (Wi-Fi local). O nosso
código engancha as 17 chamadas de UDS do jogo e responde por conta própria:
criar/buscar/entrar numa sala viram *matchmaking* NEX no
[`mh3u-revival`](https://github.com/Matt-Wood-23/mh3u-revival) (com um patch do
projeto); os pacotes da partida vão direto entre os consoles (UDP) ou pelo
relay. No **patch nativo / instalador** esse código vive **dentro do executável
do jogo** (sem plugin loader); no **plugin**, como plugin do Luma.

**Servidor:** o mesmo arquivo serve para qualquer servidor. Ordem:
`sd:/mh3u-online.cfg` (`servidor=IP`, útil no emulador) → o DNS do console para
`mh3u3ds.pretendo.cc` → o servidor do projeto embutido.
