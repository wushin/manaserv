FROM fedora:29 as builder

RUN dnf install -y cmake gcc-c++ make libxml2-devel physfs-devel sqlite-devel \
    lua-devel libsigc++20-devel

ADD . /source
WORKDIR /source

RUN cmake . -DCMAKE_INSTALL_PREFIX=/app
RUN make install

FROM fedora:29
RUN dnf update -y
RUN dnf install -y libxml2 physfs sqlite lua libsigc++20
COPY --from=builder /app/ /app/
