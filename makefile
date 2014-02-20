all:life.exe

life.obj: life.c ; icc /Fi+ /Si+ /Gd- /Gf+ /Gi+ /Gs+ /O+ /Ti+ /Q+ /C+ life.c

life.exe: life.obj life.def ; link386 /DEBUG life.obj,life.exe,,,life.def

