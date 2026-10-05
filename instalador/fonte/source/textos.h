/*  textos.h -- os textos do instalador em portugues, ingles e espanhol.
 *
 *  O idioma vem do console (CFGU_GetSystemLanguage), como no Guild Hunter:
 *  portugues e espanhol leem o seu, o resto le ingles. `idioma=pt|en|es` no
 *  sd:/mh3u-online.cfg passa por cima. Os textos que vem do manifesto
 *  (mudanca=, historico=) tem versao por idioma em atualiza.h.
 *
 *  Os BT_* sao os icones dos botoes na fonte do sistema (U+E000...).
 */
#pragma once

#define BT_A "\xEE\x80\x80"
#define BT_B "\xEE\x80\x81"
#define BT_X "\xEE\x80\x82"
#define BT_Y "\xEE\x80\x83"
#define BT_L "\xEE\x80\x84"
#define BT_R "\xEE\x80\x85"

enum { IDIOMA_PT = 0, IDIOMA_EN, IDIOMA_ES, N_IDIOMAS };

#define TEXTOS(X) \
    X(T_SUBTITULO, "Multiplayer online do Monster Hunter 3 Ultimate", "Online multiplayer for Monster Hunter 3 Ultimate", "Multijugador en línea de Monster Hunter 3 Ultimate") \
    X(T_JOGO, "Jogo (EUA)", "Game (USA)", "Juego (EE. UU.)") \
    X(T_ENCONTRADO, "encontrado (%s)", "found (%s)", "encontrado (%s)") \
    X(T_CARTAO_SD, "cartão SD", "SD card", "tarjeta SD") \
    X(T_CARTUCHO, "cartucho", "game card", "cartucho") \
    X(T_NAO_ENCONTRADO, "não encontrado", "not found", "no encontrado") \
    X(T_PATCH_JOGO, "Patch no jogo", "Patch in the game", "Parche en el juego") \
    X(T_NAO_INSTALADO, "não instalado", "not installed", "no instalado") \
    X(T_DESATUALIZADO, "%s  (desatualizado)", "%s  (outdated)", "%s  (desactualizado)") \
    X(T_PATCH_APP, "Patch no app", "Patch in the app", "Parche en la app") \
    X(T_BAIXADO, "(baixado)", "(downloaded)", "(descargado)") \
    X(T_VEIO_APP, "(veio no app)", "(built in)", "(incluido)") \
    X(T_MAIS_NOVA, "Mais nova online", "Latest online", "Más nueva en línea") \
    X(T_NAO_PROCURADA, "não procurada", "not checked", "sin buscar") \
    X(T_PROCURANDO, "procurando...", "checking...", "buscando...") \
    X(T_NOVA_DATA, "%s  NOVA (%s)", "%s  NEW (%s)", "%s  NUEVA (%s)") \
    X(T_EM_DIA, "%s  (em dia)", "%s  (up to date)", "%s  (al día)") \
    X(T_APP, "App instalador", "Installer app", "App instalador") \
    X(T_APP_REABRA, "%s  (reabra o app)", "%s  (reopen the app)", "%s  (reabre la app)") \
    X(T_APP_NOVA, "%s  (nova: %s)", "%s  (new: %s)", "%s  (nueva: %s)") \
    X(T_CONVITE, "Convite", "Invite", "Invitación") \
    X(T_NENHUM, "nenhum", "none", "ninguna") \
    X(T_CMD_INSTALAR, "Instalar o patch", "Install the patch", "Instalar el parche") \
    X(T_CMD_ATUALIZAR, "Atualizar o jogo", "Update the game", "Actualizar el juego") \
    X(T_CMD_REINSTALAR, "Reinstalar o patch", "Reinstall the patch", "Reinstalar el parche") \
    X(T_CMD_BAIXAR, "Baixar a nova versão", "Download the new version", "Descargar la versión nueva") \
    X(T_CMD_PROCURAR, "Procurar atualização", "Check for updates", "Buscar actualización") \
    X(T_CMD_CONVITE, "Convite do servidor", "Server invite", "Invitación del servidor") \
    X(T_CMD_REMOVER, "Remover o patch", "Remove the patch", "Quitar el parche") \
    X(T_RODAPE, BT_L " " BT_R "  abas        START  sair", BT_L " " BT_R "  tabs        START  exit", BT_L " " BT_R "  pestañas    START  salir") \
    X(T_NADA_JOGO, "nada do jogo vem dentro do app", "no game data ships in this app", "la app no trae nada del juego") \
    X(T_ABA_PATCH, "Patch atual", "Current patch", "Parche actual") \
    X(T_ABA_VERSOES, "Versões", "Versions", "Versiones") \
    X(T_SEM_PATCH, "Patch ainda não instalado", "Patch not installed yet", "Parche aún no instalado") \
    X(T_SEM_PATCH2, "Toque em Instalar (ou " BT_A ") para", "Tap Install (or " BT_A ") to install", "Toca Instalar (o " BT_A ") para") \
    X(T_SEM_PATCH3, "instalar o %s no seu MH3U.", "%s on your MH3U.", "instalar el %s en tu MH3U.") \
    X(T_INSTALADO_JOGO, "INSTALADO NO SEU JOGO", "INSTALLED IN YOUR GAME", "INSTALADO EN TU JUEGO") \
    X(T_O_QUE_FAZ, "O QUE FAZ", "WHAT IT DOES", "QUÉ HACE") \
    X(T_CONECTE, "Conecte à internet para ver os detalhes.", "Connect to the internet to see the details.", "Conéctate a internet para ver los detalles.") \
    X(T_NOVA_CAIXA, "Nova: %s (%s)", "New: %s (%s)", "Nueva: %s (%s)") \
    X(T_BAIXAR_BT, "Baixar " BT_B, "Download " BT_B, "Descargar " BT_B) \
    X(T_APP_TEM, "O app já tem o %s", "The app already has %s", "La app ya tiene el %s") \
    X(T_ATUALIZAR_BT, "Atualizar " BT_A, "Update " BT_A, "Actualizar " BT_A) \
    X(T_SEM_HIST, "Sem histórico ainda", "No history yet", "Aún sin historial") \
    X(T_SEM_HIST2, "Ele chega com a primeira busca", "It arrives with the first", "Llega con la primera búsqueda") \
    X(T_SEM_HIST3, "de atualização (precisa de internet).", "update check (needs internet).", "de actualización (requiere internet).") \
    X(T_COL_VERSAO, "Versão", "Version", "Versión") \
    X(T_COL_DATA, "Data", "Date", "Fecha") \
    X(T_COL_SITUACAO, "Situação", "Status", "Estado") \
    X(T_SIT_JOGO, "no seu jogo", "in your game", "en tu juego") \
    X(T_SIT_NOVA, "mais nova", "latest", "más nueva") \
    X(T_SIT_APP, "no app", "in the app", "en la app") \
    X(T_SIT_ANTERIOR, "anterior", "older", "anterior") \
    X(T_ROLAR, "cima/baixo: %d-%d de %d", "up/down: %d-%d of %d", "arriba/abajo: %d-%d de %d") \
    X(T_KB, "%lu de %lu KB", "%lu of %lu KB", "%lu de %lu KB") \
    X(T_VOLTAR, BT_B "  Voltar", BT_B "  Back", BT_B "  Volver") \
    X(T_Q_REMOVER, "Remover o patch?", "Remove the patch?", "¿Quitar el parche?") \
    X(T_Q_REMOVER2, "O MH3U volta a ser o original.", "MH3U goes back to the original.", "El MH3U vuelve a ser el original.") \
    X(T_REMOVER_BT, BT_A " Remover", BT_A " Remove", BT_A " Quitar") \
    X(T_CANCELAR_BT, BT_B " Cancelar", BT_B " Cancel", BT_B " Cancelar") \
    X(T_BT_INSTALAR, BT_A " Instalar", BT_A " Install", BT_A " Instalar") \
    X(T_BT_ATUALIZAR, BT_A " Atualizar jogo", BT_A " Update game", BT_A " Actualizar") \
    X(T_BT_REINSTALAR, BT_A " Reinstalar", BT_A " Reinstall", BT_A " Reinstalar") \
    X(T_BT_BAIXAR, BT_B " Baixar nova", BT_B " Download new", BT_B " Descargar") \
    X(T_BT_PROCURAR, BT_B " Procurar", BT_B " Check", BT_B " Buscar") \
    X(T_BT_CONVITE, BT_X " Convite", BT_X " Invite", BT_X " Invitación") \
    X(T_BT_REMOVER, BT_Y " Remover", BT_Y " Remove", BT_Y " Quitar") \
    X(T_PROCURANDO_ATU, "Procurando atualização...", "Checking for updates...", "Buscando actualización...") \
    X(T_ATUALIZACAO, "Atualização", "Update", "Actualización") \
    X(T_MANIF_INVALIDO, "manifesto inválido", "invalid manifest", "manifiesto inválido") \
    X(T_RECADO_NOVA, "Nova versão %s disponível: " BT_B " para baixar.", "New version %s available: " BT_B " to download.", "Versión nueva %s disponible: " BT_B " para descargar.") \
    X(T_DIFERENTE_ASSINADO, "%s: chegou diferente do assinado", "%s: does not match the signature", "%s: no coincide con la firma") \
    X(T_CONFERINDO_SHA, "conferindo (SHA-256)", "verifying (SHA-256)", "verificando (SHA-256)") \
    X(T_OK_KB, "%s  ok  (%lu KB, assinatura conferida)", "%s  ok  (%lu KB, signature verified)", "%s  ok  (%lu KB, firma verificada)") \
    X(T_INSTALANDO_APP, "instalando o app novo", "installing the new app", "instalando la app nueva") \
    X(T_BAIXANDO_PATCH, "Baixando o patch %s", "Downloading patch %s", "Descargando el parche %s") \
    X(T_DO_REPO, "Do repositório público do projeto (GitHub).", "From the project's public repository (GitHub).", "Del repositorio público del proyecto (GitHub).") \
    X(T_TUDO_CONFERIDO, "Tudo é conferido pela assinatura antes de usar.", "Everything is checked against the signature.", "Todo se verifica con la firma antes de usarse.") \
    X(T_RECEITA, "Receita", "Recipe", "Receta") \
    X(T_PATCH, "Patch", "Patch", "Parche") \
    X(T_APP_NOVO, "App novo", "New app", "App nueva") \
    X(T_RECEITA_INVALIDA, "Receita inválida.", "Invalid recipe.", "Receta inválida.") \
    X(T_GRAVANDO_SD, "gravando no cartão SD", "saving to the SD card", "guardando en la tarjeta SD") \
    X(T_PECA, "Peça %lu/%lu", "Part %lu/%lu", "Pieza %lu/%lu") \
    X(T_PECAS_FILA, "Conteúdo: %lu peças a baixar (%lu KB)", "Content: %lu parts to download (%lu KB)", "Contenido: %lu piezas a descargar (%lu KB)") \
    X(T_PECAS_OK, "Conteúdo completo no cartão (%lu peças).", "Content complete on the SD card (%lu parts).", "Contenido completo en la SD (%lu piezas).") \
    X(T_PECAS_FALTAM, "Falta conteúdo: conecte à internet e tente de novo.", "Content missing: connect to the internet and try again.", "Falta contenido: conéctate a internet e inténtalo de nuevo.") \
    X(T_GUARDADO, "Guardado no cartão SD.", "Saved to the SD card.", "Guardado en la tarjeta SD.") \
    X(T_NAO_GRAVEI, "Não gravei no SD: vale só até fechar o app.", "Not saved to SD: only until the app closes.", "No se guardó en la SD: vale hasta cerrar la app.") \
    X(T_APP_ATUALIZADO, "App atualizado para %s.", "App updated to %s.", "App actualizada a %s.") \
    X(T_APP_FALHOU, "Não consegui atualizar o app.", "Could not update the app.", "No pude actualizar la app.") \
    X(T_FALHOU_TROCADO, "Falhou: nada foi trocado", "Failed: nothing was changed", "Falló: no se cambió nada") \
    X(T_TENTE, "Tente de novo mais tarde.", "Try again later.", "Inténtalo más tarde.") \
    X(T_PATCH_BAIXADO, "Patch baixado", "Patch downloaded", "Parche descargado") \
    X(T_FECHE_ABRA, "Feche e abra o app; depois instale com " BT_A ".", "Close and reopen the app, then install with " BT_A ".", "Cierra y abre la app; luego instala con " BT_A ".") \
    X(T_AGORA_INSTALE, "Agora instale no jogo com " BT_A ".", "Now install it on the game with " BT_A ".", "Ahora instálalo en el juego con " BT_A ".") \
    X(T_GRAVANDO_UPDATE, "gravando o update no cartão", "writing the update to the card", "grabando la actualización") \
    X(T_ITEM_OK, "%s  ok", "%s  ok", "%s  ok") \
    X(T_ITEM_DIFERENTE, "%s  DIFERENTE (%s)", "%s  MISMATCH (%s)", "%s  DISTINTO (%s)") \
    X(T_INSTALANDO, "Instalando o patch %s", "Installing patch %s", "Instalando el parche %s") \
    X(T_NAO_DESLIGUE, "Não desligue o console.", "Do not turn off the console.", "No apagues la consola.") \
    X(T_LENDO_SD, "lendo o MH3U do cartão SD", "reading MH3U from the SD card", "leyendo el MH3U de la tarjeta SD") \
    X(T_LENDO_CART, "lendo o MH3U do cartucho", "reading MH3U from the game card", "leyendo el MH3U del cartucho") \
    X(T_NAO_LI, "Não consegui ler o jogo (0x%08lX).", "Could not read the game (0x%08lX).", "No pude leer el juego (0x%08lX).") \
    X(T_NAO_SUPORTADA, "Este MH3U não é a versão suportada", "This MH3U is not the supported version", "Este MH3U no es la versión compatible") \
    X(T_SEM_MEMORIA, "Sem memória.", "Out of memory.", "Sin memoria.") \
    X(T_DESCOMPRIMINDO, "descomprimindo o código do jogo", "decompressing the game code", "descomprimiendo el código del juego") \
    X(T_CONFERINDO_JOGO, "conferindo o jogo", "verifying the game", "verificando el juego") \
    X(T_COD_JOGO, "Código do jogo", "Game code", "Código del juego") \
    X(T_BANNER, "Banner", "Banner", "Banner") \
    X(T_ICONE, "Ícone", "Icon", "Icono") \
    X(T_JOGO_ERRADO, "O jogo não é o esperado (região/versão).", "Unexpected game (region/version).", "El juego no es el esperado (región/versión).") \
    X(T_APLICANDO, "aplicando o patch do online", "applying the online patch", "aplicando el parche en línea") \
    X(T_COD_PATCH, "Código com o patch", "Patched code", "Código con el parche") \
    X(T_CANCELADA, "%s: instalação cancelada.", "%s: installation cancelled.", "%s: instalación cancelada.") \
    X(T_REMOVENDO_ANTIGO, "removendo o patch antigo", "removing the old patch", "quitando el parche anterior") \
    X(T_ANTIGO_REMOVIDO, "Patch antigo removido.", "Old patch removed.", "Parche anterior quitado.") \
    X(T_ANTIGO_FICOU, "Não removi o antigo (0x%08lX); instalo por cima.", "Could not remove the old one (0x%08lX); installing over it.", "No quité el anterior (0x%08lX); instalo encima.") \
    X(T_INSTALADO, "Patch %s instalado!", "Patch %s installed!", "¡Parche %s instalado!") \
    X(T_LUMA_OFF, "%d arquivo(s) do plugin/Luma desligados (.off).", "%d plugin/Luma file(s) turned off (.off).", "%d archivo(s) del plugin/Luma desactivados (.off).") \
    X(T_ABRA, "Abra o MH3U: Ferry -> Multiplayer.", "Open MH3U: Ferry -> Multiplayer.", "Abre el MH3U: Ferry -> Multiplayer.") \
    X(T_OLD3DS, "Old 3DS: a tela preta ao abrir demora mais.", "Old 3DS: the black screen at boot takes longer.", "Old 3DS: la pantalla negra al abrir tarda más.") \
    X(T_SEM_CONVITE, "Sem convite: se o servidor pede, use " BT_X ".", "No invite: if the server needs one, use " BT_X ".", "Sin invitación: si el servidor la pide, usa " BT_X ".") \
    X(T_CONCLUIDO, "Concluído", "Done", "Listo") \
    X(T_FALHOU_INST, "Falhou: nada foi instalado", "Failed: nothing was installed", "Falló: no se instaló nada") \
    X(T_REMOVENDO, "Removendo o patch", "Removing the patch", "Quitando el parche") \
    X(T_VOLTOU_ORIGINAL, "O MH3U voltou ao original.", "MH3U is back to the original.", "El MH3U volvió al original.") \
    X(T_REMOVIDO, "Patch removido", "Patch removed", "Parche quitado") \
    X(T_NAO_REMOVIDO_COD, "0x%08lX (o patch estava instalado?)", "0x%08lX (was the patch installed?)", "0x%08lX (¿estaba instalado el parche?)") \
    X(T_NAO_REMOVIDO, "Não removido", "Not removed", "No se quitó") \
    X(T_CONVITE_DICA, "convite (ex.: K7M4-2QXZ)", "invite (e.g. K7M4-2QXZ)", "invitación (ej.: K7M4-2QXZ)") \
    X(T_CONVITE_INVALIDO, "Convite inválido: 8 letras/números (sem 0, O, 1, I, L).", "Invalid invite: 8 letters/digits (no 0, O, 1, I, L).", "Invitación inválida: 8 letras/números (sin 0, O, 1, I, L).") \
    X(T_CONVITE_GRAVADO, "Convite gravado: o MH3U libera a sua rede sozinho.", "Invite saved: MH3U unlocks your network by itself.", "Invitación guardada: el MH3U habilita tu red solo.") \
    X(T_CONVITE_NAO_GRAVOU, "Não consegui gravar o convite no cartão.", "Could not save the invite to the card.", "No pude guardar la invitación en la tarjeta.") \
    X(T_SEM_JOGO, "MH3U (EUA) não está no cartão SD nem no cartucho.", "MH3U (USA) is not on the SD card or game card.", "MH3U (EE. UU.) no está en la SD ni en el cartucho.") \
    X(T_EM_DIA_RECADO, "Você já tem a versão mais nova (%s).", "You already have the latest version (%s).", "Ya tienes la versión más nueva (%s).") \
    X(T_CORROMPIDO, "Instalador corrompido (receita/patch).", "Corrupted installer (recipe/patch).", "Instalador dañado (receta/parche).") \
    X(T_CORROMPIDO2, "Baixe o .cia de novo. START sai.", "Download the .cia again. START exits.", "Descarga el .cia de nuevo. START sale.") \
    X(T_R_SERVIDOR, "procurando o servidor", "looking up the server", "buscando el servidor") \
    X(T_R_CONECTANDO, "conectando", "connecting", "conectando") \
    X(T_R_BAIXANDO, "baixando", "downloading", "descargando") \
    X(T_E_SOC, "rede do console indisponível", "console network unavailable", "red de la consola no disponible") \
    X(T_E_DNS, "sem internet (DNS não respondeu)", "no internet (DNS did not answer)", "sin internet (el DNS no respondió)") \
    X(T_E_CONECTAR, "servidor não respondeu", "server did not answer", "el servidor no respondió") \
    X(T_E_TLS, "falha na conexão segura", "secure connection failed", "falló la conexión segura") \
    X(T_E_HTTP, "arquivo não encontrado no servidor", "file not found on the server", "archivo no encontrado en el servidor") \
    X(T_E_GRANDE, "arquivo maior que o esperado", "file larger than expected", "archivo más grande de lo esperado") \
    X(T_E_MEM, "sem memória", "out of memory", "sin memoria") \
    X(T_E_CORTADO, "download interrompido", "download interrupted", "descarga interrumpida") \
    X(T_E_CAMINHO, "caminho inválido", "invalid path", "ruta inválida") \
    X(T_E_REDE, "erro de rede", "network error", "error de red")

enum {
#define TX_ID(id, pt, en, es) id,
    TEXTOS(TX_ID)
#undef TX_ID
    N_TEXTOS
};

extern int g_idioma;
const char *T(int id);
/* escolhe pelo console (ou idioma= no sd:/mh3u-online.cfg) */
void idioma_escolher(void);
