FROM ubuntu:latest

RUN apt-get update -y && apt-get install -y \
g++ \
valgrind \
gdb \
fish \
siege \ 
make


COPY . /home/ubuntu/Desktop/webserve/.


