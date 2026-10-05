#!/usr/bin/env python3
"""Receita do update.cia para o INSTALADOR (instalador/, app de 3DS).

O update.cia do patch nativo tem, dentro, pedacos do jogo da Capcom (o .code,
o banner, o icone e o logo do ExeFS). Esses pedacos NAO vao no instalador: a
receita descreve o CIA byte a byte com

    L  literal (o que e nosso/gerado: cabecalhos, hashes, romfs minimo...)
    C  o .code do jogo base, descomprimido, com o code.bps aplicado
    B  o banner do jogo base          (ExeFS 'banner', intacto)
    (o logo vai literal: e um logo padrao do SDK, e varia entre copias)
    I  o icone do jogo base com os NOSSOS textos (patch de textos abaixo)

e o app, no console do jogador, le esses quatro arquivos do MH3U DELE, confere
os hashes, monta o CIA seguindo a receita, confere o SHA-256 final e instala.

    P  uma PECA do conteudo novo (1.4+): um arquivo do romfs do update,
       guardado a parte e nomeado pelo proprio SHA-256 (pecas/<sha>.bin)

    mh3u_receita.py [--cia ~/mh3u-nc/cia/MH3U-online-<v>-pub-update-pequeno.cia]
                    [--conteudo ~/mh3u-quests/saida/conteudo_patch]
                    [--saida instalador/romfs] [--pecas instalador/pecas]

Gera: receita2.bin (com o mh3u.bps dentro) e as pecas. Confere reconstruindo o
CIA a partir da receita + pecas + os arquivos do jogo original
(~/mh3u-nc/cia/exefsdir).

AS PECAS (fila de atualizacao): cada arquivo do conteudo com 64 KB ou mais sai
da receita e vira uma peca (o update tem o NCCH sem cifra -- NoCrypto --, o
arquivo esta la byte a byte). O instalador baixa so as pecas que ainda nao tem
no cartao (sdmc:/3ds/mh3u-online/pecas/): numa versao nova, o que nao mudou tem
o mesmo SHA e nao e baixado de novo. Cada peca fica bem abaixo dos 20 MB do
jsDelivr (o espelho).

Formato de receita2.bin (little-endian):
    "MH3UREC1"
    u32 versao_do_formato (2; a 1 nao tinha o bps nem pecas)
    char versao_patch[16]
    u32 tam_code_base_descomprimido, u32 tam_code_alvo
    u8  sha_code_base[32], sha_code_alvo[32]
    u8  sha_banner[32], sha_logo[32], sha_icone_base[32]
    u8  sha_cia[32]; u64 tam_cia
    u32 tam_bps; bytes do mh3u.bps
    u32 n_patch_icone; n x { u32 off, u16 len, bytes }
    u32 n_segmentos;   n x { u8 tipo, u32 len, [bytes se 'L'] [sha256 se 'P'] }
"""
import argparse, hashlib, os, struct, sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, "tools"))
import mh3u_nativo as N                                   # aplicar_bps, CODE_ORIG

CIA_DIR = os.path.expanduser("~/mh3u-nc/cia")
EXEFS_ORIG = os.path.join(CIA_DIR, "exefsdir")            # extraidos do CXI original
sha = lambda b: hashlib.sha256(b).digest()


def layout(cia):
    hsz, typ, ver, cc, tk, tmd, meta, csz = struct.unpack_from("<IHHIIIIQ", cia, 0)
    al = lambda x: (x + 63) & ~63
    o_c = al(hsz) + al(cc) + al(tk) + al(tmd)
    o_meta = o_c + al(csz)
    ncch = cia[o_c:o_c + csz]
    assert ncch[0x100:0x104] == b"NCCH"
    eo = struct.unpack_from("<I", ncch, 0x1A0)[0] * 0x200
    arqs = {}
    for i in range(10):
        nome = ncch[eo + i * 16:eo + i * 16 + 8].rstrip(b"\0").decode()
        if nome:
            off, tam = struct.unpack_from("<II", ncch, eo + i * 16 + 8)
            arqs[nome] = (o_c + eo + 0x200 + off, tam)
    return o_c, csz, o_meta, meta, arqs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cia", default=None)
    ap.add_argument("--bps", default=os.path.join(RAIZ, "plugin", "nativo", "saida-pub", "code.bps"))
    ap.add_argument("--conteudo", default=os.path.expanduser("~/mh3u-quests/saida/conteudo_patch"))
    ap.add_argument("--saida", default=os.path.join(RAIZ, "instalador", "romfs"))
    ap.add_argument("--pecas", default=os.path.join(RAIZ, "instalador", "pecas"))
    a = ap.parse_args()
    ver = N.versao()
    cia_p = a.cia or os.path.join(CIA_DIR, f"MH3U-online-{ver}-pub-update-pequeno.cia")
    cia = open(cia_p, "rb").read()
    bps = open(a.bps, "rb").read()

    base_code = open(N.CODE_ORIG, "rb").read()
    alvo_code = N.aplicar_bps(base_code, bps)
    banner = open(os.path.join(EXEFS_ORIG, "banner.bnr"), "rb").read()
    logo = open(os.path.join(EXEFS_ORIG, "logo.darc.lz"), "rb").read()
    icone_base = open(os.path.join(CIA_DIR, "icon.icn.orig"), "rb").read()

    o_c, csz, o_meta, meta_sz, arqs = layout(cia)
    # o que o CIA tem nos lugares dos arquivos do jogo
    assert cia[arqs[".code"][0]:][:arqs[".code"][1]] == alvo_code, "o .code do CIA nao e base+bps"
    assert cia[arqs["banner"][0]:][:arqs["banner"][1]] == banner
    assert cia[arqs["logo"][0]:][:arqs["logo"][1]] == logo
    icone = cia[arqs["icon"][0]:][:arqs["icon"][1]]
    assert len(icone) == len(icone_base)

    # patch do icone: so os trechos que mudaram (os nossos textos)
    patch_icone, i = [], 0
    while i < len(icone):
        if icone[i] == icone_base[i]:
            i += 1; continue
        j = i
        while j < len(icone) and (icone[j] != icone_base[j] or icone[j:j + 4] != icone_base[j:j + 4]): j += 1
        patch_icone.append((i, icone[i:j])); i = j
    assert all(len(b) <= 0xFFFF for _, b in patch_icone)

    # segmentos: onde aparecem .code/banner/logo/icone (o icone aparece de novo na meta)
    # O LOGO (vinheta de abertura) vai LITERAL: e um dos logos padrao do SDK
    # (o makerom embute: "Licensed"/"Nintendo"/...), e ha copias do MH3U EUA
    # com logos diferentes (27/09: a do .85 tem o "Nintendo", a nossa o
    # "Licensed") -- depender do logo do jogador recusava um jogo valido.
    marcas = [(arqs[".code"][0], arqs[".code"][1], b"C"),
              (arqs["banner"][0], arqs["banner"][1], b"B"),
              (arqs["icon"][0], arqs["icon"][1], b"I")]
    if meta_sz:
        k = cia.find(icone, o_meta)
        assert k >= 0, "icone nao achado na meta"
        marcas.append((k, len(icone), b"I"))
    # as pecas: os arquivos do conteudo, achados byte a byte no romfs do update
    o_romfs = o_c + struct.unpack_from("<I", cia, o_c + 0x1B0)[0] * 0x200
    assert cia[o_c + 0x18F] & 4, "o NCCH do update tem de ser sem cifra (NoCrypto)"
    pecas = {}
    for raiz, _, nomes in os.walk(a.conteudo):
        for nome in sorted(nomes):
            b = open(os.path.join(raiz, nome), "rb").read()
            if len(b) < 0x10000: continue
            k = cia.find(b, o_romfs)
            assert k >= 0, f"{nome} nao achado no romfs do update"
            marcas.append((k, len(b), b"P", sha(b))); pecas[sha(b)] = b
    marcas = [m if len(m) == 4 else m + (None,) for m in marcas]
    marcas.sort()
    segs, pos = [], 0
    for ini, tam, tipo, h in marcas:
        assert ini >= pos, "marcas sobrepostas"
        if ini > pos: segs.append((b"L", cia[pos:ini]))
        segs.append((tipo, tam, h)); pos = ini + tam
    if pos < len(cia): segs.append((b"L", cia[pos:]))

    # confere: reconstroi a partir da receita + arquivos do jogo
    icone_nosso = bytearray(icone_base)
    for off, b in patch_icone: icone_nosso[off:off + len(b)] = b
    pedaco = {b"C": alvo_code, b"B": banner, b"G": logo, b"I": bytes(icone_nosso)}
    rec = b"".join(s[1] if s[0] == b"L" else pecas[s[2]] if s[0] == b"P" else pedaco[s[0]] for s in segs)
    assert rec == cia, "a receita nao reconstroi o CIA"

    out = bytearray(b"MH3UREC1") + struct.pack("<I", 2) + ver.encode()[:15].ljust(16, b"\0")
    out += struct.pack("<II", len(base_code), len(alvo_code))
    out += sha(base_code) + sha(alvo_code) + sha(banner) + sha(logo) + sha(icone_base)
    out += sha(cia) + struct.pack("<Q", len(cia))
    out += struct.pack("<I", len(bps)) + bps
    out += struct.pack("<I", len(patch_icone))
    for off, b in patch_icone: out += struct.pack("<IH", off, len(b)) + b
    out += struct.pack("<I", len(segs))
    lit = 0
    for sg in segs:
        tipo, x = sg[0], sg[1]
        if tipo == b"L":
            out += tipo + struct.pack("<I", len(x)) + x; lit += len(x)
        elif tipo == b"P":
            out += tipo + struct.pack("<I", x) + sg[2]
        else:
            out += tipo + struct.pack("<I", x)
    os.makedirs(a.saida, exist_ok=True)
    for velho in ("receita.bin", "mh3u.bps"):          # formato 1 (ate a 1.3)
        if os.path.exists(os.path.join(a.saida, velho)): os.remove(os.path.join(a.saida, velho))
    open(os.path.join(a.saida, "receita2.bin"), "wb").write(out)
    os.makedirs(a.pecas, exist_ok=True)
    for h, b in pecas.items():
        open(os.path.join(a.pecas, h.hex() + ".bin"), "wb").write(b)
    print(f"receita2.bin: {len(out)} B ({len(segs)} segmentos, {lit} B literais, "
          f"{len(patch_icone)} trechos no icone, bps {len(bps)} B)")
    print(f"pecas: {len(pecas)} em {a.pecas}, {sum(map(len, pecas.values()))} B "
          f"(maior {max(map(len, pecas.values()), default=0)} B)")
    print(f"reconstrucao == CIA ({len(cia)} B, sha {sha(cia).hex()[:16]})")


if __name__ == "__main__":
    main()
