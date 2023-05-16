CC = gcc

BIN = smon
# Directories which have header files.
INCDIRS = . include
# Directories which have source code files.
SRCDIRS = . src
# Object files stored here.
OBJDIR = obj
# Dependency files stored here.
#DEPDIR = dep
BINDIR = bin

# Flags to the compiler
CFLAGS = -Wall -Wextra -Werror
# Optimization level flag
OPTFLAGS = -O2
# Dependency flags to allow to detect header file changes.
DEPFLAGS = -MP -MD
# Tell compiler where to look for header files.
INCFLAGS := $(addprefix -I, $(INCDIRS))

# For every field in $SRCDIRS get $DIR/*.c
SRCFILES := $(foreach DIR,$(SRCDIRS),$(wildcard $(DIR)/*.c))
# Get base names for all source files.
SRCBASE  := $(notdir $(SRCFILES))
# Substitude every *.c in $SRCFILES to $OBJFILES/*.o
OBJFILES := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCFILES))
# Substitude every *.c in $SRCFILES to $DEPDIR/*.d (Inactive for now)
#DEPFILES := $(patsubst %.c,$(DEPDIR)/%.d,$(SRCFILES))

BINFILES := $(patsubst %.c,bin/%,$(SRCBASE))

#all: $(BIN)

all: $(BIN)

# used for debugging purpose: print variables.
makeinfo:
	$(info SRCFILES: $(SRCFILES))
	$(info OBJFILES: $(OBJFILES))
	$(info DEPFILES: $(DEPFILES))
	$(info INCFLAGS: $(INCFLAGS))
	$(info BINFILES: $(BINFILES))
	$(info SRCBASE: $(SRCBASE))

# All .c files into single file.
# link object files together. (we need .o files, $objfiles, before this though)
$(BIN): $(OBJFILES)
	$(CC) -o $@ $^

# if/when needed: generate object files with <-c>.
$(OBJFILES): $(OBJDIR)/%.o: %.c
	mkdir -p $(patsubst %,$(OBJDIR)/%,$(SRCDIRS))
	$(CC) $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) $(INCFLAGS) -c -o $@ $<


# Seperately all .c to executables (No dependency, no objects)
$(BINFILES): $(SRCFILES)
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(OPTFLAGS) $(INCFLAGS) $< -o $@
