# MH3U Multiplayer — online no 3DS (sem Wii U)

Dá **multiplayer online** ao **Monster Hunter 3 Ultimate** de **Nintendo 3DS**,
que de fábrica só tinha co-op local (Wi-Fi local) — o online era só no Wii U.
O multiplayer local do jogo vira online:

- **salas num servidor NEX** (o mesmo protocolo online dos outros Monster Hunter
  de 3DS);
- **partida direta console ⇄ console (P2P)**, com relay do servidor quando o
  P2P não abre;
- até **4 jogadores**; se o dono sai, outro herda a sala e quem saiu volta.

> **Estado: beta.** Testado em Old 3DS, New 3DS e no emulador Azahar, os três
> juntos na mesma sala e na mesma quest — inclusive pelo instalador.

---

## Três jeitos de instalar — escolha UM

| | O que é | Precisa de | Recomendado p/ |
|---|---|---|---|
| **1. Instalador** | um app que instala o **update** do online por cima do **seu** MH3U | só o FBI p/ instalar o app | **quem quer o jeito mais simples** |
| **2. Patch nativo** | os mesmos arquivos do update, aplicados pelo Luma | "game patching" do Luma | quem não quer instalar título novo |
| **3. Plugin** | o online como plugin do Luma (o jeito antigo) | "game patching" **e** plugin loader do Luma | emulador / quem já usa |

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
2. Abra o **MH3U Online**. Ele mostra a versão (ex.: `1.0-beta2`, e no FBI o
   título aparece como `v1.0.2`) e se achou o seu MH3U (no SD ou no cartucho).
3. Aperte **A**. O app lê o jogo **do seu console**, confere que é a versão
   certa, aplica o patch e instala o **update** (~13 MB). A tela mostra cada
   passo e a porcentagem; no fim aparece **CONCLUIDO**. Não desligue no meio.
4. Abra o MH3U → **Ferry → Multiplayer**.

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

Ligue o **"Enable game patching"** no Luma (segure SELECT ao ligar). Não deixe
`code.ips` nem `.3gx` deste jogo junto. No **Azahar**: botão direito no jogo →
*Open Mods Location* → `exefs/code.bps` e `exheader.bin`.

### 3. Plugin (o jeito antigo)

| Arquivo | Vai no SD como |
|---|---|
| `plugin/mh3u-online.3gx` | `sd:/luma/plugins/00040000000AE400/mh3u-online.3gx` |
| `patch/code.ips` | `sd:/luma/titles/00040000000AE400/code.ips` |

"Enable game patching" + Rosalina (L + ↓ + SELECT) → "Plugin Loader: Enabled".
Para voltar ao original: `patch/code-reverse.ips`.

---

## Jogar

**Ferry → Multiplayer** (Port Tanzia). Entrar no Port já cria a sua sala no
servidor; para entrar na de alguém: *Player Select* → a sala → entrar.

**Dois aparelhos na mesma casa:** muitos roteadores não deixam um aparelho
alcançar o outro pelo IP público da casa; aí a partida passa pelo **relay** do
servidor (automático) e fica com um pouco mais de latência.

## Nada do jogo é distribuído aqui

- O **instalador** não traz nada da Capcom: ele lê o executável, o ícone e o
  banner do **MH3U do próprio console**, confere os hashes (só aceita a versão
  certa), aplica o `code.bps` (~126 KB, só as nossas mudanças) e monta o update
  seguindo uma *receita* (`romfs/receita.bin`). O resultado é conferido por
  SHA-256 antes de instalar. Código-fonte em `instalador/fonte/`.
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
