#!/bin/bash

if ! command -v cmake &> /dev/null; then
  echo "Installing CMake..."
  sudo apt update
  sudo apt install -y cmake
fi

if ! command -v g++ &> /dev/null; then
  echo "Installing g++..."
  sudo apt update
  sudo apt install -y g++
fi

if ! command -v make &> /dev/null; then
  echo "Installing build-essential..."
  sudo apt update
  sudo apt install -y build-essential
fi

if [ ! -d "submodules/asio" ]; then
  echo "Cloning Boost.Asio..."
  git clone https://github.com/chriskohlhoff/asio.git submodules/asio
else
  echo "Boost.Asio submodule already exists."
fi

if [ ! -d "submodules/plog" ]; then
  echo "Cloning Plog..."
  git clone https://github.com/SergiusTheBest/plog.git submodules/plog
else
  echo "Plog submodule already exists."
fi

if [ ! -d "submodules/jsoncpp" ]; then
  echo "Cloning Jsoncpp..."
  git clone https://github.com/open-source-parsers/jsoncpp.git submodules/jsoncpp
else
  echo "Jsoncpp submodule already exists."
fi

if [ -d "build_release" ]; then
  echo "Cleaning build_release..."
  rm -rf build_release
fi

mkdir -p build_release
cd build_release

cmake -DCMAKE_BUILD_TYPE=Release ..
make -j

cd ..

if [ -d "build_debug" ]; then
  echo "Cleaning build_debug..."
  rm -rf build_debug
fi

mkdir -p build_debug
cd build_debug

cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j

cd ..
if [ -d "build_relwithdebinfo" ]; then
  echo "Cleaning build_relwithdebinfo..."
  rm -rf build_relwithdebinfo
fi

mkdir -p build_relwithdebinfo
cd build_relwithdebinfo

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j