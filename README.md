# Persistance Demo

Small demo of persistent / resumable programming techniques.

The idea is to store all program state in a memory mapped file, so that when the program closes it can start and continue where it left off. Obviously, some things cannot be directly resumed, so this has to be taken care of when the program initializes.

It turns out that in C, it is extraordinarily simple to get this to work.