#!/bin/bash

g++ -std=c++14 -c draw_grid.cpp 
g++ -o draw_grid draw_grid.o ../text_ui/libtext_ui.a
