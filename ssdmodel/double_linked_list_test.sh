#!/bin/bash

make double_linked_list.o
make double_linked_list_test.o

gcc -m32 double_linked_list.o double_linked_list_test.o -o dll_test

gdb ./dll_test

rm dll_test

