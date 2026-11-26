"""Fahrbahn und Verkehr auf der Karolinenbrücke."""

import numpy as np


WEST = np.array([-203.10, 260.30])
OST = np.array([-96.50, 215.00])
LAENGE = float(np.linalg.norm(OST - WEST))
LAENGS = (OST - WEST) / LAENGE
QUER = np.array([-LAENGS[1], LAENGS[0]])
HALBE_BREITE = 6.75
BRUECKENSEKTOR = "S_N1_P0"


def deckhoehe(s):
    t = np.clip(s / LAENGE, 0, 1)
    deck = 6.37 + 0.04 * t + 0.22 * np.sin(np.pi * t)
    return np.where(s < 0, 6.37 + s * 0.022,
                    np.where(s > LAENGE, 6.41 + (s - LAENGE) * 0.023, deck))


def brueckenpunkt(s, q):
    return WEST + s * LAENGS + q * QUER


def brueckenprofil():
    return {"west": WEST.tolist(), "winkel": float(-np.arctan2(*LAENGS[::-1])),
            "laenge": LAENGE, "halb": HALBE_BREITE,
            "hoehen": np.round(deckhoehe(np.linspace(0, LAENGE, 25)), 4).tolist()}


def entferne_versunkene_fahrbahn(pos, idx):
    """Entfernt die alte, an den Flussgrund angepasste Fahrbahn."""
    teile = idx.reshape(-1, 3)
    p = pos[teile].mean(axis=1)
    delta = p[:, [0, 2]] - WEST
    s, q = delta @ LAENGS, delta @ QUER
    innen = (s > -0.08) & (s < LAENGE + 0.08) & (abs(q) < HALBE_BREITE + 0.25)
    return np.ascontiguousarray(teile[~innen]).reshape(-1)


def wasser_am_brueckenkopf(pos, wehr):
    # Der Wasserimport folgt stellenweise der Böschung bis auf Straßenniveau.
    # Oberhalb des Wehrs bleibt die Oberfläche auf dem Niveau des Flussbetts.
    x, y, z = pos.T
    gewicht = np.clip((z - 219.0) / 10, 0, 1) * np.clip((357.0 - z) / 25, 0, 1)
    gewicht *= (x >= -237) & (x <= -58)
    y -= np.maximum(y - 3.17, 0) * gewicht
    # Die Stufen liegen quer zum Lech. Das Unterwasser bleibt unter dem Wehr.
    wehr_x = np.interp(z, wehr[::-1, 1], wehr[::-1, 0])
    abstand = (wehr_x - x) * 0.92
    z_faktor = np.clip((z - wehr[-1, 1]) / 8, 0, 1) * np.clip((wehr[0, 1] - z) / 8, 0, 1)
    tief = np.clip((abstand + 0.15) / 8.5, 0, 1)
    ufer = np.clip((29.0 - abstand) / 15, 0, 1)
    y -= np.maximum(y - (3.15 - tief * 0.93), 0) * z_faktor * ufer


def fahrbahnstreifen():
    # Der rote Streifen liegt auf der flussaufwärtigen Seite der Fahrbahn.
    stationen = np.r_[-6.5, np.linspace(0, LAENGE, 25), LAENGE + 2.5]
    for name, q0, q1, dy, farbe in (
        ("Gehweg", -6.75, -3.76, 0.10, [0.24, 0.24, 0.22]),
        ("Road", -3.76, 3.38, 0.0, [0.13, 0.14, 0.14]),
        ("Brueckenradweg", 3.38, 4.47, 0.0, [0.30, 0.11, 0.09]),
        ("Gehweg", 4.47, 6.75, 0.10, [0.24, 0.24, 0.22]),
    ):
        punkte, idx = [], []
        for i, s in enumerate(stationen):
            for q in (q0, q1):
                x, z = brueckenpunkt(s, q)
                punkte.append((x, deckhoehe(s) + dy, z))
            if i:
                a = (i - 1) * 2
                idx.extend([a, a + 2, a + 3, a, a + 3, a + 1])
        yield name, np.asarray(punkte, np.float32), np.asarray(idx, np.int32), farbe
    # Am flussaufwärtigen Westkopf schließt der Bürgersteig an die Uferterrasse an.
    # Der unvollständige Geländeimport ließ dort zwei Wasserkeile im Fußweg offen.
    punkte = []
    stationen = ((-12.0, 15.1, 6.12), (-6.5, 18.2, 6.19), (-1.5, 16.9, 6.31), (2.15, 12.6, 6.34))
    for s, q, h in stationen:
        for seitlich, y in ((6.67, deckhoehe(s) + 0.097), (q, h)):
            x, z = brueckenpunkt(s, seitlich)
            punkte.append((x, y, z))
    idx = np.array([a + i for a in (0, 2, 4) for i in (0, 2, 3, 0, 3, 1)], np.int32)
    yield "Gehweg", np.asarray(punkte, np.float32), idx, [0.24, 0.24, 0.22]


def verkehr_auf_bruecke(punkte, fussweg=False):
    p = np.asarray(punkte).reshape(-1, 3).astype(float) / 100
    d = p[:, :2] - WEST
    s, q = d @ LAENGS, d @ QUER
    streifen = (s >= -6.5) & (s <= LAENGE + 2.5) & (np.abs(q) <= HALBE_BREITE + 0.30)
    if not np.any(streifen):
        return punkte
    if fussweg:
        mitte = streifen & (s > 8.0) & (s < LAENGE - 8.0)
        seite = -1 if np.median(q[mitte] if np.any(mitte) else q[streifen]) < 0 else 1
        uebergang = np.clip(np.minimum(s, LAENGE - s) / 5.5, 0, 1)
        dq = (seite * 5.35 - q[streifen]) * uebergang[streifen]
        p[streifen, :2] += dq[:, None] * QUER
    p[streifen, 2] = deckhoehe(s[streifen]) + (0.10 if fussweg else 0)
    return np.rint(p * 100).astype(int).reshape(-1).tolist()
