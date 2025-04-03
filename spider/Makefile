NAME    = spider
SRCS    = main.c spider.c tools.c
HEADERS = spider.h
OBJ_DIR = obj
OBJS    = $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -lssl -lcrypto -lpthread -lxml2 -I/usr/include/libxml2

##############################################

ORANGE  = \033[0;33m
GREEN   = \033[1;32m
RED     = \033[0;31m
RESET   = \033[0m

##############################################

LIB_INSTALLED_FLAG = .lib_installed

all: lib $(NAME)

lib:
	@if [ ! -f $(LIB_INSTALLED_FLAG) ]; then \
		echo "🗃 Installing openssl and libxml...$(ORANGE)"; \
		sudo apt-get install -y libssl-dev libxml2-dev; \
		touch $(LIB_INSTALLED_FLAG); \
		echo "$(RESET)✅ openssl and libxml installed$(ORANGE)"; \
	else \
		echo "✅ openssl and libxml already installed$(ORANGE)"; \
	fi

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "$(RESET)✅ Compiled$(ORANGE)"

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $(NAME) $(CFLAGS)
	@echo "$(RESET)✅ Linked$(ORANGE)"
	@echo "$(RED)"
	@cat spider.txt
	@echo "$(GREEN)\n---> Ready to launch! <---$(RESET)\nUsage: ./$(NAME) [-rlp] [N] [PATH] <URL> \n$(ORANGE)"

clean:
	@echo "🗑 Cleaning$(ORANGE)"
	rm -rf $(OBJ_DIR)

fclean: clean
	@rm -rf $(LIB_INSTALLED_FLAG)
	rm -rf */
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re lib

