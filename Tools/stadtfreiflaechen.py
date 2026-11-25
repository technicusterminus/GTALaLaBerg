"""Ortsgebundene Beläge auf dem Stadtnetz, ohne Fototexturen im Spiel."""

import struct

import numpy as np


# Lagebezug der manuell aus dem DOP20 abgezeichneten Flächen.
# Kontrollpunkte: REWE-Baukanten, Klinikzufahrt und Wohnstraßen.
def hartmann_nord(x, z):
    return (-1864.5 + 0.3286 * x - 0.0060 * z,
            -303.0 + 0.0075 * x + 0.3280 * z)


def hartmann_sued(x, z):
    return hartmann_nord(-22.66 + x * 1.041, 1508.8 + z * 1.039)


ASPHALT = (31, 33, 35)
PFLASTER = (67, 62, 54)
KIES = (81, 70, 53)
WEG = (72, 68, 57)
RASEN = (34, 52, 22)
INSEL = (29, 45, 21)
ACKER = (46, 56, 24)
ROT = (86, 39, 28)
WEISS = (159, 148, 122)

HARTMANN_GRENZEN = (-1940, -148, -1280, 750)
KATHARINEN_GRENZEN = (-912, 836, -466, 1335)


class Flaechenkarte:
    """Rasterisiert Entwurfsflächen direkt in Weltkoordinaten."""

    def __init__(self, grenzen, meter_pro_pixel=0.32):
        self.grenzen = np.array(grenzen, dtype=float)
        self.breite, self.hoehe = np.ceil(
            (self.grenzen[2:] - self.grenzen[:2]) / meter_pro_pixel).astype(int)
        self.belge = np.zeros((self.hoehe, self.breite, 4), np.uint8)
        self.schritt = (self.grenzen[2:] - self.grenzen[:2]) / [self.breite, self.hoehe]

    def polygon(self, punkte, farbe):
        p = (np.asarray(punkte) - self.grenzen[:2]) / self.schritt
        x0, z0 = np.maximum(np.floor(p.min(axis=0)).astype(int), 0)
        x1, z1 = np.minimum(np.ceil(p.max(axis=0)).astype(int), [self.breite, self.hoehe])
        if x0 >= x1 or z0 >= z1:
            return
        x, z = np.arange(x0, x1)[None, :] + 0.5, np.arange(z0, z1)[:, None] + 0.5
        innen = np.zeros((z1 - z0, x1 - x0), dtype=bool)
        for (ax, az), (bx, bz) in zip(p, np.roll(p, -1, axis=0)):
            if az != bz:
                innen ^= ((az > z) != (bz > z)) & (x < ax + (z - az) * (bx - ax) / (bz - az))
        self.belge[z0:z1, x0:x1][innen] = (*farbe, 255)

    def kreis(self, mitte, radius, farbe):
        ring = np.linspace(0, 2 * np.pi, 24, endpoint=False)
        self.polygon(np.asarray(mitte) + radius * np.column_stack((np.cos(ring), np.sin(ring))), farbe)

    def linie(self, punkte, breite, farbe):
        for a, b in zip(punkte, punkte[1:]):
            a, b = np.asarray(a), np.asarray(b)
            dx, dz = b - a
            quer = np.array([-dz, dx]) * (breite / (2 * np.hypot(dx, dz)))
            self.polygon([a - quer, b - quer, b + quer, a + quer], farbe)
        for p in punkte[1:-1]:
            self.kreis(p, breite / 2, farbe)

    def speichern(self, pfad):
        # Unkomprimiertes, oben beginnendes 32-bit-TGA für Godots Texturimport.
        kopf = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0,
                           self.breite, self.hoehe, 32, 0x28)
        pfad.write_bytes(kopf + self.belge[:, :, [2, 1, 0, 3]].tobytes())

    def farbe_an(self, punkt):
        x, z = np.floor((punkt - self.grenzen[:2]) / self.schritt).astype(int)
        if 0 <= x < self.breite and 0 <= z < self.hoehe:
            return tuple(int(kanal) for kanal in self.belge[z, x])
        return (0, 0, 0, 0)


def parkreihen(flaeche, a, b, tiefe, doppelt=True):
    """Stellplatzstreifen, Mittelinsel und die Mittelpunkte nutzbarer Buchten."""
    a, b = np.asarray(a), np.asarray(b)
    laenge = np.linalg.norm(b - a)
    achse = (b - a) / laenge
    quer = np.array([-achse[1], achse[0]])
    if doppelt:
        flaeche.linie([a, b], 1.18, INSEL)
    anzahl = int(laenge / 2.62)
    buchten = []
    for seite in (-1, 1) if doppelt else (1,):
        innen = quer * seite * (0.66 if doppelt else 0.0)
        aussen = innen + quer * seite * tiefe
        for i in range(anzahl + 1):
            mitte = a + achse * (i * 2.62)
            flaeche.linie([mitte + innen, mitte + aussen], 0.22, WEISS)
            if i < anzahl:
                punkt = mitte + achse * 1.31 + (innen + aussen) / 2
                buchten.append((punkt, quer * seite))
    return buchten


def hartmann_karte():
    flaeche = Flaechenkarte(HARTMANN_GRENZEN)
    n, s = hartmann_nord, hartmann_sued

    def polygon(farbe, *punkte, bezug=n):
        flaeche.polygon([bezug(*p) for p in punkte], farbe)

    def linie(farbe, breite, *punkte, bezug=n):
        flaeche.linie([bezug(*p) for p in punkte], breite, farbe)

    # Klinikparkplatz, Marktvorplatz, Grünzüge und Höfe an der Römerauterrasse.
    polygon(ASPHALT, (137, 863), (346, 794), (507, 1248), (284, 1318), (233, 1174))
    polygon(ASPHALT, (1054, 950), (1186, 859), (1260, 957), (1319, 1011),
            (1281, 1043), (1165, 1092), (1099, 1036))
    linie(PFLASTER, 3.8, (1056, 955), (1188, 860))
    linie(ASPHALT, 6.1, (1280, 1018), (1321, 1004), (1334, 1004))
    polygon(PFLASTER, (514, 1232), (694, 1176), (938, 1138), (984, 1199),
            (748, 1261), (535, 1302))
    polygon(RASEN, (855, 696), (922, 683), (1046, 725), (939, 816), (910, 839), (862, 789))
    polygon(RASEN, (868, 1485), (970, 1486), (1082, 1520), (1103, 1589),
            (1061, 1644), (888, 1654), (824, 1610))
    linie(WEG, 2.2, (714, 1557), (790, 1543), (871, 1503), (965, 1470), (1029, 1523),
          (1085, 1556), (1072, 1614), (1031, 1640), (960, 1647))
    linie(WEG, 1.85, (853, 1640), (846, 1598), (869, 1530), (901, 1495))
    linie(WEG, 1.95, (1035, 1526), (1028, 1583))
    flaeche.kreis(n(1029, 1595), 8.0, PFLASTER)
    flaeche.kreis(n(1029, 1595), 5.5, ROT)
    flaeche.linie([n(1017, 1598), n(1041, 1592)], 0.93, WEISS)
    flaeche.linie([n(1027, 1584), n(1032, 1606)], 0.93, WEISS)

    polygon(RASEN, (271, 1392), (386, 1355), (503, 1354), (510, 1418), (312, 1470))
    for x, z, radius in [(295, 1440, 5.1), (278, 1406, 4.6), (478, 1395, 6.6),
                         (949, 723, 5.0), (994, 745, 4.3), (942, 770, 3.8)]:
        flaeche.kreis(n(x, z), radius, KIES)
    linie(WEG, 2.25, (297, 1422), (352, 1411), (402, 1412), (472, 1387), (510, 1372))
    polygon(PFLASTER, (536, 1412), (596, 1436), (604, 1506), (558, 1524), (533, 1489))
    polygon(ACKER, (7, 1469), (247, 1376), (310, 1539), (76, 1647))
    polygon(ACKER, (761, 264), (1007, 193), (1063, 328), (799, 404), bezug=s)
    polygon(ACKER, (301, 615), (725, 513), (968, 1518), (557, 1556), (415, 1120), bezug=s)
    for a, b in [((1227, 856), (1255, 897)), ((1264, 912), (1287, 953)),
                 ((1336, 1031), (1357, 1067)), ((1380, 1108), (1398, 1144))]:
        linie(INSEL, 1.58, a, b)
    linie(KIES, 2.3, (252, 767), (356, 1024), (496, 1453), (530, 1600), bezug=s)
    linie(WEG, 2.1, (527, 1598), (617, 1620), (744, 1576), (858, 1555),
          (970, 1550), (1138, 1548), bezug=s)
    polygon(RASEN, (846, 588), (1160, 493), (1228, 618), (896, 726), bezug=s)
    for a, b in [((858, 603), (1168, 508)), ((883, 679), (1194, 583)), ((973, 544), (1004, 650)),
                 ((1083, 512), (1123, 629))]:
        linie(WEG, 1.35, a, b, bezug=s)

    buchung = []
    baeume = []
    for a, b in [((162, 873), (193, 956)), ((228, 847), (264, 938)),
                 ((291, 831), (326, 918)), ((216, 1010), (300, 1261)),
                 ((280, 989), (363, 1242)), ((344, 966), (430, 1222)),
                 ((409, 949), (488, 1190)), ((1108, 955), (1178, 1047)),
                 ((1163, 914), (1230, 1009))]:
        start, ende = np.asarray(n(*a)), np.asarray(n(*b))
        buchung.extend(parkreihen(flaeche, start, ende, 4.4))
        for t in np.linspace(0.12, 0.93, int(np.linalg.norm(ende - start) / 11) + 1):
            baeume.append((start + t * (ende - start), 4.6))
    for i in range(9):
        x = 1167 + 12 * i
        z = 1074 - 4.0 * i
        flaeche.kreis(n(x, z), 1.4, INSEL)
    for x, z, groesse in [
        (173, 835, 7.3), (217, 818, 6.7), (303, 798, 7.2), (316, 1320, 7.0),
        (363, 1311, 7.4), (405, 1295, 7.8), (496, 1259, 6.3),
        (273, 1407, 6.7), (317, 1443, 6.3), (354, 1401, 5.2),
        (440, 1408, 5.1), (480, 1411, 5.8), (524, 1398, 4.6),
        (836, 1550, 7.1), (875, 1580, 8.2), (970, 1609, 6.0),
        (1075, 1547, 7.3), (1090, 1620, 8.1), (799, 1604, 8.7),
        (1105, 1030, 6.1), (1134, 1070, 5.3), (1184, 1078, 5.1),
        (1232, 1063, 5.6), (1284, 1039, 5.6), (1150, 914, 4.4),
        (624, 1009, 5.5), (701, 1029, 6.1), (826, 1015, 6.9),
    ]:
        baeume.append((np.asarray(n(x, z)), groesse))
    return flaeche, buchung, baeume


def katharinen_karte():
    flaeche = Flaechenkarte(KATHARINEN_GRENZEN, 0.25)

    def p(x, z):
        return (-886.8 + 0.2744 * x - 0.0048 * z,
                835.4 + 0.0068 * x + 0.3047 * z)

    def polygon(farbe, *punkte):
        flaeche.polygon([p(*punkt) for punkt in punkte], farbe)

    def linie(farbe, breite, *punkte):
        flaeche.linie([p(*punkt) for punkt in punkte], breite, farbe)

    polygon(RASEN, (142, 728), (319, 695), (375, 901), (195, 935))
    polygon(PFLASTER, (341, 807), (407, 786), (465, 715), (502, 860), (382, 907))
    polygon(ROT, (321, 721), (390, 705), (414, 800), (347, 815))
    for a, b in [((329, 730), (384, 717)), ((384, 717), (404, 792)),
                 ((404, 792), (351, 805)), ((351, 805), (329, 730)), ((340, 767), (394, 754))]:
        linie(WEISS, 0.17, a, b)
    flaeche.kreis(p(368, 761), 1.2, WEISS)
    flaeche.kreis(p(368, 761), 1.04, ROT)
    linie(ROT, 4.6, (151, 733), (198, 935))
    for x in [145, 150, 155]:
        linie(WEISS, 0.15, (x, 738), (x + 47, 932))
    linie(WEG, 2.2, (418, 691), (447, 769), (465, 827), (477, 889))
    # Öffentliche Grünachse östlich des Geländes und die Verbindung zum Wohnquartier.
    polygon(RASEN, (803, 608), (879, 585), (921, 985), (847, 1006))
    linie(WEG, 1.65, (799, 625), (786, 711), (798, 833), (836, 1009),
          (872, 1071), (868, 1136), (830, 1230), (803, 1298))
    linie(WEG, 1.85, (700, 982), (753, 1014), (827, 1013), (947, 1009), (998, 1003))
    polygon(KIES, (710, 641), (736, 615), (783, 624), (786, 675), (752, 697), (701, 683))
    linie(WEG, 1.8, (708, 615), (760, 614), (792, 651), (781, 692), (740, 711), (701, 715))
    polygon(PFLASTER, (978, 395), (1029, 342), (1237, 303), (1294, 357),
            (1261, 412), (1100, 452))
    baeume = [(np.asarray(p(x, z)), groesse) for x, z, groesse in [
        (166, 704, 7.4), (212, 701, 8.3), (254, 695, 8.0), (300, 690, 7.8),
        (410, 719, 6.3), (421, 805, 6.7), (403, 896, 6.1), (341, 915, 7.4),
        (267, 928, 6.8), (211, 932, 7.0), (559, 720, 7.9), (562, 772, 6.4),
        (603, 839, 7.7), (822, 1024, 6.5), (853, 1098, 8.9), (813, 1174, 7.7),
        (737, 1239, 7.9), (803, 1331, 8.2),
    ]]
    return flaeche, baeume
