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

SERVER_SRC	=	$(wildcard $(SERVER_SRC_DIR)/*.c)
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
# Décommenter une fois SFML utilisé dans le code :
# GUI_LDLIBS	=	-lsfml-graphics -lsfml-window -lsfml-system

# ---------------------------------------------------------------------------
# AI (Python)
# ---------------------------------------------------------------------------
AI_SRC_DIR	=	src/ai
AI_MAIN		=	$(AI_SRC_DIR)/main.py

# ---------------------------------------------------------------------------
# Rules
# ---------------------------------------------------------------------------
all:	$(SERVER_NAME) $(GUI_NAME) $(AI_NAME)

$(SERVER_NAME):	$(SERVER_OBJ)
	$(CC) -o $(SERVER_NAME) $(SERVER_OBJ)

$(GUI_NAME):	$(GUI_OBJ)
	$(CXX) -o $(GUI_NAME) $(GUI_OBJ) $(GUI_LDLIBS)

$(AI_NAME):	$(AI_MAIN)
	cp $(AI_MAIN) $(AI_NAME)
	chmod +x $(AI_NAME)

clean:
	rm -f $(SERVER_OBJ) $(GUI_OBJ)

fclean:	clean
	rm -f $(SERVER_NAME) $(GUI_NAME) $(AI_NAME)

re:	fclean all

.PHONY:	all clean fclean re zappy_server zappy_gui zappy_ai
