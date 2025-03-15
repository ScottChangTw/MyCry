#export CROSS_COMPILE?=i686-w64-mingw32-

ifeq ($(CROSS_COMPILE),)
CC        = gcc
CPP       = g++
LD        = g++
else
CC          = $(CROSS_COMPILE)gcc
LD          = $(CROSS_COMPILE)gcc
endif

$(info CROSS_COMPILE=$(CROSS_COMPILE))

INCLUDE   = -Iinclude -Isrc

CFLAGS_FPNG = -march=native -msse4.1 -mpclmul

CFLAGS    =  -ggdb -O2 -Wall $(INCLUDE)

#CPPFLAGS = -Wnounused-variable
#LDFLAGS   = -g -LC:/MinGW/lib 
LDFLAGS   = -g --static -static-libstdc++ -static-libgcc -lpthread -lsodium -lpng -lz -Llib



# **********************************************
# Files to be compiled
# **********************************************

SRCDIR = src 
OBJDIR = obj

# Search Path for All Prerequisties
# If a file that is listed as a target or prerequisite does not exist in the current directory, 
# make searches the directories listed in VPATH for a file with that name.
VPATH		= $(SRCDIR)

SRC_C	= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c)) 
OBJ_C	= $(addprefix $(OBJDIR)/, $(notdir $(patsubst %.c, %.o, $(SRC_C))))
SRC_CPP	= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp)) 
OBJ_CPP	= $(addprefix $(OBJDIR)/, $(notdir $(patsubst %.cpp, %.o, $(SRC_CPP))))

OUT_F  = cry.exe

all:$(OBJ_C) $(OBJ_CPP)
	@echo "Linking   Flag : $(LDFLAGS)"
	@$(LD) $(OBJ_C) $(OBJ_CPP) -o $(OUT_F) $(LDFLAGS)
	@echo "     LINKING      :  $(OUT_F)"
	
$(OBJ_C) : $(OBJDIR)/%.o : %.c 
#	@echo "Compiling Flag : $(CFLAGS)"
	@echo "     COMPILING    :  $<"
	-@test -d $(OBJDIR) || mkdir $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_CPP) : $(OBJDIR)/%.o : %.cpp
	@echo "Compiling Flag : $(CFLAGS)"
	@echo "     COMPILING    :  $<"
	-@test -d $(OBJDIR) || mkdir $(OBJDIR)
	@$(CPP) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)/*
	rm -rf $(OUT_F) *.o *.out *~ *.bak
