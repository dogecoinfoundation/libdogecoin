#!/bin/bash

target_path=$(grep `pwd` ~/.bashrc)
if [[ -z $target_path ]]; then
    echo "export PATH=\$PATH:$(pwd)" >> ~/.bashrc
    source ~/.bashrc
fi
