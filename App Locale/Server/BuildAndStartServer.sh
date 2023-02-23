#! /bin/bash

gcc -pthread -Wall -o Server -I /usr/local/opt/libpq/include/ Libs/Structures/Data_Structures.c Libs/DB/Database_Utils.c Main.c -lpq && ./Server ;
