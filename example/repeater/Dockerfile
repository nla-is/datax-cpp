FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        git \
        g++ \
        libgrpc++-dev \
        libprotobuf-dev \
        make \
        protobuf-compiler-grpc \
    && apt-get clean

RUN mkdir /src

ADD CMakeLists.txt /src

RUN mkdir /build
WORKDIR /build

ADD main.cpp /src

RUN cmake /src && make

FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libgrpc++1.51 \
    && apt-get clean

COPY --from=build /build/repeater /repeater

ENTRYPOINT ["/repeater"]
