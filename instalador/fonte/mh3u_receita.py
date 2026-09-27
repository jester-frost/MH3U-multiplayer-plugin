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

    mh3u_receita.py [--cia ~/mh3u-nc/cia/MH3U-online-<v>-pub-update-pequeno.cia]
                    [--saida instalador/romfs]

Gera: receita.bin, mh3u.bps (copia do patch pub). Confere reconstruindo o CIA a
partir da receita + os arquivos do jogo original (~/mh3u-nc/cia/exefsdir).

Formato de receita.bin (little-endian):
    "MH3UREC1"
    u32 versao_do_formato (1)
    char versao_patch[16]
    u32 tam_code_base_descomprimido, u32 tam_code_alvo
    u8  sha_code_base[32], sha_code_alvo[32]
    u8  sha_banner[32], sha_logo[32], sha_icone_base[32]
    u8  sha_cia[32]; u64 tam_cia
    u32 n_patch_icone; n x { u32 off, u16 len, bytes }
    u32 n_segmentos;   n x { u8 tipo, u32 len, [bytes se tipo == 'L'] }
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
    ap.add_argument("--saida", default=os.path.join(RAIZ, "instalador", "romfs"))
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
    marcas.sort()
    segs, pos = [], 0
    for ini, tam, tipo in marcas:
        if ini > pos: segs.append((b"L", cia[pos:ini]))
        segs.append((tipo, tam)); pos = ini + tam
    if pos < len(cia): segs.append((b"L", cia[pos:]))

    # confere: reconstroi a partir da receita + arquivos do jogo
    icone_nosso = bytearray(icone_base)
    for off, b in patch_icone: icone_nosso[off:off + len(b)] = b
    pedaco = {b"C": alvo_code, b"B": banner, b"G": logo, b"I": bytes(icone_nosso)}
    rec = b"".join(s[1] if s[0] == b"L" else pedaco[s[0]] for s in segs)
    assert rec == cia, "a receita nao reconstroi o CIA"

    out = bytearray(b"MH3UREC1") + struct.pack("<I", 1) + ver.encode()[:15].ljust(16, b"\0")
    out += struct.pack("<II", len(base_code), len(alvo_code))
    out += sha(base_code) + sha(alvo_code) + sha(banner) + sha(logo) + sha(icone_base)
    out += sha(cia) + struct.pack("<Q", len(cia))
    out += struct.pack("<I", len(patch_icone))
    for off, b in patch_icone: out += struct.pack("<IH", off, len(b)) + b
    out += struct.pack("<I", len(segs))
    lit = 0
    for tipo, x in segs:
        if tipo == b"L":
            out += tipo + struct.pack("<I", len(x)) + x; lit += len(x)
        else:
            out += tipo + struct.pack("<I", x)
    os.makedirs(a.saida, exist_ok=True)
    open(os.path.join(a.saida, "receita.bin"), "wb").write(out)
    open(os.path.join(a.saida, "mh3u.bps"), "wb").write(bps)
    print(f"receita.bin: {len(out)} B ({len(segs)} segmentos, {lit} B literais, "
          f"{len(patch_icone)} trechos no icone); mh3u.bps: {len(bps)} B")
    print(f"reconstrucao == CIA ({len(cia)} B, sha {sha(cia).hex()[:16]})")


if __name__ == "__main__":
    main()
