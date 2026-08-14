# complier.
CC = cc

# instalation prefix.
PREFIX = /usr/local

# includes and libs.
INCS =
LIBS = -lpulse -ludev -lm

# flags
CFLAGS = -std=c23 -pedantic -Wall -Os $(INCS)
LDFLAGS = $(LIBS)
