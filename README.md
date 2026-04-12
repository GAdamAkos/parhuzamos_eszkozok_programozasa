# Mandelbrot OpenCL — Párhuzamos eszközök beadandó

## Projekt leírása

A projekt célja egy Mandelbrot-halmazt megjelenítő program készítése C nyelven, OpenCL használatával.

A program egy 2 dimenziós képrács minden pontjához hozzárendel egy komplex számot, majd kiszámítja, hogy az adott ponthoz tartozó iterációs sorozat konvergál-e vagy divergenssé válik. Az eredmény alapján a program kirajzolja a Mandelbrot-halmaz képét.

A feladat jól párhuzamosítható, mert az egyes pixelek számítása egymástól függetlenül elvégezhető. Emiatt a program GPU-n futtatott OpenCL kernel segítségével számolja ki a képet.

## A projekt célja

A beadandó célja egy olyan OpenCL-alapú program elkészítése, amely:

- helyesen előállítja a Mandelbrot-halmaz képét
- a számításokat párhuzamos módon hajtja végre
- alkalmas futásidő mérések készítésére
- jól dokumentálható eredményeket ad különböző paraméterek mellett

A projekt nemcsak a helyes működésre, hanem a párhuzamos végrehajtás vizsgálatára is épül.

## A program működése

A program egy `Width × M` méretű képrácson dolgozik.

Minden pixelhez:

- hozzárendel egy pontot a komplex síkon
- elvégzi a Mandelbrot-iterációt
- megszámolja, hogy hány lépés után lépi túl az érték a megadott határt
- ezt az eredményt eltárolja a kimeneti tömbben

Az iteráció alapja:

`z(n+1) = z(n)^2 + c`

ahol:

- `z` az iterált komplex szám
- `c` az adott pixelhez tartozó komplex pont

A számítás eredménye az úgynevezett escape-time érték, amely alapján a kép később előállítható.

## Miért alkalmas a feladat párhuzamosításra

A Mandelbrot-halmaz generálása tipikus adatpárhuzamos feladat:

- minden pixelre ugyanaz a számítási séma fut
- az egyes pixelek egymástól függetlenek
- a feladat természetesen felosztható sok szálra
- a számítási idő jól vizsgálható felbontás és iterációszám szerint

Ezért a feladat alkalmas GPU-alapú OpenCL megvalósításra és teljesítménymérésre.

## Megvalósítási koncepció

A projekt két fő részből áll:

### Host program C nyelven

A host oldal feladatai:

- OpenCL platformok és eszközök lekérdezése
- megfelelő GPU kiválasztása
- context létrehozása
- command queue létrehozása
- kernel forrás betöltése
- program fordítása
- buffer-ek lefoglalása
- kernel paraméterezése
- NDRange futtatás
- eredmények visszaolvasása
- futási idők mérése

### Kernel program OpenCL C nyelven

A kernel feladatai:

- az aktuális pixel koordinátáinak meghatározása
- a pixel leképezése a komplex síkra
- a Mandelbrot-iteráció végrehajtása
- az escape-time érték kiszámítása
- az eredmény kiírása a kimeneti pufferbe

## Mérési lehetőségek

A projekt egyik fontos része a teljesítményvizsgálat.

A program alkalmas lehet az alábbi paraméterek szerinti mérésekre:

- különböző felbontások
- különböző maximális iterációszámok
- eltérő globális work size
- eltérő lokális work-group méretek

A mérések alapján vizsgálható:

- a kernel futási ideje
- a teljes GPU-s végrehajtási idő
- a paraméterek hatása a teljesítményre
- a skálázódás nagyobb problémaméretek esetén
- összehasonlítás CPU-s megoldással

## Technikai jellemzők

- Nyelv: C
- Párhuzamos API: OpenCL
- Kernel nyelv: OpenCL C
- Build rendszer: make
- Kétdimenziós rács: `Width × M`
- Végrehajtás: GPU

## Projektstruktúra

```text
/mandelbrot_opencl
  /include
  /src
  /kernels
  /output
  Makefile
README.md
```

## Belépési pont

`src/main.c`

## Fejlesztési környezet

A projekt a tantárgyhoz kapott C/OpenCL fejlesztői környezetben készül.

A fordításhoz szükséges OpenCL fejlécek és könyvtárak a kiadott SDK csomagban találhatók. A fejlesztés során a tantárgyi segédanyagok és a `me-courses` repository OpenCL példái is felhasználhatók referenciaanyagként.

## Fordítás és futtatás

A projektet a tantárgyhoz kapott fejlesztői környezeten belül ajánlott elhelyezni.

A fordítás a projekt gyökérmappájából történik:

```bash
make
```

A futtatás:

### Windows alatt

```bash
mandelbrot.exe
```

### Linux alatt

```bash
./mandelbrot.exe
```

## Jelenlegi állapot

A projekt első lépése egy stabil OpenCL inicializáló váz elkészítése.

A jelenlegi változat tartalmazza:

- OpenCL platformok felismerését
- GPU kiválasztását
- context létrehozását
- profiling engedélyezett command queue létrehozását
- moduláris projektstruktúrát

A Mandelbrot-számítás és a képgenerálás a következő fejlesztési lépésekben kerül megvalósításra.

## Összegzés

A projekt egy jól mérhető, jól dokumentálható OpenCL-alapú párhuzamosítási feladatot valósít meg.

A Mandelbrot-halmaz generálása alkalmas arra, hogy bemutassa a GPU-s párhuzamos végrehajtás működését, és lehetőséget adjon különböző futási paraméterek hatásának vizsgálatára.
