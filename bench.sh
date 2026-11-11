#!/usr/bin/env bash
# Genera casos de prueba, comprime, descomprime y verifica con cmp.
set -e
g++ -O2 -std=c++17 huffman.cpp -o huffman
mkdir -p bench && cd bench
head -c 1000000 /dev/urandom > aleatorio.bin
head -c 1000000 /dev/zero | tr '\0' 'A' > solo_A.txt
: > vacio.txt
[ -f texto.txt ] || echo "Coloca aquí el Quijote como bench/texto.txt"
for f in texto.txt aleatorio.bin solo_A.txt vacio.txt; do
  [ -f "$f" ] || continue
  echo "== $f"
  ../huffman c "$f" "$f.huf" && ../huffman d "$f.huf" "$f.out" && cmp "$f" "$f.out" && echo "OK"
done
