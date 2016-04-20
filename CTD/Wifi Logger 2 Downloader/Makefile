# ------------------------------------------------
# Author: Jean Michel Leconte
# Date  : 2014-07-14
#
# Changelog :
# ------------------------------------------------

# 
TARGET   = logger2wifidownloader

CC       = gcc
# compiling flags here
CFLAGS   = -Wall -I. -std=gnu99

LINKER   = gcc -o
# linking flags here
LFLAGS   = -Wall -I. -lm

# directories
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

MKDIR_P = mkdir -p

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -rf

all: directories $(BINDIR)/$(TARGET)

directories: ${OBJDIR} ${BINDIR} 

${OBJDIR}:
	@${MKDIR_P} ${OBJDIR}

${BINDIR}:
	@${MKDIR_P} ${BINDIR}

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@$(rm) $(OBJDIR)
	
.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)
	