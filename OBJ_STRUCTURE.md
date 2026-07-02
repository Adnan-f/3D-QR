# Structure du `cube.obj` — dimensions réelles

Toutes les valeurs sont proportionnelles à `cubeSize` (longueur d'une
arête du cube). Le tableau ci-dessous donne les valeurs numériques pour
les paramètres par défaut (`cubeSize = 2.0`) ; pour une taille réelle
différente (ex. en mm), il suffit de multiplier chaque valeur par
`cubeSize_réel / 2.0` (tout est strictement linéaire).

## 1. Repère et niveaux de hauteur (axe normal à chaque face)

| Niveau | Hauteur / distance au centre du cube | Valeur (cubeSize=2.0) |
|---|---|---|
| Panneau de fond (face nominale du cube) | `cubeSize/2` | 1.000 |
| Sommet d'une tuile (bordure + plancher) | `cubeSize/2 + tileRaiseHeight` | 1.050 |
| Sommet d'un plot/bit "actif" | `cubeSize/2 + tileRaiseHeight + pegHeight` | 1.085 |

`tileRaiseHeight` et `pegHeight` sont les deux paramètres de hauteur
dans `GeometryParams` (`Geometry.h`).

## 2. Grille 3x3 de tuiles (taille réelle des tuiles, espacement)

| Mesure | Formule | Valeur (cubeSize=2.0) |
|---|---|---|
| Pas de grille (cellule) | `cubeSize / 3` | 0.6667 |
| Jeu total entre deux tuiles voisines | `cellule × tileGapRatio` | 0.0667 |
| **Marge bord du cube → 1ʳᵉ tuile** | `jeu / 2` | **0.0333** |
| **Espacement entre deux tuiles voisines** | `jeu` | **0.0667** |
| Côté réel d'une tuile (jusqu'au bord de la bordure) | `cellule − jeu` | 0.6000 |

La marge extérieure (bord du cube ↔ première tuile) vaut donc la
**moitié** du jeu inter-tuiles, car le jeu de `tileGapRatio` est répart
i également des deux côtés de chaque tuile dans sa cellule.

## 3. Bordure d'une tuile (distance bordure → zone de données)

| Mesure | Formule | Valeur (cubeSize=2.0) |
|---|---|---|
| **Épaisseur de la bordure (rim)** = distance entre le bord de la tuile et la 1ʳᵉ donnée (bit) | `côté tuile × tileRimRatio` | **0.1080** |
| Côté de la zone utile (4x4 bits) | `côté tuile − 2×bordure` | 0.3840 |

La bordure est une bande de largeur constante sur les 4 côtés de la
tuile : il n'y a **aucun jeu supplémentaire** entre la bordure et le
premier bit — le plancher utile commence juste après.

## 4. Grille 4x4 de données/bits (taille et espacement de chaque bit)

| Mesure | Formule | Valeur (cubeSize=2.0) |
|---|---|---|
| Pas centre-à-centre entre deux bits (cellule de bit) | `zone utile / 4` | 0.0960 |
| Marge par côté autour d'un bit (dans sa cellule) | `cellule de bit × pegMarginRatio` | 0.0192 |
| **Côté réel d'un plot/bit "actif"** | `cellule de bit − 2×marge` | **0.0576** |
| **Espacement (jeu) entre deux bits voisins** | `2 × marge` | **0.0384** |

➡️ Donc dans une tuile : 4 bits de **0.0576** de large, espacés de
**0.0384** entre eux, sur un total de **0.3840** de largeur utile,
elle-même précédée d'une bordure de **0.1080**.

## 5. Récapitulatif visuel (une tuile, vue de dessus, en unités cubeSize=2.0)

```
|<-- bordure 0.108 -->|<--------- zone utile 0.384 (4 bits) --------->|<-- bordure 0.108 -->|
                       |bit 0.0576|gap 0.0384|bit 0.0576|gap 0.0384|bit 0.0576|gap 0.0384|bit 0.0576|
```

## 6. Calculer la position monde (x,y,z) d'un bit donné

Pour la face `f` (repère local `u`,`v`,`n` défini dans `Geometry.cpp`),
le centre du bit `(tileRow, tileCol, pegRow, pegCol)` est à
`P = n*(cubeSize/2 + h) + u*s + v*t` avec, en partant du coin
`(-cubeSize/2, -cubeSize/2)` de la face :

```
cell      = cubeSize/3
gap       = cell * tileGapRatio
tileFoot  = cell - gap
rim       = tileFoot * tileRimRatio
innerW    = tileFoot - 2*rim
pegCell   = innerW / 4
pegMargin = pegCell * pegMarginRatio

tileS0 = -cubeSize/2 + tileCol*cell + gap/2
tileT0 = -cubeSize/2 + tileRow*cell + gap/2
floorS0 = tileS0 + rim
floorT0 = tileT0 + rim

s_centre = floorS0 + pegCol*pegCell + pegCell/2
t_centre = floorT0 + pegRow*pegCell + pegCell/2
h        = tileRaiseHeight + (pegHeight si bit=1, sinon 0 → plancher seul)
```

## 7. Adapter à une taille réelle (ex. mm)

Tout est linéaire en `cubeSize`. Pour une arête réelle de, par
exemple, 60 mm, fixez `cubeSize = 60.0` dans `GeometryParams` (et
gardez les mêmes `tileRaiseHeight`/`pegHeight` en mm) — toutes les
distances ci-dessus se multiplient alors simplement par `60/2 = 30`.
