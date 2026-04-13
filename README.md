# Mandelbrot OpenCL — Párhuzamos eszközök beadandó

## Projekt leírása

A projekt célja egy Mandelbrot-halmazt megjelenítő program készítése C nyelven, OpenCL használatával.

A program egy 2 dimenziós képrács pontjaihoz komplex számokat rendel, majd minden pixelre kiszámítja a Mandelbrot-iterációt. Az eredmény alapján előállítható a Mandelbrot-halmaz képe.

A feladat jól párhuzamosítható, mert az egyes pixelek számítása egymástól függetlenül végezhető el, ezért a program GPU-n futtatott OpenCL kernel segítségével dolgozik.

## A projekt célja

A beadandó célja egy olyan OpenCL-alapú program elkészítése, amely:

- helyesen előállítja a Mandelbrot-halmaz képét
- a számítást párhuzamos módon hajtja végre
- alkalmas futásidő mérések készítésére
- jól dokumentálható eredményeket ad

## A program működése

A program egy `Width × M` méretű képrácson dolgozik.

Minden pixelhez:

- hozzárendel egy pontot a komplex síkon
- elvégzi a Mandelbrot-iterációt
- meghatározza az escape-time értéket
- eltárolja az eredményt a kimeneti tömbben

Az iteráció alapja:

`z(n+1) = z(n)^2 + c`

A GPU-s megvalósításban egy work-item egy pixel kiszámításáért felel.

## Miért alkalmas a feladat párhuzamosításra

A Mandelbrot-halmaz generálása tipikus adatpárhuzamos feladat, mert:

- minden pixelre ugyanaz a számítás fut
- az egyes pixelek egymástól függetlenek
- a teljesítmény jól vizsgálható felbontás és iterációszám szerint

Ezért a feladat alkalmas OpenCL-alapú GPU-s megvalósításra és teljesítménymérésre.

## Technikai jellemzők

- Nyelv: C
- Párhuzamos API: OpenCL
- Kernel nyelv: OpenCL C
- Build rendszer: make
- Rácsméret: `Width × M`
- Végrehajtás: GPU

## Projektstruktúra

```text
/parhuzamos_eszkozok_programozasa
  /app
    /include
    /src
    /kernels
    /output
    Makefile
README.md
```

## Belépési pont

`app/src/main.c`

## Fejlesztési környezet

A projekt a tantárgyhoz kapott `c_sdk_220203` fejlesztői környezetben készül.

A fordításhoz szükséges OpenCL fejlécek és könyvtárak a csomagban található `MinGW` mappában érhetők el.

## Elhelyezés

A projektet a `c_sdk_220203` mappán belül ajánlott elhelyezni az alábbi szerkezetben:

```text
c_sdk_220203/
  MinGW/
  parhuzamos/
    parhuzamos_eszkozok_programozasa/
      app/
      README.md
```

Ez szükséges ahhoz, hogy a `Makefile` a megfelelő include és library útvonalakat használja.

## Fordítás és futtatás

A fordítást az `app` mappából kell indítani:

```bash
make clean
make
```

Futtatás Windows alatt:

```bash
mandelbrot.exe
```

## Fontos megjegyzés

A projekt a `c_sdk_220203` csomagban található MinGW fordítóra épül. Más telepített toolchainnel, például MSYS2 / UCRT64 környezetben, header- és könyvtárütközés fordulhat elő.