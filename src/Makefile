# Lab 12 — Speedup (CC3069)
# Linux / WSL: gcc ya trae OpenMP.
# macOS: brew install libomp   (o: brew install gcc  y luego  make CC=gcc-14)

UNAME := $(shell uname)

ifeq ($(UNAME),Darwin)
  ifeq ($(CC),cc)
    CC = clang
  endif
  ifneq (,$(findstring clang,$(CC)))
    LIBOMP := $(shell brew --prefix libomp 2>/dev/null)
    CFLAGS = -O2 -Wall -Xpreprocessor -fopenmp -I$(LIBOMP)/include
    LDFLAGS = -L$(LIBOMP)/lib -lomp
  else
    CFLAGS = -O2 -Wall -fopenmp
  endif
else
  CC = gcc
  CFLAGS = -O2 -Wall -fopenmp
endif

lab12: lab12_speedup.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f lab12 *.png *_con_S_E.csv

.PHONY: clean
