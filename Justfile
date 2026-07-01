run: build
    ./build/sim

build:
    cmake -Bbuild -GNinja
    cmake --build build