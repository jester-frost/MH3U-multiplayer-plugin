#!/usr/bin/env python3
"""Grava a versao do titulo no TMD de um .cia (e confere).

    cia_versao.py <arquivo.cia> <versao>     versao: numero (1056) ou "1.2" / "1.0-beta7"

O makerom ZERA a parte do meio da versao (com -ver ou -major/-minor): a 1.1
saiu como 1.0.0 (04/10/2026) -- menor que a 1.0-beta7 (1.0.7) ja instalada,
e o 3DS recusa gravar o update por cima. O Guild Hunter ja contornava assim
(build.sh). O campo nao entra em hash nenhum do conteudo, e a assinatura de
um .cia nosso ja nao e valida. Formato da versao: maior<<10 | menor<<4 | micro.
"""
import re, struct, sys


def numero(versao):
    """ "1.2" -> 1056; "1.0-beta7" -> 1031; numero -> ele mesmo. """
    if isinstance(versao, int) or str(versao).isdigit():
        return int(versao)
    m = re.match(r"(\d+)\.(\d+)(?:-beta(\d+))?$", str(versao))
    if not m:
        raise ValueError(f"versao invalida: {versao!r}")
    return (int(m[1]) << 10) | (int(m[2]) << 4) | int(m[3] or 0)


def _pos_tmd(f):
    hs, _, _, cs, ts, _ = struct.unpack_from('<IHHIII', f.read(0x20), 0)
    al = lambda x: (x + 63) // 64 * 64
    tmd = al(hs) + al(cs) + al(ts)
    f.seek(tmd)
    if struct.unpack('>I', f.read(4))[0] != 0x10004:      # RSA-2048 SHA-256
        raise SystemExit('TMD com assinatura de tipo inesperado; versao nao gravada')
    return tmd + 0x140 + 0x9C


def ler(caminho):
    with open(caminho, 'rb') as f:
        f.seek(_pos_tmd(f))
        return struct.unpack('>H', f.read(2))[0]


def gravar(caminho, versao):
    v = numero(versao)
    with open(caminho, 'r+b') as f:
        f.seek(_pos_tmd(f))
        f.write(struct.pack('>H', v))
    if ler(caminho) != v:
        raise SystemExit(f'{caminho}: a versao nao ficou gravada')
    return v


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    v = gravar(sys.argv[1], sys.argv[2])
    print(f'{sys.argv[1]}: versao do titulo {v >> 10}.{(v >> 4) & 63}.{v & 15} ({v})')
