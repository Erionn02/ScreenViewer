FROM gcc:13

WORKDIR /ScreenViewer
RUN apt update
RUN apt install -y sudo cmake
COPY ./setup.sh .
RUN ./setup.sh
COPY ./conanfile.txt .
RUN conan install --build missing -pr screen-viewer-profile conanfile.txt
COPY . .
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build . -t ScreenViewerServer -- -j $(nproc --all)

CMD ["./build/bin/ScreenViewerServer"]