"""Danziger Platz, Katharinenstraße und Lechufer aus dem amtlichen Orthofoto."""

import numpy as np

from stadtfreiflaechen import Flaechenkarte, INSEL, KIES, PFLASTER, RASEN, ROT, WEG, WEISS


PLATZ = np.array([-1014.70, 788.60])
GRENZEN = (-215.0, -95.0, 185.0, 1210.0)
ZUR_KARTE = np.array([[0.55, 0.8351647], [0.8351647, -0.55]])
FAHRBAHN = (30, 32, 34)
FLICKEN = (24, 26, 28)
BORD = (103, 100, 88)
KREUZUNGEN = (((-1014.7, 788.6), 31.0), ((-978.7, 813.8), 26.0), ((-939.5, 824.4), 20.0),
              ((-988.0, 884.2), 23.0), ((-625.0, 671.2), 19.0), ((-389.0, 352.9), 24.0),
              ((-221.2, 268.0), 22.0))
ACHSE = np.array([
    [-977.1, 812.7], [-956.7, 818.1], [-944.7, 819.6], [-932.1, 819.5], [-914.7, 815.9],
    [-869.5, 801.6], [-789.5, 775.5], [-763.5, 765.7], [-731.4, 752.1], [-677, 724.4],
    [-667.5, 718.9], [-661, 713.8], [-648.3, 700.9], [-623.5, 670.7], [-611.7, 657.8],
    [-586.2, 634.3], [-549.2, 602.5], [-525, 578.3], [-497.4, 546.6], [-478.4, 523.1],
    [-459.3, 496.2], [-446.2, 470.5], [-409.9, 385.1], [-398.8, 365.4], [-388.7, 352.9],
    [-383.3, 348], [-372.9, 340.8], [-341.8, 324.9], [-319.3, 314.2], [-290.6, 300.5],
    [-272.9, 292.1], [-214.1, 265], [-203.1, 260.3],
])


def katharinenachse_raster(punkt):
    return ZUR_KARTE @ (np.asarray(punkt) - PLATZ)


def danziger_ort(x, z):
    # 26,8 cm/px im DOP-Ausschnitt. Die Einmündung und die Baukanten an
    # der Breslauer Straße dienen als Kontrollpunkte im bestehenden Stadtnetz.
    dx, dz = x - 1168, z - 1927
    return PLATZ + [0.2685 * dx - 0.0028 * dz, 0.0028 * dx + 0.2679 * dz]


def bruecken_ort(x, z):
    # Gerechnet zwischen den beiden Auflagern im separaten Uferorthofoto.
    dx, dz = x - 568, z - 934
    return np.array([-203.10, 260.30]) + [0.22533 * dx + 0.00474 * dz,
                                           -0.00474 * dx + 0.22533 * dz]


def uferkante_ost():
    return [(*bruecken_ort(x, z), hoehe) for x, z, hoehe in [
        (888, 1, 4.24), (905, 90, 4.52), (930, 208, 4.52), (952, 328, 4.78),
        (969, 422, 4.60), (990, 520, 4.88), (1014, 624, 5.02), (1033, 710, 5.80),
        (1051, 775, 5.95), (1062, 834, 4.42), (1080, 903, 4.78), (1101, 978, 4.72),
        (1127, 1049, 4.32), (1150, 1113, 4.08),
    ]]


def karolinenwehr():
    return np.asarray([bruecken_ort(x, z) for x, z in [
        (427, 707), (445, 690), (487, 656), (531, 600), (558, 548), (586, 484), (613, 411),
        (638, 327), (669, 225), (704, 121), (745, 13), (774, -48), (804, -105),
    ]])


def katharinenachse_karte(kreuzungsdecke):
    flaeche = Flaechenkarte(GRENZEN, 0.32)
    markierungen = []
    for dreieck, mitte, radius in kreuzungsdecke:
        flaeche.polygon([katharinenachse_raster(p) for p in dreieck], FAHRBAHN,
                        kreis=(katharinenachse_raster(mitte), radius))

    def linie(farbe, breite, *punkte, bezug=lambda x, z: (x, z)):
        punkte = [np.asarray(bezug(*p)) for p in punkte]
        if farbe == WEISS:
            for a, b in zip(punkte, punkte[1:]):
                dx, dz = b - a
                quer = np.array([-dz, dx]) * (breite / (2 * np.hypot(dx, dz)))
                markierungen.append(np.array([a - quer, b - quer, b + quer, a + quer]))
        else:
            flaeche.linie([katharinenachse_raster(p) for p in punkte], breite, farbe)

    def polygon(farbe, *punkte, bezug):
        punkte = [bezug(*p) for p in punkte]
        if farbe == WEISS:
            markierungen.append(np.asarray(punkte))
        else:
            flaeche.polygon([katharinenachse_raster(p) for p in punkte], farbe)

    def dan_linie(farbe, breite, *p):
        linie(farbe, breite, *p, bezug=danziger_ort)

    def dan_polygon(farbe, *p):
        polygon(farbe, *p, bezug=danziger_ort)

    # Zwei unterschiedlich breite Arme laufen aus Nordwesten und Norden ein.
    # Die einzelnen Kanten folgen den Bordsteinen, inklusive der breiten Aufweitung.
    dan_polygon(FAHRBAHN, (970, 1634), (1044, 1634), (1125, 1774), (1151, 1800),
                (1150, 1757), (1128, 1655), (1182, 1655), (1204, 1894), (1226, 1941),
                (1284, 1983), (1380, 2007), (1460, 2015), (1553, 2017), (1553, 2058),
                (1462, 2060), (1370, 2050), (1290, 2032), (1241, 2020), (1231, 2046),
                (1250, 2254), (1214, 2254), (1188, 2031), (1161, 1941), (1107, 1847))
    dan_polygon(BORD, (1237, 2041), (1268, 2025), (1290, 2043), (1283, 2079), (1249, 2085))
    dan_polygon(INSEL, (1242, 2044), (1268, 2031), (1284, 2045), (1278, 2074), (1251, 2079))
    dan_polygon(BORD, (1333, 2057), (1370, 2059), (1454, 2073), (1431, 2094),
                (1387, 2131), (1347, 2152), (1329, 2120))
    dan_polygon(RASEN, (1338, 2062), (1372, 2064), (1445, 2076), (1428, 2090),
                (1381, 2129), (1350, 2146), (1334, 2118))
    dan_polygon(FAHRBAHN, (1282, 2249), (1344, 2158), (1412, 2110), (1470, 2060),
                (1513, 2059), (1512, 2082), (1466, 2127), (1374, 2191), (1331, 2252))
    dan_polygon(ROT, (1468, 2061), (1524, 2065), (1519, 2073), (1465, 2069))
    # Haltelinien, Leitlinien, kurze Sperrfläche und Furten vor dem Abbiegebereich.
    dan_linie(WEISS, 0.43, (1074, 1780), (1110, 1764))
    dan_linie(WEISS, 0.42, (1150, 1765), (1178, 1763))
    dan_linie(WEISS, 0.43, (1189, 2039), (1229, 2034))
    dan_linie(WEISS, 0.36, (1192, 2046), (1231, 2041))
    dan_linie(WEISS, 0.38, (1254, 1990), (1281, 2004))
    for x, z, dx, dz in [
        (1024, 1681, 8, 14), (1037, 1707, 8, 14), (1051, 1734, 8, 14), (1065, 1759, 7, 14),
        (1092, 1804, 8, 14), (1106, 1831, 8, 14), (1121, 1857, 8, 14), (1135, 1883, 8, 14),
        (1151, 1911, 8, 14), (1168, 1936, 6, 13), (1174, 1785, 2, 15), (1179, 1811, 2, 15),
        (1183, 1837, 2, 15), (1190, 1887, 2, 15), (1206, 2072, 2, 14), (1209, 2098, 2, 14),
        (1212, 2124, 2, 14), (1215, 2150, 2, 14), (1218, 2176, 2, 14), (1220, 2202, 2, 14),
        (1312, 2013, 16, 4), (1340, 2020, 16, 4), (1368, 2027, 16, 4), (1396, 2033, 16, 2),
        (1424, 2037, 16, 2), (1452, 2040, 16, 1), (1480, 2041, 16, 0), (1510, 2041, 16, 0),
    ]:
        dan_linie(WEISS, 0.25, (x, z), (x + dx, z + dz))
    for j in range(5):
        z = 1793 + 5 * j
        dan_linie(WEISS, 0.26, (1124 + j * 2, z), (1144 + j, z - 6))
    for j in range(8):
        x = 1466 + 8 * j
        dan_linie(WEISS, 0.26, (x, 2058), (x + 4, 2058))
    for x, z, dx, dz in [(1069, 1744, -4, -10), (1161, 1736, -1, -11),
                         (1224, 2080, -1, -11), (1211, 2074, 1, 11), (1327, 2006, -11, -3)]:
        ende = np.array([x + dx, z + dz])
        v = np.array([dx, dz]) / np.hypot(dx, dz)
        n = np.array([-v[1], v[0]])
        dan_linie(WEISS, 0.29, (x, z), ende)
        dan_polygon(WEISS, ende + 5 * v, ende - 1.5 * v + 3 * n, ende - 1.5 * v - 3 * n)

    # Belag und abschnittsweise erneuerte Fahrspuren vom Platz bis zum Brückenkopf.
    linie(FAHRBAHN, 6.6, *ACHSE)
    laengen = np.r_[0.0, np.cumsum(np.linalg.norm(np.diff(ACHSE, axis=0), axis=1))]

    def strassenpunkt(s, q=0.0):
        i = min(np.searchsorted(laengen[1:], s, side="right"), len(ACHSE) - 2)
        a, b = ACHSE[i:i + 2]
        v = (b - a) / (laengen[i + 1] - laengen[i])
        return a + v * (s - laengen[i]) + q * np.array([-v[1], v[0]])

    for s, q, laenge in [(65, 1.70, 8), (169, -1.64, 13), (247, 1.65, 9), (403, -1.30, 11),
                        (515, 1.68, 8), (655, -1.35, 6), (783, 1.65, 10), (887, -1.60, 6)]:
        linie(FLICKEN, 1.78, strassenpunkt(s, q), strassenpunkt(s + laenge, q))
    for s in range(19, int(laengen[-1]) - 11, 12):
        p = strassenpunkt(s)
        if s < 228 or s > 392:
            if all(np.linalg.norm(p - mitte) > 10 for mitte in ((-625, 671), (-389, 353), (-220, 268))):
                linie(WEISS, 0.20, p, strassenpunkt(s + 2.8))
    westkopf = bruecken_ort
    linie(ROT, 1.05, (460, 1004), (530, 973), (568, 954), bezug=westkopf)
    for x, z, dx, dz in [(426, 945, 21, 0), (470, 917, 18, 18)]:
        for j in range(5):
            linie(WEISS, 0.38, (x + dx * j / 5, z + dz * j / 5),
                  (x + dx * j / 5 - dz * 0.65, z + dz * j / 5 + dx * 0.65), bezug=westkopf)

    # Westufer: Grünanlage und wassergebundene Wege, darunter die Kiesbank am Wehr.
    polygon(RASEN, (291, 580), (347, 607), (397, 653), (453, 705), (510, 786),
            (550, 862), (566, 900), (511, 878), (431, 810), (369, 756),
            (332, 716), (286, 676), (260, 616), bezug=westkopf)
    linie(WEG, 2.9, (252, 647), (289, 665), (317, 700), (337, 733), (370, 757),
          (433, 803), (476, 844), (524, 865), (560, 905), bezug=westkopf)
    linie(KIES, 1.65, (336, 732), (344, 684), (369, 630), (404, 660), (442, 699), bezug=westkopf)
    polygon(KIES, (363, 627), (390, 610), (418, 649), (449, 667), (445, 700),
            (405, 661), bezug=westkopf)
    linie(KIES, 2.4, (552, 991), (584, 1058), (619, 1126), (642, 1194), (672, 1270),
          (704, 1340), (722, 1395), bezug=westkopf)
    # Östliche befestigte Promenade unterhalb der Altstadthäuser.
    linie(PFLASTER, 2.5, (1060, 781), (1073, 832), (1091, 894), (1116, 965),
          (1146, 1044), (1169, 1114), bezug=westkopf)
    baeume = [(westkopf(x, z), h) for x, z, h in [
        (304, 616, 8.5), (353, 585, 10.5), (370, 703, 8.9), (417, 729, 7.8),
        (458, 766, 10.1), (488, 787, 9.3), (524, 834, 8.6), (540, 862, 7.2),
        (601, 1018, 10.8), (631, 1103, 12.4), (669, 1189, 9.8), (690, 1245, 8.0),
    ]]
    baeume.extend((danziger_ort(x, z), h) for x, z, h in [
        (1130, 1868, 6.2), (1231, 1852, 6.7), (1229, 1901, 6.1), (1157, 2004, 7.4),
        (1136, 2050, 8.2), (1128, 2094, 8.5), (1362, 2080, 5.1), (1375, 2100, 6.3),
    ])
    # Die Grüninseln bekommen dieselbe niedrige Bepflanzung wie die Mittelstreifen.
    inseln = [danziger_ort(x, z) for x, z in [(1260, 2050), (1265, 2063), (1353, 2080),
                                                   (1372, 2081), (1397, 2081), (1360, 2114)]]
    return flaeche, inseln, baeume, markierungen
