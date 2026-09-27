#!/usr/bin/env python3
"""Arte do instalador (nossa, nada da Capcom): icone 48x48, banner 256x128 e o
jingle do banner. Espada de cacador cruzada com ondas de sinal (o "online"),
noite azul e laranja de fogueira.

    python3 arte/gerar_arte.py      -> arte/icone.png, arte/banner.png, arte/jingle.wav
"""
import math, os, struct, wave
from PIL import Image, ImageDraw, ImageFilter, ImageFont

AQUI = os.path.dirname(os.path.abspath(__file__))
NOITE_A, NOITE_B = (14, 24, 48), (22, 78, 98)
FOGO, FOGO_CLARO = (255, 138, 36), (255, 206, 120)
ACO, ACO_ESC = (222, 230, 238), (120, 134, 150)


def gradiente(w, h, a, b):
    im = Image.new("RGB", (w, h))
    px = im.load()
    for y in range(h):
        for x in range(w):
            t = (0.65 * y / h + 0.35 * x / w)
            px[x, y] = tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))
    return im


def fonte(tam):
    for f in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
              "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
              "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"):
        if os.path.exists(f): return ImageFont.truetype(f, tam)
    return ImageFont.load_default()


def emblema(d, cx, cy, s):
    """Espada na diagonal + tres arcos de sinal saindo do cabo. s = escala."""
    # arcos (o online), atras da espada
    for i, r in enumerate((0.55, 0.78, 1.0)):
        rr = r * s
        box = (cx - rr, cy - rr, cx + rr, cy + rr)
        d.arc(box, 200, 290, fill=FOGO if i < 2 else FOGO_CLARO, width=max(1, int(s * 0.1)))
    # lamina (do canto inferior-esquerdo ao superior-direito)
    ang = math.radians(-45)
    ux, uy = math.cos(ang), math.sin(ang)          # direcao da lamina
    px_, py_ = -uy, ux                            # perpendicular
    base = (cx - ux * 0.55 * s, cy - uy * 0.55 * s)
    ponta = (cx + ux * 0.95 * s, cy + uy * 0.95 * s)
    lg = 0.13 * s
    lamina = [(base[0] + px_ * lg, base[1] + py_ * lg), (ponta[0] + px_ * lg * 0.3, ponta[1] + py_ * lg * 0.3),
              (ponta[0] + ux * lg, ponta[1] + uy * lg),
              (ponta[0] - px_ * lg * 0.3, ponta[1] - py_ * lg * 0.3), (base[0] - px_ * lg, base[1] - py_ * lg)]
    d.polygon(lamina, fill=ACO, outline=ACO_ESC)
    d.line([base, (ponta[0] + ux * lg * 0.6, ponta[1] + uy * lg * 0.6)], fill=ACO_ESC, width=max(1, int(s * 0.03)))
    # guarda
    g = 0.34 * s
    d.line([(base[0] + px_ * g, base[1] + py_ * g), (base[0] - px_ * g, base[1] - py_ * g)],
           fill=FOGO, width=max(2, int(s * 0.1)))
    # cabo e pomo
    cabo = (base[0] - ux * 0.36 * s, base[1] - uy * 0.36 * s)
    d.line([base, cabo], fill=(90, 58, 40), width=max(2, int(s * 0.1)))
    rp = 0.08 * s
    d.ellipse((cabo[0] - rp, cabo[1] - rp, cabo[0] + rp, cabo[1] + rp), fill=FOGO_CLARO)


def icone():
    S = 4                                          # desenha grande e reduz (antialias)
    im = gradiente(48 * S, 48 * S, NOITE_A, NOITE_B)
    d = ImageDraw.Draw(im)
    emblema(d, 24 * S, 25 * S, 17 * S)
    d.rounded_rectangle((1, 1, 48 * S - 2, 48 * S - 2), radius=8 * S, outline=FOGO, width=S * 2)
    im = im.resize((48, 48), Image.LANCZOS)
    im.save(os.path.join(AQUI, "icone.png"))


def banner():
    W, H, S = 256, 128, 3
    im = gradiente(W * S, H * S, NOITE_A, NOITE_B)
    d = ImageDraw.Draw(im)
    # lua e estrelas
    d.ellipse((200 * S, 12 * S, 226 * S, 38 * S), fill=(235, 232, 210))
    d.ellipse((207 * S, 10 * S, 231 * S, 34 * S), fill=NOITE_A)
    for (x, y) in ((30, 14), (70, 26), (120, 10), (160, 30), (240, 58), (100, 40)):
        d.ellipse(((x - 1) * S, (y - 1) * S, (x + 1) * S, (y + 1) * S), fill=(220, 230, 255))
    # montanhas em silhueta
    d.polygon([(0, 128 * S), (0, 92 * S), (40 * S, 60 * S), (78 * S, 90 * S), (118 * S, 52 * S),
               (160 * S, 88 * S), (205 * S, 58 * S), (256 * S, 96 * S), (256 * S, 128 * S)], fill=(10, 16, 30))
    d.polygon([(0, 128 * S), (0, 108 * S), (60 * S, 94 * S), (130 * S, 110 * S), (190 * S, 96 * S),
               (256 * S, 112 * S), (256 * S, 128 * S)], fill=(6, 10, 20))
    # fogueira no chao
    for r, c in ((10, FOGO), (6, FOGO_CLARO)):
        d.ellipse(((128 - r) * S, (116 - r) * S, (128 + r) * S, (116 + r // 2) * S), fill=c)
    emblema(d, 44 * S, 50 * S, 26 * S)
    f1, f2 = fonte(26 * S), fonte(11 * S)
    d.text((82 * S, 30 * S), "MH3U", font=f1, fill=(255, 255, 255))
    d.text((82 * S, 58 * S), "ONLINE", font=fonte(20 * S), fill=FOGO)
    d.text((82 * S, 84 * S), "multiplayer online  |  3DS", font=f2, fill=(200, 215, 230))
    im = im.resize((W, H), Image.LANCZOS)
    im.save(os.path.join(AQUI, "banner.png"))


def jingle():
    """Tres notas de 'chamado' (como uma trompa curta) + um acorde, ~1,6 s."""
    sr = 32000
    notas = [(392.0, 0.22), (523.25, 0.22), (659.25, 0.3), (0, 0.04), (523.25, 0.8)]
    amostras = []
    for f, dur in notas:
        n = int(sr * dur)
        for i in range(n):
            t = i / sr
            env = min(1.0, t / 0.02) * math.exp(-2.2 * t / max(dur, 0.1))
            if f == 0:
                v = 0.0
            else:
                v = (math.sin(2 * math.pi * f * t) + 0.35 * math.sin(2 * math.pi * 2 * f * t)
                     + 0.15 * math.sin(2 * math.pi * 3 * f * t))
                if dur > 0.5:                                  # acorde final
                    v += 0.6 * math.sin(2 * math.pi * f * 1.25 * t) + 0.5 * math.sin(2 * math.pi * f * 1.5 * t)
            amostras.append(int(max(-1, min(1, 0.28 * env * v)) * 32767))
    with wave.open(os.path.join(AQUI, "jingle.wav"), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
        w.writeframes(b"".join(struct.pack("<h", a) for a in amostras))


if __name__ == "__main__":
    icone(); banner(); jingle()
    print("arte/icone.png, arte/banner.png, arte/jingle.wav")
