# Prova skärmens sidor på datorn

Kompilerar skärmens riktiga ritkod (`ui.h`, `vtpool.h`, `vtview.h`) med Arduino_GFX och U8g2:s typsnitt och
ritar en sida till en bild (800 × 480).

```
G=<Arduino_GFX>/src; U=<U8g2>/src/clib; S=../../../src/Faltloggen
for f in Arduino_G Arduino_GFX Arduino_DataBus; do g++ -std=gnu++17 -O1 -w -c -Istub -I$U -I$G $G/$f.cpp -o $f.o; done
gcc -O1 -w -c -I$U $U/u8g2_fonts.c -o u8g2_fonts.o
gcc -O1 -w -c $S/qrcodegen.c -o qrcodegen.o
g++ -std=gnu++17 -O1 -w -Istub -I$U -Iinc -I$G -I$S -o vtview_test vtview_test.cpp *.o
mkdir -p sd && PAGE=6 ./vtview_test ut.raw && python3 topng.py ut.raw ut.png
```

Sidorna (PAGE): 0 viloläge, 1 inställningar, 2 WiFi, 3 tangentbord, 4 diagnos, 5 redskap, 6 traktor, 7 karta,
8 kartinställningar, 9 startval, 10 terminal, 11 analys, 12 sifferfält, 13 fältrapport, 14 manual, 15 enhetsinfo,
16 MER, 17 feedback. `EN=1` ritar på engelska, `NATT=1` i nattläge. Fler val står överst i `main()` i `vtview_test.cpp`.

## Kartan och fältrapporten

`kartatest.cpp` simulerar en körning genom `karta.h` och ritar helskärmskartan; `falttest.cpp` gör dessutom
fältrapportens PDF.

```
python3 karta_pc.py karta_pc.h          # karta.h utan nätdelen
g++ -std=gnu++17 -O1 -w -Istub -I$U -Iinc -I$G -I$S -I. -o kartatest kartatest.cpp Arduino_G.o Arduino_GFX.o Arduino_DataBus.o u8g2_fonts.o
g++ -std=gnu++17 -O1 -w -Istub -I$U -Iinc -I$G -I$S -I. -o falttest falttest.cpp Arduino_G.o Arduino_GFX.o Arduino_DataBus.o u8g2_fonts.o qrcodegen.o -lz
mkdir -p sd sdk
./kartatest k.raw && python3 topng.py k.raw karta.png
RAPPORT=1 ./falttest k.raw && pdftoppm -r 70 -png sdk/RAPPORT/*.pdf rapport
```
Obs: `karta_pc.h` måste hittas före `karta.h` – kopiera filerna till en arbetsmapp om kompilatorn tar fel fil.
