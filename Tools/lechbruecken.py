"""Brückenprofile der Landsberger Lechquerungen, in Godot-Koordinaten (m)."""

import numpy as np

STRASSENBRUECKEN = {"verbund", "kanalstrasse", "bahnstrasse"}


class Lechbruecke:
    def __init__(self, name, bauart, achse, halbe_breite, *, y0, y1, wölbung, pfeiler=()):
        self.name = name
        self.bauart = bauart
        achse = np.asarray(achse, dtype=float)
        self.seite = halbe_breite
        self.pfeiler = pfeiler
        abschnitte = np.linalg.norm(np.diff(achse, axis=0), axis=1)
        stationen = np.r_[0.0, np.cumsum(abschnitte)]
        self.laenge = float(stationen[-1])
        # Enthält auch die Knicke der im Luftbild erfassten Achse.
        self.s = np.unique(np.r_[np.linspace(0, self.laenge, max(3, 2 + int(self.laenge / 4))), stationen])
        self.xz = np.column_stack([np.interp(self.s, stationen, achse[:, i]) for i in (0, 1)])
        t = self.s / self.laenge
        self.y = y0 + (y1 - y0) * t + wölbung * np.sin(np.pi * t)
        richtung = np.gradient(self.xz, self.s, axis=0)
        self.quer = richtung[:, [1, 0]] * [-1, 1] / np.linalg.norm(richtung, axis=1)[:, None]
        self.links = np.full(t.shape, halbe_breite)
        self.rechts = self.links.copy()
        if bauart == "corten":
            self.rechts += np.interp(t, [0.36, 0.44, 0.53, 0.66], [0.0, 1.90, 1.90, 0.0])
        self.schnittrand = 4.4 if bauart in {"steg", "corten"} else 0.20
        rand = max(self.links.max(), self.rechts.max()) + self.schnittrand
        self.grenzen = np.r_[self.xz.min(axis=0) - rand, self.xz.max(axis=0) + rand]

    def sektor(self, groesse):
        return tuple(np.floor(self.xz[len(self.xz) // 2] / groesse).astype(int))

    def profil(self):
        return {"name": self.name, "bauart": self.bauart, "laenge": self.laenge,
                "pfeiler": list(self.pfeiler),
                "stationen": np.round(np.column_stack((self.xz[:, 0], self.y, self.xz[:, 1],
                                                         self.quer, self.links, self.rechts, self.s)), 4).tolist()}

    def streifen(self):
        if self.bauart == "verbund":
            belaege = [("Gehweg", -7.50, -5.27, 0.11), ("Brueckenradweg", -5.27, -3.53, 0.0),
                       ("Road", -3.53, 3.53, 0.0), ("Brueckenradweg", 3.53, 5.27, 0.0),
                       ("Gehweg", 5.27, 7.50, 0.11)]
        elif self.bauart in {"bahnstrasse", "kanalstrasse"}:
            weg = 1.72 if self.bauart == "bahnstrasse" else min(0.92, self.seite * 0.3)
            innen = self.seite - weg
            belaege = [("Gehweg", -self.seite, -innen, 0.09), ("Road", -innen, innen, 0.0),
                       ("Gehweg", innen, self.seite, 0.09)]
        elif self.bauart == "holz":
            belaege = [("Holzsteg", -self.links, self.rechts, 0.0)]
        else:
            belaege = [("Stegdeck", -self.links, self.rechts, 0.0)]
        i = np.arange(len(self.s) - 1, dtype=np.int32) * 2
        idx = (i[:, None] + [0, 2, 3, 0, 3, 1]).astype(np.int32).reshape(-1)
        for name, q0, q1, dy in belaege:
            punkte = np.empty((len(self.s), 2, 3), np.float32)
            punkte[:, 0, ::2] = self.xz + self.quer * np.asarray(q0)[..., None]
            punkte[:, 1, ::2] = self.xz + self.quer * np.asarray(q1)[..., None]
            punkte[:, :, 1] = (self.y + dy)[:, None]
            yield name, punkte.reshape(-1, 3), idx, [0.24, 0.24, 0.22]

    def _projektion(self, punkte):
        # Projektion auf die geknickte Achse, einschließlich Vor- und Nachlauf.
        start = self.xz[:-1]
        v = np.diff(self.xz, axis=0)
        d = punkte[:, None, :] - start
        t = (d * v).sum(axis=2) / np.sum(v * v, axis=1)
        t_begrenzt = np.clip(t, 0, 1)
        rest = d - t_begrenzt[:, :, None] * v
        i = np.argmin(np.sum(rest * rest, axis=2), axis=1)
        r = np.arange(len(punkte))
        q = (rest[r, i] * self.quer[i]).sum(axis=1)
        anteil = t_begrenzt[r, i]
        station = self.s[i] + anteil * np.diff(self.s)[i]
        innerhalb = ((i > 0) | (t[r, i] >= 0)) & ((i < len(v) - 1) | (t[r, i] <= 1))
        return station, q, innerhalb

    def entferne_flaechen(self, pos, idx):
        teile = idx.reshape(-1, 3)
        mitte = pos[teile].mean(axis=1)[:, ::2]
        nah = np.flatnonzero(np.all((mitte >= self.grenzen[:2]) & (mitte <= self.grenzen[2:]), axis=1))
        if not nah.size:
            return idx
        s, q, innerhalb = self._projektion(mitte[nah])
        links, rechts = np.interp(s, self.s, self.links), np.interp(s, self.s, self.rechts)
        entfernen = nah[innerhalb & (q >= -links - self.schnittrand) & (q <= rechts + self.schnittrand)]
        return np.ascontiguousarray(np.delete(teile, entfernen, axis=0)).reshape(-1)

    def verkehr(self, punkte, fussweg):
        p = np.asarray(punkte).reshape(-1, 3).astype(float) / 100
        nah = np.flatnonzero(np.all((p[:, :2] >= self.grenzen[:2]) & (p[:, :2] <= self.grenzen[2:]), axis=1))
        if not nah.size:
            return punkte
        s, q, innerhalb = self._projektion(p[nah, :2])
        links, rechts = np.interp(s, self.s, self.links), np.interp(s, self.s, self.rechts)
        maske = innerhalb & (q >= -links - 0.35) & (q <= rechts + 0.35)
        # Das Straßennetz führt die Autos auf dem Verbundträger. Der alte
        # Export hat vereinzelt Fußwege als schmale Fahrbahn gespeichert.
        if self.bauart in STRASSENBRUECKEN and not fussweg:
            maske &= np.abs(q) < (3.68 if self.bauart == "verbund" else self.seite - 0.55)
        h = np.interp(s[maske], self.s, self.y)
        if self.bauart == "verbund" and fussweg:
            h += 0.11
            t = s[maske]
            innen = (t > 6) & (t < self.laenge - 6)
            seite = np.sign(np.median(q[maske][innen])) if np.any(innen) else 0
            if seite:
                quer = np.column_stack([np.interp(t, self.s, self.quer[:, i]) for i in (0, 1)])
                q_ziel = (seite * 6.4 - q[maske]) * np.clip(np.minimum(t, self.laenge - t) / 6, 0, 1)
                p[nah[maske], :2] += q_ziel[:, None] * quer
        p[nah[maske], 2] = h
        return np.rint(p * 100).astype(int).reshape(-1).tolist() if np.any(maske) else punkte

    def fussweg(self):
        """Ein Weg über beide Uferanschlüsse, im Format des Passantennetzes."""
        return np.rint(np.column_stack((self.xz, self.y)) * 100).astype(int).tolist()


# Westliches und östliches Auflager aus dem DOP20, Deckhöhen an das Gelände angepasst.
BRUECKEN = (
    Lechbruecke("LadyHerkomerSteg", "corten",
               [(-474.8, -224.0), (-439.7, -224.2), (-407.3, -222.5),
                (-381.2, -217.5), (-344.4, -208.8)], 2.75,
               y0=4.57, y1=4.35, wölbung=0.30, pfeiler=(0.50,)),
    Lechbruecke("Lechsteg", "steg", [(-284.28, -476.29), (-235.55, -421.26)], 1.37,
               y0=3.63, y1=3.62, wölbung=1.01, pfeiler=(0.53,)),
    Lechbruecke("SandauerBruecke", "verbund", [(-78.5, -611.9), (-28.9, -554.5)], 7.50,
               y0=2.26, y1=3.87, wölbung=1.66, pfeiler=(0.28, 0.72)),
)

# Lage aus OSM, mit den Ufer- und Wegeflächen im Orthofoto abgeglichen.
# Gemeinsame Geh- und Radwege auf einer Fahrbahnplatte bilden ein Bauwerk.
KLEINBRUECKEN = (
    ("MuehlbachbrueckeSandauerTor", "kanalstrasse", 10.8, [(13.91, -498.70), (24.36, -486.14)]),
    ("VordereMuehlgasse", "kanalstrasse", 5.6, [(-146.17, -151.02), (-133.32, -150.57)]),
    ("HintereMuehlgasse", "kanalstrasse", 4.7, [(-124.82, -222.55), (-116.40, -219.79)]),
    ("AdolphKolpingStrasse", "kanalstrasse", 5.1, [(-140.27, 13.54), (-133.56, 16.20)]),
    ("VonKuehlmannStrasse", "kanalstrasse", 6.7, [(-490.18, -57.63), (-488.62, -53.76)]),
    ("Wildparkweg", "kanalstrasse", 3.7, [(-33.44, 350.92), (-33.04, 353.49)]),
    ("BahnbrueckeAugsburgerStrasse", "bahnstrasse", 13.4, [(-590.54, -830.97), (-565.92, -835.81)]),
    ("BahnbrueckeFruehlingsweg", "bahnsteg", 3.0, [(-571.92, -476.18), (-547.95, -470.07)]),
    ("PaterRupertMayerSteg", "bahnsteg", 2.9, [(-431.18, 938.84), (-409.44, 948.36)]),
    ("PaterRupertMayerRampe", "bahnsteg", 2.5, [(-409.44, 948.36), (-383.93, 887.88)]),
    ("PaterRupertMayerLandung", "bahnsteg", 2.5, [(-383.93, 887.88), (-381.00, 888.63)]),
    ("PaterRupertMayerRuecklauf", "bahnsteg", 2.5, [(-381.00, 888.63), (-405.02, 947.55), (-401.67, 949.08)]),
    ("Muellersteg", "kleinsteg", 2.2, [(-96.07, -312.37), (-90.70, -310.79)]),
    ("Muehlbachsteg", "kleinsteg", 2.1, [(-141.86, -58.59), (-136.41, -58.70)]),
    ("PapierbachstegWest", "holz", 1.8, [(-458.93, -44.34), (-458.40, -47.57)]),
    ("PapierbachstegMitte", "holz", 1.9, [(-439.79, -41.44), (-438.94, -44.41)]),
    ("PapierbachstegOst", "holz", 2.4, [(-416.69, -39.06), (-419.23, -33.17)]),
    ("PapierbachstegLech", "holz", 2.1, [(-408.81, -36.78), (-409.70, -31.83)]),
    ("PapierbachstegSued", "holz", 1.7, [(-428.22, -18.17), (-430.39, -20.79)]),
    ("PapierbachstegNord", "holz", 2.1, [(-461.20, -43.03), (-465.83, -42.43)]),
    ("BachstegAmBahnhof", "kleinsteg", 2.1, [(-606.41, -78.38), (-604.31, -81.99)]),
    ("BachstegKatharinenvorstadt", "holz", 1.6, [(-775.65, 58.46), (-770.17, 60.13)]),
    ("BachstegWest", "holz", 1.6, [(-695.43, -43.25), (-696.25, -45.20)]),
    ("InselbadstegNord", "kleinsteg", 2.2, [(-302.98, -153.10), (-299.47, -156.20)]),
    ("InselbadstegOst", "kleinsteg", 2.2, [(-204.15, -38.93), (-198.42, -37.52)]),
    ("InselbadstegMitte", "kleinsteg", 2.6, [(-253.91, -107.56), (-248.74, -114.58)]),
    ("AuwaldstegOst", "holz", 1.8, [(161.20, 1287.35), (169.81, 1287.25)]),
    ("AuWanderwegOst", "holz", 2.0, [(17.85, 1243.63), (9.64, 1242.93)]),
    ("AuWanderwegWest", "holz", 2.0, [(-2.60, 1243.68), (-11.30, 1240.01)]),
    ("AuwaldForstweg", "holz", 2.5, [(3.28, 1506.73), (4.37, 1503.31)]),
    ("WildparkstegNord", "holz", 1.9, [(81.00, 1065.58), (79.39, 1060.68)]),
    ("WildparkstegMitte", "holz", 1.8, [(79.06, 1106.12), (86.11, 1108.10)]),
    ("WildparkstegSued", "holz", 1.4, [(86.55, 1113.27), (87.11, 1111.48)]),
    ("WildparkstegAu", "holz", 1.8, [(106.08, 1498.41), (109.85, 1506.94)]),
    ("Hangwegsteg", "kleinsteg", 2.0, [(206.20, 251.89), (213.44, 267.37)]),
)


def kleine_bruecken(hoehe):
    for name, bauart, breite, achse in KLEINBRUECKEN:
        y = [hoehe(np.asarray(achse[i])) for i in (0, -1)]
        if any(h is None for h in y):
            continue
        bogen = 0.25 if bauart == "bahnstrasse" else 0.11
        yield Lechbruecke(name, bauart, achse, breite * 0.5, y0=y[0] + 0.05,
                         y1=y[1] + 0.05, wölbung=bogen)


def bahneinschnitte(pos):
    # Der DGM-Import zieht die Straße und die Gleise auf dieselbe Oberfläche.
    # An den drei Bahnübergängen läuft das Gleis im Einschnitt unter der Platte.
    x, y, z = pos.T
    for mx, mz, dx, tiefe in ((-573.05, -833.90, 0.244, 4.26),
                              (-556.50, -473.20, -0.260, 4.05),
                              (-412.09, 948.36, -0.463, 3.65)):
        laengs = abs(z - mz)
        quer = abs(x - mx - dx * (z - mz))
        y -= tiefe * np.clip((31.0 - laengs) / 16.0, 0, 1) * np.clip((11.0 - quer) / 6.5, 0, 1)


def verkehr_auf_lechbruecken(punkte, bruecken, fussweg=False):
    for bruecke in bruecken:
        if fussweg or bruecke.bauart in STRASSENBRUECKEN:
            punkte = bruecke.verkehr(punkte, fussweg)
    return punkte
