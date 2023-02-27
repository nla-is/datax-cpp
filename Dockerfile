FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y build-essential cmake libgrpc-dev libprotobuf-dev

RUN apt-get install -y libgrpc++-dev protobuf-compiler-grpc

RUN mkdir /src
WORKDIR /src
COPY . .

RUN mkdir /build
WORKDIR /build
RUN cmake /src
RUN make
