"""Öffentlicher Straßenraum zwischen REWE und Danziger Platz nach DOP20."""

import numpy as np

from stadtfreiflaechen import Flaechenkarte, INSEL, PFLASTER, ROT, WEISS


# Die Achse folgt den vorhandenen Straßenknoten. Die Stationen der Einbauten
# wurden am Luftbild zwischen den jeweiligen Kreuzungen abgelesen.
ACHSE = np.array([
    [-1456.30, 0.80], [-1423.50, 60.30], [-1415.20, 75.40],
    [-1340.80, 210.70], [-1320.60, 246.60], [-1297.80, 286.50],
    [-1231.70, 402.90], [-1113.60, 616.20], [-1090.30, 657.20],
    [-1079.30, 676.70], [-1061.40, 709.20], [-1026.40, 771.50], [-1014.70, 788.60],
])
RASTER_GRENZEN = (-47.0, -7.0, 56.0, 919.0)
ZUR_KARTE = np.array([[0.868745, -0.495259], [0.495259, 0.868745]])
MITTELINSELN = (
    (98, 119, 1.65), (130, 148, 1.55), (165, 183, 1.62), (188, 201, 1.70),
    (216, 237, 1.61), (267, 283, 1.68), (295, 310, 1.64), (323, 339, 1.72),
    (360, 380, 1.70), (425, 447, 1.63), (457, 477, 1.71), (487, 505, 1.67),
    (516, 537, 1.75), (546, 564, 1.73), (577, 603, 1.82), (638, 665, 1.80),
    (688, 715, 1.83),
)
GEH_RADWEGE = ((79, 202), (215, 237), (256, 392), (414, 562),
               (574, 605), (627, 668), (684, 750), (760, 774))
QUERUNGEN = (207.0, 400.9, 615.8, 774.2)
KREUZUNGEN = ((207.0, 17.0), (246.6, 21.0), (402.9, 25.0),
              (616.2, 25.0), (788.6, 38.0))
RADWEG = (52, 56, 54)
ASPHALT_FREI = (36, 38, 40)
ASPHALT_REPARATUR = (28, 30, 33)
KANTSTEIN = (88, 84, 74)


def strassenpunkt(z, seite=0.0):
    """Längsposition und rechtwinkliger Abstand von der lokalen Fahrbahnachse."""
    i = min(np.searchsorted(ACHSE[1:, 1], z, side="right"), len(ACHSE) - 2)
    a, b = ACHSE[i:i + 2]
    x = a[0] + (z - a[1]) * (b[0] - a[0]) / (b[1] - a[1])
    richtung = (b - a) / np.linalg.norm(b - a)
    return np.array([x, z]) + seite * np.array([richtung[1], -richtung[0]])


def breslauer_raster(punkt):
    return ZUR_KARTE @ (punkt - ACHSE[0])


def breslauer_karte(kreuzungsdecke=()):
    flaeche = Flaechenkarte(RASTER_GRENZEN, 0.23)

    def linie(farbe, breite, punkte):
        flaeche.linie([breslauer_raster(p) for p in punkte], breite, farbe)

    def band(a, b, seite, breite, farbe):
        linie(farbe, breite, [strassenpunkt(z, seite) for z in np.linspace(a, b, 2 + int((b - a) / 6))])

    # Die Aussparungen folgen den echten Fahrbahndreiecken der einmündenden
    # Straßen. So laufen die hellen Gehwegbänder optisch an der Furt aus.
    for dreieck, mitte, radius in kreuzungsdecke:
        flaeche.polygon([breslauer_raster(p) for p in dreieck], ASPHALT_FREI,
                        kreis=(breslauer_raster(mitte), radius))

    # Asphaltdecke, Grüntrennung und Radweg folgen der Kurve bis zum Platz.
    band(78, 780, 0.0, 6.56, ASPHALT_FREI)
    for a, b in GEH_RADWEGE:
        band(a, b, -5.64, 2.02, INSEL)
        band(a, b, -7.70, 2.32, RADWEG)
        band(a, b, -9.18, 0.86, PFLASTER)
        band(a, b, 5.71, 1.08, INSEL)
        band(a, b, 7.66 if a < 570 else 7.95, 1.83, RADWEG)

    # Kurze, matte Flicken auf den Fahrspuren vor den Einmündungen.
    for a, b, seite in [(147, 154, 1.9), (271, 274, -1.7), (449, 455, 1.3),
                        (588, 593, -2.1), (735, 741, 1.8)]:
        band(a, b, seite, 1.6, ASPHALT_REPARATUR)

    wiese = []
    for i, (a, b, breite) in enumerate(MITTELINSELN):
        seite = 0.17 if a < 415 else 0.62
        farbe = PFLASTER if i in {8, 11, 15} else INSEL
        band(a, b, seite, breite + 0.44, KANTSTEIN)
        band(a, b, seite, breite, farbe)
        for z in (a, b):
            p = breslauer_raster(strassenpunkt(z, seite))
            flaeche.kreis(p, breite / 2 + 0.22, KANTSTEIN)
            flaeche.kreis(p, breite / 2, farbe)
        if i not in {8, 11, 15}:
            for z in np.arange(a + 1.4, b - 1.0, 1.75):
                wiese.append(strassenpunkt(z, seite + (0.16 if int(z) % 2 else -0.16)))

    # Begrenzung der bestehenden Querungen und kurze Radfurten an den Anschlüssen.
    for z in QUERUNGEN:
        for rand in (z - 1.15, z + 1.15):
            for quer in np.arange(-4.2, 4.3, 0.94):
                linie(WEISS, 0.19, [strassenpunkt(rand, quer), strassenpunkt(rand, quer + 0.53)])
        for seite in (-1.0, 1.0):
            band(z - 1.35, z + 1.35, seite * 6.27, 2.26, PFLASTER)
    for z, seite in [(208, -7.7), (399, 7.3), (615, -7.65), (777, 7.9)]:
        band(z - 1.6, z + 1.6, seite, 2.06, ROT)
    for z in np.arange(83, 777, 8):
        if any(a - 1.9 < z < b + 1.9 for a, b, _ in MITTELINSELN):
            continue
        if all(abs(z - querung) > 8.0 for querung in QUERUNGEN) and abs(z - 246.6) > 10:
            band(z, z + 2.4, 0.08, 0.17, WEISS)

    baeume = [(strassenpunkt(z, seite), groesse) for z, seite, groesse in [
        (88, -12.2, 5.8), (117, 11.9, 6.1), (135, -11.7, 6.8), (165, 11.5, 5.6),
        (189, -11.9, 6.4), (228, -11.9, 5.6), (265, 12.2, 6.5), (286, -12.7, 5.9),
        (315, 10.7, 5.1), (346, -11.1, 5.5), (370, 11.0, 6.8), (389, -10.9, 6.1),
        (430, 10.3, 7.2), (453, -11.4, 6.6), (483, 11.0, 7.2), (504, -10.8, 5.5),
        (540, 10.4, 5.8), (561, -11.2, 5.6), (578, 11.8, 7.4), (608, -11.2, 6.2),
        (643, 13.0, 7.1), (662, 13.0, 6.4), (710, -11.1, 7.2), (751, 11.4, 6.9),
    ]]
    objekte = []
    for i, z in enumerate(range(92, 775, 33)):
        if any(abs(z - p) < 8.0 for p in (*QUERUNGEN, 246.6, 674.0)):
            continue
        objekte.append((z, -5.58 if i % 2 == 0 else 5.81, "Leuchte"))
    objekte.extend([
        (220.5, 5.70, "Römerhang"), (226.0, -5.78, "Römerhang"),
        (583.5, 6.05, "Agentur für Arbeit"), (592.8, -5.93, "Agentur für Arbeit"),
    ])
    return flaeche, wiese, baeume, objekte
