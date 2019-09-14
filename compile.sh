#g++ -g\
#g++ -O3 -march=native -flto -fopenmp -funroll-loops\
#icpc -flto -xHost -qopenmp -ipo -O3 -Wall\
g++ -O3 -march=native -flto -funroll-loops\
    src/main.cpp\
    src/zseb.cpp\
    src/huffman.cpp\
    src/lzss.cpp\
    src/crc32.cpp\
    src/stream.cpp -o zseb

