# Práctica 3. Algoritmos Voraces y Codificación Huffman

ESCOM-IPN · Ingeniería en Inteligencia Artificial · Análisis y Diseño de Algoritmos · Grupo 3BV1
Alumna: Molina Zavaleta Angie Lalaine

## Compilar
    g++ -O2 -std=c++17 huffman.cpp -o huffman

## Usar
    ./huffman c entrada salida.huf     # comprimir
    ./huffman d salida.huf recuperado  # descomprimir
    ./huffman selftest                 # mensaje de la práctica

## Formato del archivo
`"HUF1"` (4 B) | N = nº de caracteres originales (uint64) | árbol en preorden (0 = interno, 1 + 8 bits = hoja) | datos | relleno con ceros

## Contenido
- `huffman.cpp`: compresor/descompresor, BitWriter/BitReader
- `bench.sh`: genera los casos de benchmark y verifica la descompresión
- `reporte/`: reporte en PDF
- `docs/conversacion_IA.md`: conversación con la herramienta de IA
