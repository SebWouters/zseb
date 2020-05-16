#g++ -g -pthread -Wall\
#icpc -flto -xHost -qopenmp -ipo -O3 -Wall\
g++ -O3 -pthread -march=native -flto -funroll-loops -Wall\
    src/main.cpp\
    src/zseb.cpp\
    src/huffman.cpp -o zseb

