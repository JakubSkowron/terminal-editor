#!/bin/bash

g++ -std=c++14 -o examine_esc_sequences -I.. examine_esc_sequences.cpp ../text_ui/terminal_io.cpp -pthread

#TODO: remove std::thread dependency from terminal_io

