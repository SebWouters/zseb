#g++ -g -Wall\
#icpc -flto -xHost -qopenmp -ipo -O3 -Wall\
g++ -O3 -march=native -flto -funroll-loops -Wall\
    src/main.cpp\
    src/zseb.cpp\
    src/huffman.cpp\
    src/lzss.cpp\
    src/stream.cpp -o zseb

