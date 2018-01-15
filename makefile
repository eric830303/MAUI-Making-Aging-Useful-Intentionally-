# Makefile
# Project: MAUI
# Author: Ting-Wei Chang
# Date: 2017/04/20

CXX := g++
INCLUDE  = . 
CXXFLAGS = -std=c++11 -g -w
#CXXFLAGS = -std=c++11 -g -w
OBJDIR	:= obj
BINDIR	:= bin
SRCDIR	:= src
SRC		:= $(wildcard $(SRCDIR)/*.cpp)
INC		:= $(wildcard $(SRCDIR/*.cpp))
OBJ		:= $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
#CXXFLAGS += -Wall -g 


EXE  := maui
SRCS := maui.cpp
OUTPUTFILE := output_file
MINISAT    := minisat
OS		   := $(shell uname)

#---------------- COMPILE -----------------------------------
all: info $(EXE)
	@printf "\033[32m[Info]:\033[0m Complete! Execution file:"
	@printf "\033[92;1m %s\033[0m\n" "$(EXE)"
$(EXE): $(OBJ)
	@$(CXX) $(CXXFLAGS) -o $(EXE) $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INC)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

#----------------- CLEAN --------------------------------------	
cleanall: clean deloutput
	@printf "\033[32m[Info]:\033[0m Finish.\n"

clean: clear
	@printf "\033[32m[Info]:\033[0m Clean all SAT and obj files.\n"
	@rm -rf *.out $(EXE)
	@rm -rf *_output
clear:
	@printf "\033[32m[Info]:\033[0m Clean all obj files.\n"
	@rm -rf $(OBJDIR)/*.o

deloutput:
	@printf "\033[32m[Info]:\033[0m Delete all output files.\n"
	@rm -rf *.out
	@rm -rf *_output
#--------------------- CHECK -----------------------------------
info:
	@clear
	@printf " **************** Information ****************\n"
	@printf " * Project: MAUI                             *\n"
	@printf "\033[31;1m **************** !!Notice!! *****************\n"
	@printf " * Available compilation with g++-4.8(c++11) *\n"
	@printf " * \033[0mThe version of your compiler  : \033[33;1mg++-"
	@printf "`g++ --version | grep g++ | cut -d ')' -f 2 | cut -d ' ' -f 2` \033[31;1m*\n"
	@printf " * \033[0m"

  ifeq ($(OS),Darwin)
	@cp ./MINISATs/minisat_osx ./minisat
  else
	@cp ./MINISATs/minisat_linux ./minisat
  endif
run:
	./$(EXE) -h

