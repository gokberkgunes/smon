CC = gcc

BIN = smon
# Directories which have header files.
INCDIRS = .
# Directories which have source code files.
SRCDIRS = . lib
# Object files stored here.
OBJDIR = obj
# Dependency files stored here.
#DEPDIR = dep

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
# Substitude every *.c in $SRCFILES to $OBJFILES/*.o
OBJFILES := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCFILES))
# Substitude every *.c in $SRCFILES to $DEPDIR/*.d (Inactive for now)
#DEPFILES := $(patsubst %.c,$(DEPDIR)/%.d,$(SRCFILES))

all: $(BIN)

# used for debugging purpose: print variables.
makeinfo:
	$(info SRCFILES: $(SRCFILES))
	$(info OBJFILES: $(OBJFILES))
	$(info DEPFILES: $(DEPFILES))
	$(info INCFLAGS: $(INCFLAGS))

# link object files together. (we need .o files, $objfiles, before this though)
$(BIN): $(OBJFILES)
	$(CC) -o $@ $^

# if/when needed: generate object files with <-c>.
$(OBJFILES): $(OBJDIR)/%.o: %.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) $(INCFLAGS) -c -o $@ $<
