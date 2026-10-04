#!/usr/bin/env python3
"""MH3U 3DS com o multiplayer DENTRO do .code (E9) -- sem plugin loader, sem CTRPF.

Monta, a partir do jogo ORIGINAL e do blob do plugin nativo
(plugin/nativo: `make` -> mh3u-nativo-*.elf/.bin):

  code.bin      .code descomprimido = original + patchA-heap + ganchos + blob
  exheader.bin  o exheader com o segmento de dados maior (o blob mora nele)

    mh3u_nativo.py montar  [--elf plugin/nativo/mh3u-nativo-dev.elf] [--saida DIR]
    mh3u_nativo.py azahar  [--elf ...] [--dados ~/.local/share/azahar-emu]
                           -> instala como mod: load/mods/00040000000AE400/
    mh3u_nativo.py tirar   [--dados ...]       -> remove o mod
    mh3u_nativo.py update  [--elf ...]  -> o mesmo como UPDATE (0004000E000AE400, v1.0.0):
                              fica por cima do jogo instalado, desinstalavel; romfs
                              INTEIRO (o MH3U so monta `rom:`, ver docs/mh3u-nativo-e9.md)
    mh3u_nativo.py ips     [--elf ...]  -> o que se DISTRIBUI: code.ips (so as nossas
                              mudancas, sem o codigo da Capcom) + exheader.bin, p/
                              sd:/luma/titles/00040000000AE400/ (game patching do Luma)
    mh3u_nativo.py cia     [--elf ...] [--cia-dir ~/mh3u-nc/cia]
                           -> CIA do jogo inteiro com o nativo (3dstool + makerom,
                              em ~/.local/bin). E o jogo da Capcom: SO uso local.

Layout (MH3U US 00040000000AE400 v0):
  text 0x100000 | rodata 0xB00000 | data 0xB84000 (+bss ate 0xD99BB4)
  blob a partir de 0xD9A000: o bss original vira zeros NO ARQUIVO, o blob vem
  depois, e o bss novo = bss do blob + arena do malloc.

Ganchos: em cada alvo da tabela g_nativoDestinos (lida do binario do blob)
entra `ldr pc,[pc,#-4]; .word destino`; as 2 instrucoes originais vao para o
trampolim g_nativoTramp[i] = [orig0, orig1, ldr pc,[pc,#-4], alvo+8]. A entrada
do jogo (0x100000: `bl 0x100024`) vira `bl nativo_entrada`.
"""
import argparse, hashlib, os, shutil, struct, subprocess, sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TID = "00040000000AE400"
PONTO_ZERO = os.path.expanduser("~/mh3u-nc/ponto-zero")
CODE_ORIG = os.path.join(PONTO_ZERO, TID + ".dec.code.ORIG")
CXI_ORIG = os.path.join(PONTO_ZERO, "nocrypto.cxi.ORIG")
SHA_CODE = "80db9caa329faacfaa580e435fce99614029a4bc201b5a898f56ff8a9a2b9006"
HEAP_IPS = os.path.join(RAIZ, "patches", "patchA-heap.ips")
BASE = 0x100000
LDR_PC = 0xE51FF004            # ldr pc, [pc, #-4]
NM = os.path.join(os.environ.get("DEVKITARM", "/opt/devkitpro/devkitARM"), "bin", "arm-none-eabi-nm")


def u32(b, o): return struct.unpack_from("<I", b, o)[0]
def p32(b, o, v): struct.pack_into("<I", b, o, v & 0xFFFFFFFF)


def aplicar_ips(code, caminho):
    d = open(caminho, "rb").read()
    assert d[:5] == b"PATCH"
    i = 5
    while d[i:i + 3] != b"EOF":
        off = int.from_bytes(d[i:i + 3], "big"); n = int.from_bytes(d[i + 3:i + 5], "big"); i += 5
        if n == 0:
            rn = int.from_bytes(d[i:i + 2], "big"); code[off:off + rn] = d[i + 2:i + 3] * rn; i += 3
        else:
            code[off:off + n] = d[i:i + n]; i += n


def simbolos(elf):
    s = {}
    for linha in subprocess.check_output([NM, elf], text=True).splitlines():
        p = linha.split()
        if len(p) == 3: s[p[2]] = int(p[0], 16)
    return s


def exheader_orig():
    with open(CXI_ORIG, "rb") as f:
        h = f.read(0x200)
        assert h[0x100:0x104] == b"NCCH"
        return bytearray(f.read(0x800))


def imm_arm(v):
    """Codifica v como imediato ARM (8 bits rotacionados) ou None."""
    for rot in range(16):
        x = ((v << (2 * rot)) | (v >> (32 - 2 * rot))) & 0xFFFFFFFF if rot else v
        if x < 256: return (rot << 8) | x
    return None


ESTAGIO, ESTAGIO_FIM = 0xAFF600, 0xB00000     # sobra zerada no fim do text (0x9FF5D0..)
SVC_0x70 = 0xF4010000
DE_A_PADRAO = 0x100000                         # heap A cede (Azahar 27/09: 653 KB ok, sobe e conecta)


def estagio(entrada, base, tam):
    """Estagio de arranque no text: RWX no blob e salto p/ nativo_entrada (lr intacto)."""
    # A 0x70 NAO aceita o pseudo-handle 0xFFFF8001 (27/09, .85: prefetch abort
    # em 0xD9A000, IFSR 0xF) -- duplica-se o handle do processo primeiro.
    # r11/r12 = resultado da 0x27/0x70: aparecem no dump do Luma se falhar.
    ins = [
        0xE92D403F,     # 0  push {r0-r5, lr}
        0xE59F102C,     # 1  ldr r1, =0xFFFF8001
        0xEF000027,     # 2  svc 0x27             svcDuplicateHandle -> r1
        0xE1A0B000,     # 3  mov r11, r0
        0xE1A00001,     # 4  mov r0, r1           handle real do processo
        0xE59F1020,     # 5  ldr r1, =base
        0xE1A02001,     # 6  mov r2, r1
        0xE59F301C,     # 7  ldr r3, =tam
        0xE3A04006,     # 8  mov r4, #6           MEMOP_PROT
        0xE3A05007,     # 9  mov r5, #7           RWX
        0xEF000070,     # 10 svc 0x70             svcControlProcessMemory
        0xE1A0C000,     # 11 mov r12, r0
        0xE8BD403F,     # 12 pop {r0-r5, lr}
        0xE59FF008,     # 13 ldr pc, =nativo_entrada
        0xFFFF8001, base, tam, entrada,     # 14..17
    ]
    for i, alvo in ((1, 14), (5, 15), (7, 16), (13, 17)):
        assert (ins[i] & 0xFFF) == (alvo - i - 2) * 4, (i, hex(ins[i]))
    return b"".join(struct.pack("<I", x) for x in ins)


def pc_relativa(ins):
    """True se a instrucao ARM le o PC (nao pode ir para o trampolim)."""
    if (ins >> 25) & 7 == 5: return True                 # b / bl
    if (ins >> 16) & 0xF == 0xF: return True              # Rn = pc
    if (ins >> 12) & 0xF == 0xF: return True              # Rd = pc
    if (ins >> 26) & 3 == 0 and not (ins >> 25) & 1 and (ins & 0xF) == 0xF: return True   # Rm = pc (imediato nao tem Rm)
    return False


def montar(elf, saida, modo80=False, de_a=None, n3ds_rapido=False):
    binario = elf[:-4] + ".bin"
    code = bytearray(open(CODE_ORIG, "rb").read())
    if hashlib.sha256(code).hexdigest() != SHA_CODE:
        sys.exit("o .code original nao confere com o ponto zero")
    ex = exheader_orig()
    t_addr, t_pag, t_tam = struct.unpack_from("<III", ex, 0x10)
    r_addr, r_pag, r_tam = struct.unpack_from("<III", ex, 0x20)
    d_addr, d_pag, d_tam = struct.unpack_from("<III", ex, 0x30)
    bss = u32(ex, 0x3C)
    assert (t_addr, r_addr, d_addr) == (0x100000, 0xB00000, 0xB84000), "nao e o MH3U US v0"
    assert len(code) == (t_pag + r_pag + d_pag) * 0x1000

    aplicar_ips(code, HEAP_IPS)

    sim = simbolos(elf)
    blob = bytearray(open(binario, "rb").read())
    base = sim["__start__"] if "__start__" in sim else sim["nativo_entrada"]
    fim_bss_jogo = d_addr + d_tam + bss
    assert base >= fim_bss_jogo and base % 0x1000 == 0, hex(base)
    fim = sim["__end__"]

    # ganchos
    tab = sim["g_nativoDestinos"] - base
    tramp = sim["g_nativoTramp"] - base
    n = 0
    while True:
        alvo, fn = u32(blob, tab + n * 8), u32(blob, tab + n * 8 + 4)
        if not alvo: break
        o = alvo - BASE
        i0, i1 = u32(code, o), u32(code, o + 4)
        if i0 >> 16 != 0xE92D or pc_relativa(i1):
            sys.exit(f"gancho {alvo:#x}: instrucoes inesperadas {i0:08x} {i1:08x}")
        for k, v in enumerate((i0, i1, LDR_PC, alvo + 8)):
            p32(blob, tramp + n * 16 + k * 4, v)
        p32(code, o, LDR_PC); p32(code, o + 4, fn)
        n += 1

    # entrada: bl 0x100024 -> bl ESTAGIO (no text, executavel) -> nativo_entrada.
    # No hardware o segmento de dados e XN: o estagio pede ao kernel RWX para o
    # blob (svcControlProcessMemory, MEMOP_PROT) antes do salto. O Azahar nao
    # aplica XN, por isso funcionava sem isto.
    assert u32(code, 0) == 0xEB000007, "entrada inesperada"
    ent = sim["nativo_entrada"]
    tam_rwx = ((fim - base) + 0xFFF) & ~0xFFF
    est = estagio(ent, base, tam_rwx)
    assert len(est) <= ESTAGIO_FIM - ESTAGIO and not any(code[ESTAGIO - BASE:ESTAGIO_FIM - BASE])
    code[ESTAGIO - BASE:ESTAGIO - BASE + len(est)] = est
    p32(code, 0, 0xEB000000 | (((ESTAGIO - (BASE + 8)) >> 2) & 0xFFFFFF))

    # segmento de dados novo: dados originais + zeros (bss antigo) + blob
    ini_dados = (t_pag + r_pag) * 0x1000
    novo_d_tam = (base - d_addr) + len(blob)
    novo_d_pag = (novo_d_tam + 0xFFF) // 0x1000
    dados = bytearray(novo_d_pag * 0x1000)
    dados[:d_tam] = code[ini_dados:ini_dados + d_tam]
    dados[base - d_addr:base - d_addr + len(blob)] = blob
    code = code[:ini_dados] + dados
    novo_bss = fim - (d_addr + novo_d_tam)

    # MEMORIA. O crt0 do jogo (0x1001F8) faz heap = min(total,64MB) - usado
    # - 0x2BC0000 - 0x2E400: o blob sai do heap principal do jogo.
    # --modo80: modo de 80 MB no exheader + desconto do blob na constante (o
    # jogo fica com o heap de sempre). So vale no Azahar/New 3DS: no Old 3DS o
    # NS escolhe o modo no boot e o exheader.bin do Luma nao muda (27/09, .85:
    # data abort por falta de heap). Padrao: SEM, igual ao console.
    extra = fim - fim_bss_jogo
    desconto = (extra + 0x3FFFF) & ~0x3FFFF
    SUB = 0x10021C                          # sub r0, r0, #0x2bc0000  (conta do heap A)
    LIT_B = 0x100240                        # .word 0x2BEE000         (heap B, linear)
    assert u32(code, SUB - BASE) == 0xE24007AF, "conta do heap inesperada"
    assert u32(code, LIT_B - BASE) == 0x02BEE000, "heap B inesperado"
    imm = imm_arm(0x2BC0000 - desconto)
    assert imm is not None, hex(0x2BC0000 - desconto)
    p32(code, SUB - BASE, 0xE2400000 | imm)
    de_a = DE_A_PADRAO if de_a is None else de_a
    if not modo80:
        # A parte que o heap A aguenta (a sobra depois dos sub-heaps fixos do
        # arranque, medida no Azahar) sai dele; o resto, do heap B.
        tira_a = min(desconto, de_a)
        imm = imm_arm(0x2BC0000 - (desconto - tira_a))
        assert imm is not None, hex(0x2BC0000 - (desconto - tira_a))
        p32(code, SUB - BASE, 0xE2400000 | imm)
        desconto -= tira_a
        # PADRAO (Old 3DS, 64 MB): a nossa parte sai do heap B (linear, 44 MB) e
        # o heap A fica do tamanho de sempre -- o jogo reparte o A em sub-heaps
        # fixos no arranque e nao tem folga de 1 MB (Azahar 27/09: escrita em
        # 0xBEE00, heap nulo).
        p32(code, LIT_B - BASE, 0x2BEE000 - desconto)
    if modo80:
        # Modo de memoria do OLD 3DS = flag0 (0x20E) bits 4-7; 3 = "Dev2" 80 MB,
        # igual ao MH4U (flag0 0x34). O flag2 (0x20D) e o modo do NEW 3DS -- foi
        # o que eu mexia antes (so o Azahar/N3DS via). O MH3U limita a conta do
        # heap a 64 MB, entao o jogo fica com os heaps de sempre e o blob usa a
        # sobra ate 80 MB.
        for o in (0x20E, 0x60E):            # flag0 (ACI e a copia)
            ex[o] = (ex[o] & 0x0F) | 0x30

    if n3ds_rapido:
        # New 3DS: cache L2 + 804 MHz (flag1 bits 0-1), como o MH4U (flag1 0x3).
        # O Old 3DS ignora.
        for o in (0x20C, 0x60C):
            ex[o] |= 0x3
    for o in (0x370, 0x770):            # capacidades de kernel (ACI e a copia)
        kc = list(struct.unpack_from("<28I", ex, o))
        if SVC_0x70 not in kc:
            kc[kc.index(0xFFFFFFFF)] = SVC_0x70
        struct.pack_into("<28I", ex, o, *kc)
    struct.pack_into("<III", ex, 0x30, d_addr, novo_d_pag, novo_d_tam)
    p32(ex, 0x3C, novo_bss)
    ex[0x0D] &= ~1                       # code.bin descomprimido

    os.makedirs(saida, exist_ok=True)
    open(os.path.join(saida, "code.bin"), "wb").write(code)
    open(os.path.join(saida, "exheader.bin"), "wb").write(ex)
    print(f"{n} ganchos + entrada; blob {len(blob)} B em {base:#x}; "
          f"dados {d_tam:#x}->{novo_d_tam:#x}, bss {bss:#x}->{novo_bss:#x}; "
          f"{extra / 1024:.0f} KB; " + ("modo 80 MB" if modo80 else f"heap A -{tira_a:#x}, heap B -{desconto:#x}"))
    print(f"-> {saida}/code.bin, exheader.bin")
    return saida


def fazer_ips(saida):
    """code.ips = diferencas entre o .code ORIGINAL e o nosso code.bin (inclusive
    o blob, que fica ALEM do fim do original -- o exheader.bin aumenta o segmento
    de dados e o loader do Luma aloca o tamanho novo antes de aplicar o IPS)."""
    orig = open(CODE_ORIG, "rb").read()
    novo = open(os.path.join(saida, "code.bin"), "rb").read()
    base = orig + bytes(max(0, len(novo) - len(orig)))
    regs, i = [], 0
    while i < len(novo):
        if novo[i] == base[i]:
            i += 1; continue
        j = i
        while j < len(novo):                     # junta diferencas proximas (< 8 iguais)
            if novo[j] != base[j]:
                j += 1; continue
            k = j
            while k < len(novo) and k - j < 8 and novo[k] == base[k]: k += 1
            if k < len(novo) and k - j < 8: j = k
            else: break
        ini = i
        if ini == 0x454F46: ini -= 1             # o offset "EOF" encerraria o IPS
        for o in range(ini, j, 0xFFFF):
            fim = min(j, o + 0xFFFF)
            if o == 0x454F46: o -= 1
            regs.append((o, novo[o:fim]))
        i = j
    ips = bytearray(b"PATCH")
    for o, d in regs:
        assert o != 0x454F46 and o <= 0xFFFFFF and 0 < len(d) <= 0xFFFF
        ips += o.to_bytes(3, "big") + len(d).to_bytes(2, "big") + d
    ips += b"EOF"
    open(os.path.join(saida, "code.ips"), "wb").write(ips)
    # confere: original + ips == code.bin
    teste = bytearray(base)
    aplicar_ips(teste, os.path.join(saida, "code.ips"))
    assert bytes(teste) == novo, "o IPS nao reproduz o code.bin"
    print(f"code.ips: {len(ips)} B, {len(regs)} registros (confere com o code.bin)")


def exheader_p_patch(saida):
    """No formato de PATCH (code.bps/code.ips) o codigo vem do jogo original, que
    e COMPRIMIDO: o exheader tem de manter a flag (bit 0 de 0x0D). Desligada, o
    loader nao descomprime e aplica o patch sobre os bytes comprimidos (Azahar:
    'patch feito para outro aplicativo', CRC da origem nao bate)."""
    p = os.path.join(saida, "exheader.bin")
    ex = bytearray(open(p, "rb").read())
    ex[0x0D] |= exheader_orig()[0x0D] & 1
    open(p, "wb").write(ex)


def fazer_bps(saida):
    """code.bps: como o IPS, so as nossas mudancas -- mas o BPS declara o tamanho
    FINAL, entao cresce o arquivo (o blob fica alem do fim do .code original).
    O Azahar recusa IPS que escreve alem do fim (Error 12); Luma e Azahar leem BPS.
    Formato: 'BPS1', tamanhos (varint), acoes SourceRead(0)/TargetRead(1), CRC32s."""
    import zlib
    orig = open(CODE_ORIG, "rb").read()
    novo = open(os.path.join(saida, "code.bin"), "rb").read()
    def vi(n):
        b = bytearray()
        while True:
            x = n & 0x7F; n >>= 7
            if n == 0: b.append(0x80 | x); return bytes(b)
            b.append(x); n -= 1
    out = bytearray(b"BPS1") + vi(len(orig)) + vi(len(novo)) + vi(0)
    i = 0
    tco = 0                                                # cursor do TargetCopy
    while i < len(novo):
        j = i
        if i < len(orig) and novo[i] == orig[i]:
            while j < len(novo) and j < len(orig) and novo[j] == orig[j]: j += 1
            out += vi(((j - i - 1) << 2) | 0)            # SourceRead
        else:
            # literal ate aparecer um trecho igual de >= 8 bytes
            while j < len(novo):
                if j < len(orig) and novo[j:j + 8] == orig[j:j + 8] and j + 8 <= len(orig): break
                j += 1
            k = i
            while k < j:                                   # zeros longos: RLE com TargetCopy
                z = k
                while z < j and novo[z] == 0: z += 1
                if z - k >= 32:
                    out += vi((0 << 2) | 1) + b"\0"        # TargetRead de 1 zero
                    # TargetCopy (3) de z-k-1 bytes a partir do zero recem-escrito:
                    # offset relativo ao cursor de copia (tco), com sinal no bit 0
                    rel = k - tco
                    out += vi(((z - k - 2) << 2) | 3) + vi((abs(rel) << 1) | (1 if rel < 0 else 0))
                    tco = k + (z - k - 1)
                    k = z
                    continue
                q = k
                while q < j and not (novo[q] == 0 and novo[q:q + 32] == bytes(min(32, j - q)) and j - q >= 32): q += 1
                if q > k:
                    out += vi(((q - k - 1) << 2) | 1) + novo[k:q]
                k = q
        i = j
    out += struct.pack("<I", zlib.crc32(orig)) + struct.pack("<I", zlib.crc32(novo))
    out += struct.pack("<I", zlib.crc32(bytes(out)))
    open(os.path.join(saida, "code.bps"), "wb").write(out)
    # confere aplicando
    assert aplicar_bps(orig, bytes(out)) == novo, "o BPS nao reproduz o code.bin"
    print(f"code.bps: {len(out)} B (confere com o code.bin)")


def aplicar_bps(src, p):
    import zlib
    assert p[:4] == b"BPS1"
    pos = 4
    def rd():
        nonlocal pos
        d, sh = 0, 1
        while True:
            x = p[pos]; pos += 1
            d += (x & 0x7F) * sh
            if x & 0x80: return d
            sh <<= 7; d += sh
    ss, ts, ms = rd(), rd(), rd(); pos += ms
    out = bytearray(); so = to = 0
    while pos < len(p) - 12:
        d = rd(); cmd, n = d & 3, (d >> 2) + 1
        if cmd == 0: out += src[len(out):len(out) + n]
        elif cmd == 1: out += p[pos:pos + n]; pos += n
        elif cmd == 3:
            d = rd(); to += (-1 if d & 1 else 1) * (d >> 1)
            for _ in range(n): out.append(out[to]); to += 1
        else: raise ValueError("acao BPS nao usada aqui")
    assert len(out) == ts and zlib.crc32(bytes(out)) == struct.unpack_from("<I", p, len(p) - 8)[0]
    return bytes(out)


def versao():
    return subprocess.check_output(["make", "-s", "-C", os.path.join(RAIZ, "plugin", "nativo"), "versao"],
                                   text=True).strip()


def smdh_com_versao(icn, ver, variante):
    """Mesmo icone do jogo; titulos com a versao do patch (todos os idiomas).
    SMDH: 16 x [curto 0x80 | longo 0x100 | editora 0x80] em UTF-16 a partir de 0x8."""
    d = bytearray(icn)
    curto = f"MH3U Online {ver}" + (" dev" if variante == "dev" else "")
    longo = f"MONSTER HUNTER 3 ULTIMATE\nOnline {ver}: salas NEX, P2P+relay, 4 jogadores"
    editora = "CAPCOM | patch MH3U Online"
    for campo, lim in ((curto, 0x40), (longo, 0x80), (editora, 0x40)):
        assert len(campo) < lim, (campo, len(campo))
    for i in range(16):
        o = 8 + i * 0x200
        for rel, tam, txt in ((0, 0x80, curto), (0x80, 0x100, longo), (0x180, 0x80, editora)):
            b = txt.encode("utf-16le")
            d[o + rel:o + rel + tam] = b + bytes(tam - len(b))
    return bytes(d)


LEIAME = """MH3U Online -- patch nativo {ver} ({variante})
==========================================================

Arquivo: {cia}
Jogo:    Monster Hunter 3 Ultimate (EUA), 00040000000AE400, v0
Gerado:  tools/mh3u_nativo.py cia  (repositorio MHXX-LOCAL, commit {commit})

O QUE E
  O multiplayer online do MH3U de 3DS (sem Wii U) embutido no proprio jogo:
  NAO precisa de plugin loader, de .3gx nem de "game patching" do Luma.

O QUE INCLUI E FAZ
  - Multiplayer local (Wi-Fi local/UDS) vira online: o jogo continua achando
    que esta no multiplayer local; os 17 pedidos de UDS sao respondidos pelo
    patch e o "modo local" do ndm e simulado para o Wi-Fi seguir ligado.
  - Salas no servidor NEX (matchmaking do mh3u-revival): hospedar, buscar e
    entrar pela tela normal do Port Tanzia (Ferry -> Multiplayer).
  - Partida P2P direta entre consoles; relay do servidor quando o P2P nao
    abre em ~6 s (ex.: mesma casa sem hairpin).
  - Ate 4 jogadores (o dono e o hub); sala cheia recusa na entrada com o
    erro do proprio jogo -- ninguem que ja joga e expulso.
  - Troca de dono: se o dono sai, outro herda a sala e quem saiu pode voltar;
    a sala e uma so (atualiza o status, nao duplica).
  - Robustez: pedidos ao servidor reenviados, reconexao automatica,
    batimento contra o limpador de salas do servidor.
  - Servidor: sd:/mh3u-online.cfg (servidor=IP) > DNS do console para
    mh3u3ds.pretendo.cc > {ip} embutido.
  - Patch de heap (antes o code.ips) ja incluido; modo de memoria de 80 MB
    para caber o codigo do online sem tirar memoria do jogo.
{extra}
TECNICO
  Blob nativo em 0xD9A000 (depois do bss do jogo); entrada no 0x100000
  antes do crt0; 19 ganchos (17 UDS + 2 ndm) com trampolim; heap do jogo
  compensado. Detalhes: docs/mh3u-p2p-direto-plano.md (E9).

AVISO
  Contem o jogo da Capcom: uso pessoal, com a sua propria copia. Nao
  distribuir. Save do jogo nao e tocado (mesmo titulo).
"""


TID_UPDATE = 0x0004000E000AE400
def versao_titulo():
    """Versao do TITULO (TMD) a partir da VERSAO do Makefile: '1.0-betaN' -> 1.0.N.
    Aparece no FBI/HOME -- da p/ saber qual update/instalador esta instalado."""
    import re
    m = re.match(r"(\d+)\.(\d+)(?:-beta(\d+))?", versao())
    ma, mi, mc = int(m.group(1)), int(m.group(2)), int(m.group(3) or 0)
    return (ma << 10) | (mi << 4) | mc


def fazer_cia(saida, d, nome, update=False, romfs_minimo=False):
    """CXI original -> troca exheader + code do ExeFS -> CXI -> CIA."""
    os.makedirs(d, exist_ok=True)
    x = lambda *c: subprocess.run(c, cwd=d, check=True, stdout=subprocess.DEVNULL)
    if not os.path.exists(os.path.join(d, "romfs.bin")):
        x("3dstool", "-xtf", "cxi", CXI_ORIG, "--header", "ncch.hdr", "--exh", "exh.bin",
          "--plain", "plain.bin", "--exefs", "exefs.bin", "--romfs", "romfs.bin")
        x("3dstool", "-xtf", "exefs", "exefs.bin", "--exefs-dir", "exefsdir", "--header", "exefs.hdr")
    shutil.copy(os.path.join(saida, "code.bin"), os.path.join(d, "exefsdir", "code.bin"))
    ver, variante = versao(), ("pub" if nome.endswith("pub") else "dev")
    icn_orig = os.path.join(d, "icon.icn.orig")
    if not os.path.exists(icn_orig):
        shutil.copy(os.path.join(d, "exefsdir", "icon.icn"), icn_orig)
    open(os.path.join(d, "exefsdir", "icon.icn"), "wb").write(
        smdh_com_versao(open(icn_orig, "rb").read(), ver, variante))
    shutil.copy(os.path.join(saida, "exheader.bin"), os.path.join(d, "exh-nativo.bin"))
    x("3dstool", "-ctf", "exefs", "exefs-nativo.bin", "--exefs-dir", "exefsdir", "--header", "exefs.hdr")
    hdr = "ncch.hdr"
    if update:
        # Como o update OFICIAL (MH4U v1.3): o console so troca o base pelo
        # update se a "versao remaster" do exheader (0x0E) for MAIOR que a do
        # base (0). Com 0 ele ignorava o update e abria o jogo original (27/09,
        # .85/.29: co-op local, nada no servidor; o Azahar nao confere). Jump ID
        # (0x1C8) = o proprio update, tambem como no oficial.
        exu = bytearray(open(os.path.join(d, "exh-nativo.bin"), "rb").read())
        struct.pack_into("<H", exu, 0x0E, 1)
        struct.pack_into("<Q", exu, 0x1C8, TID_UPDATE)
        open(os.path.join(d, "exh-nativo.bin"), "wb").write(exu)
        # o NCCH do update leva o ID de update (particao 0x108, programa 0x118);
        # o exheader continua com o programa BASE -> mesmo save do jogo
        h = bytearray(open(os.path.join(d, "ncch.hdr"), "rb").read())
        assert h[0x100:0x104] == b"NCCH"
        struct.pack_into("<Q", h, 0x108, TID_UPDATE)
        struct.pack_into("<Q", h, 0x118, TID_UPDATE)
        h[0x150:0x160] = b"CTR-U-AMHE".ljust(16, b"\0")    # codigo de produto de update
        hdr = "ncch-update.hdr"
        open(os.path.join(d, hdr), "wb").write(h)
    romfs = "romfs.bin"
    if romfs_minimo:
        # Update PEQUENO: o jogo le o romfs do jogo BASE (SelfNCCH RomFS); o do
        # update e outro arquivo (UpdateRomFS, o 'patch:' do MH4U) que o MH3U nao
        # usa. Entao o update leva so um romfs minimo.
        rd = os.path.join(d, "romfs-min"); os.makedirs(rd, exist_ok=True)
        open(os.path.join(rd, "mh3u-online.txt"), "w").write("MH3U online nativo -- romfs do update (vazio de proposito)\n")
        x("3dstool", "-ctf", "romfs", "romfs-min.bin", "--romfs-dir", "romfs-min")
        romfs = "romfs-min.bin"
    x("3dstool", "-ctf", "cxi", "mh3u-nativo.cxi", "--header", hdr, "--exh", "exh-nativo.bin",
      "--plain", "plain.bin", "--exefs", "exefs-nativo.bin", "--romfs", romfs,
      # SEM isto o 3dstool CIFRA o NCCH (chave normal) e zera a NoCrypto: o
      # Azahar recusa ("Blocked unauthorized encrypted CIA installation")
      "--not-encrypt")
    cia = os.path.join(d, f"MH3U-online-{ver}-{variante}" + ("-update" if update else "")
                       + ("-pequeno" if romfs_minimo else "") + ".cia")
    extra_mk = ["-ver", str(versao_titulo())] if update else []
    subprocess.run(["makerom", "-f", "cia", "-o", cia, "-content", "mh3u-nativo.cxi:0:0", "-ignoresign"] + extra_mk,
                   cwd=d, check=True, stdout=subprocess.DEVNULL)
    os.remove(os.path.join(d, "mh3u-nativo.cxi"))
    commit = subprocess.check_output(["git", "-C", RAIZ, "rev-parse", "--short", "HEAD"], text=True).strip()
    extra = ("  - Build DEV (emuladores): log de diagnostico em sd:/mh3u-online.log.\n"
             if variante == "dev" else
             "  - Build de PUBLICACAO: sem log nem diagnostico.\n")
    open(cia[:-4] + ".LEIA-ME.txt", "w").write(LEIAME.format(
        ver=ver, variante=variante, cia=os.path.basename(cia), commit=commit,
        ip="24.199.103.160", extra=extra))
    print("CIA:", cia)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("acao", choices=["montar", "azahar", "tirar", "cia", "update", "ips", "bps"])
    ap.add_argument("--romfs-minimo", action="store_true", help="update pequeno: sem o romfs do jogo (le o do base)")
    ap.add_argument("--cia-dir", default=os.path.expanduser("~/mh3u-nc/cia"))
    ap.add_argument("--elf", default=os.path.join(RAIZ, "plugin", "nativo", "mh3u-nativo-dev.elf"))
    ap.add_argument("--saida", default=os.path.join(RAIZ, "plugin", "nativo", "saida"))
    ap.add_argument("--sem-modo80", dest="modo80", action="store_false",
                    help="NAO usar o modo 80 MB: o blob sai do heap do jogo (--de-a/heap B). So p/ teste")
    ap.add_argument("--n3ds-rapido", action="store_true", help="New 3DS: L2 + 804 MHz (flag1 = 3, como o MH4U)")
    ap.add_argument("--de-a", type=lambda x: int(x, 0), default=None, help="bytes tirados do heap A (resto do B)")
    ap.add_argument("--dados", default=os.path.expanduser("~/.local/share/azahar-emu"))
    a = ap.parse_args()
    mod = os.path.join(a.dados, "load", "mods", TID)
    if a.acao == "tirar":
        for f in ("exheader.bin", "exefs/code.bin"):
            p = os.path.join(mod, f)
            if os.path.exists(p): os.remove(p); print("removido", p)
        return
    s = montar(a.elf, a.saida, a.modo80, a.de_a, a.n3ds_rapido)
    if a.acao == "ips":
        fazer_ips(s); exheader_p_patch(s)
    if a.acao == "bps":
        fazer_bps(s); exheader_p_patch(s)
    if a.acao in ("cia", "update"):
        fazer_cia(s, a.cia_dir, os.path.basename(a.elf)[:-4], update=(a.acao == "update"),
                  romfs_minimo=a.romfs_minimo)
    if a.acao == "azahar":
        os.makedirs(os.path.join(mod, "exefs"), exist_ok=True)
        shutil.copy(os.path.join(s, "code.bin"), os.path.join(mod, "exefs", "code.bin"))
        shutil.copy(os.path.join(s, "exheader.bin"), os.path.join(mod, "exheader.bin"))
        print("mod instalado em", mod)
        print("ATENCAO: tire o .3gx de sdmc/luma/plugins/%s e o code.ips de sdmc/luma/titles/%s" % (TID, TID))


if __name__ == "__main__":
    main()
