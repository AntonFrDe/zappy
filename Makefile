##
## Zappy - Makefile
##

SERVER_NAME	=	zappy_server
GUI_NAME	=	zappy_gui
AI_NAME		=	zappy_ai

# ---------------------------------------------------------------------------
# Server (C)
# ---------------------------------------------------------------------------
SERVER_SRC_DIR	=	src/server
SERVER_INC_DIR	=	include/server

SERVER_SRC	=	$(wildcard $(SERVER_SRC_DIR)/*.c) $(wildcard $(SERVER_SRC_DIR)/commands/*.c)
SERVER_OBJ	=	$(SERVER_SRC:.c=.o)

CC		=	gcc
CFLAGS		+=	-Wall -Wextra -I$(SERVER_INC_DIR)

# ---------------------------------------------------------------------------
# GUI (C++)
# ---------------------------------------------------------------------------
GUI_SRC_DIR	=	src/gui
GUI_INC_DIR	=	include/gui

GUI_SRC		=	$(wildcard $(GUI_SRC_DIR)/*.cpp)
GUI_OBJ		=	$(GUI_SRC:.cpp=.o)

CXX		=	g++
CXXFLAGS	+=	-Wall -Wextra -I$(GUI_INC_DIR)
GUI_LDLIBS	=	-lsfml-graphics -lsfml-window -lsfml-system

# ---------------------------------------------------------------------------
# AI (Python)
# ---------------------------------------------------------------------------
AI_SRC_DIR	=	src/ai
AI_MAIN		=	$(AI_SRC_DIR)/main.py
AI_SRC		=	$(wildcard $(AI_SRC_DIR)/*.py)

# ---------------------------------------------------------------------------
# Tests (Criterion)
# ---------------------------------------------------------------------------
CRITERION_DIR	=	/usr/local/criterion-2.4.2
CRITERION_CFLAGS=	-I$(CRITERION_DIR)/include
CRITERION_LDLIBS=	-L$(CRITERION_DIR)/lib -Wl,-rpath,$(CRITERION_DIR)/lib -lcriterion

TESTS_NAME	=	tests_server

TESTS_SRC_DIR	=	tests/server
TESTS_SRC	=	$(wildcard $(TESTS_SRC_DIR)/*.c)
TESTS_PARSER_SRC=	$(filter-out $(SERVER_SRC_DIR)/main.c, $(SERVER_SRC))

# -I. et -Iinclude : nécessaires tant que src/server mélange
# #include <server/server.h> et #include <include/server/server.h>
TESTS_CFLAGS	=	-Wall -Wextra -I. -Iinclude $(CRITERION_CFLAGS)
TESTS_LDLIBS	=	$(CRITERION_LDLIBS) -lpthread -lm

TESTS_GUI_NAME	=	tests_gui

TESTS_GUI_SRC_DIR=	tests/gui
TESTS_GUI_SRC	=	$(wildcard $(TESTS_GUI_SRC_DIR)/*.cpp)
TESTS_GUI_PARSER_SRC=	$(filter-out $(GUI_SRC_DIR)/main.cpp, $(GUI_SRC))

TESTS_GUI_CXXFLAGS=	-Wall -Wextra -I$(GUI_INC_DIR) $(CRITERION_CFLAGS)
TESTS_GUI_LDLIBS=	$(CRITERION_LDLIBS) -lpthread $(GUI_LDLIBS)

# ---------------------------------------------------------------------------
# Rules
# ---------------------------------------------------------------------------
all:	$(SERVER_NAME) $(GUI_NAME) $(AI_NAME)

$(SERVER_NAME):	$(SERVER_OBJ)
	$(CC) -o $(SERVER_NAME) $(SERVER_OBJ) -lm

$(GUI_NAME):	$(GUI_OBJ)
	$(CXX) -o $(GUI_NAME) $(GUI_OBJ) $(GUI_LDLIBS)

$(AI_NAME):	$(AI_SRC)
	cp $(AI_MAIN) $(AI_NAME)
	chmod +x $(AI_NAME)

$(TESTS_NAME):	$(TESTS_PARSER_SRC) $(TESTS_SRC)
	$(CC) $(TESTS_CFLAGS) -o $(TESTS_NAME) $(TESTS_PARSER_SRC) $(TESTS_SRC) $(TESTS_LDLIBS)

$(TESTS_GUI_NAME):	$(TESTS_GUI_PARSER_SRC) $(TESTS_GUI_SRC)
	$(CXX) $(TESTS_GUI_CXXFLAGS) -o $(TESTS_GUI_NAME) $(TESTS_GUI_PARSER_SRC) $(TESTS_GUI_SRC) $(TESTS_GUI_LDLIBS)

tests_run:	$(TESTS_NAME) $(TESTS_GUI_NAME)
	./$(TESTS_NAME)
	./$(TESTS_GUI_NAME)

clean:
	rm -f $(SERVER_OBJ) $(GUI_OBJ)

fclean:	clean
	rm -f $(SERVER_NAME) $(GUI_NAME) $(AI_NAME) $(TESTS_NAME) $(TESTS_GUI_NAME)

re:	fclean all

.PHONY:	all clean fclean re tests_run zappy_server zappy_gui zappy_ai
