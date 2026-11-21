"""Gelaendehoehe der ganzen Stadt als Raster und die Verschiebung, die das grossraeumige Relief
staucht (siehe konvertiere_sektoren.py). Alles, was an einem Ort steht (Boden, Strassen, Wasser,
Haeuser, Baeume, Routen, Ampeln, Spawn), wird um dieselbe glatte Verschiebung angehoben oder
gesenkt; dadurch bleibt alles aufeinander abgestimmt und nur die Steigungen werden flacher.

  y' = y + (alpha - 1) * (L(x, z) - R)

L ist das Gelaende, mit einem Mittelwertfilter geglaettet (nur das Grossraeumige), R die mittlere
Hoehe. Kleine Formen (Bordsteine, Treppen, Dachformen) bleiben unveraendert.
"""
import numpy as np


class Relief:
    def __init__(self, punkte: np.ndarray, alpha: float = 0.5, zelle: float = 4.0, radius: int = 4):
        """punkte: (n, 3) Bodenpunkte in Metern (x, hoehe, z), Godot-Achsen."""
        self.alpha = alpha
        self.zelle = zelle
        self.x0 = float(punkte[:, 0].min()) - 2 * zelle
        self.z0 = float(punkte[:, 2].min()) - 2 * zelle
        nx = int((punkte[:, 0].max() - self.x0) / zelle) + 3
        nz = int((punkte[:, 2].max() - self.z0) / zelle) + 3
        ix = ((punkte[:, 0] - self.x0) / zelle).astype(int)
        iz = ((punkte[:, 2] - self.z0) / zelle).astype(int)
        summe = np.zeros((nx, nz))
        anzahl = np.zeros((nx, nz))
        np.add.at(summe, (ix, iz), punkte[:, 1])
        np.add.at(anzahl, (ix, iz), 1.0)
        maske = (anzahl > 0).astype(float)
        mittel = np.where(anzahl > 0, summe / np.maximum(anzahl, 1.0), 0.0)
        # Normalisierte Glaettung: Luecken (keine Daten) tragen nicht zum Mittel bei.
        zaehler = _glaette(mittel * maske, radius)
        nenner = _glaette(maske, radius)
        self.gelaende = np.where(nenner > 1e-6, zaehler / np.maximum(nenner, 1e-6), np.nan)
        gueltig = nenner > 0.05
        self.referenz = float(np.median(self.gelaende[gueltig]))
        self.gelaende = np.where(np.isnan(self.gelaende), self.referenz, self.gelaende)
        self.verschiebung = (alpha - 1.0) * (self.gelaende - self.referenz)
        self.hoehe_vorher = (float(np.percentile(self.gelaende[gueltig], 1)),
                             float(np.percentile(self.gelaende[gueltig], 99)))

    def delta(self, x, z) -> np.ndarray:
        """Verschiebung an (x, z) in Metern, bilinear zwischen den Rasterzellen."""
        fx = (np.asarray(x, dtype=float) - self.x0) / self.zelle
        fz = (np.asarray(z, dtype=float) - self.z0) / self.zelle
        fx = np.clip(fx, 0, self.verschiebung.shape[0] - 1.001)
        fz = np.clip(fz, 0, self.verschiebung.shape[1] - 1.001)
        i, j = fx.astype(int), fz.astype(int)
        u, v = fx - i, fz - j
        d = self.verschiebung
        return (d[i, j] * (1 - u) * (1 - v) + d[i + 1, j] * u * (1 - v)
                + d[i, j + 1] * (1 - u) * v + d[i + 1, j + 1] * u * v)


def _kasten(a: np.ndarray, r: int, achse: int) -> np.ndarray:
    """Mittelwert ueber 2r+1 Zellen entlang einer Achse (Summentabelle, Raender wiederholt)."""
    pad = [(0, 0), (0, 0)]
    pad[achse] = (r + 1, r)
    s = np.cumsum(np.pad(a, pad, mode="edge"), axis=achse)
    n = a.shape[achse]
    oben = np.take(s, range(2 * r + 1, 2 * r + 1 + n), axis=achse)
    unten = np.take(s, range(0, n), axis=achse)
    return (oben - unten) / (2 * r + 1)


def _glaette(a: np.ndarray, r: int, durchgaenge: int = 3) -> np.ndarray:
    """Dreifacher Kastenfilter in beiden Richtungen, nahe an einem Gauss-Filter."""
    for _ in range(durchgaenge):
        a = _kasten(_kasten(a, r, 0), r, 1)
    return a
