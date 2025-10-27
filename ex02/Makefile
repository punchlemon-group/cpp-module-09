NAME = PmergeMe

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic $(DEFS)

DEFS =
SRCS = main.cpp PmergeMe.cpp Utils.cpp CounterUint.cpp
OBJDIR = obj
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))

.PHONY: all clean fclean re

.PHONY: test

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

test:
	make re DEFS="-DDEFINE_TEST"
# 	@chmod +x test.sh
# 	@./test.sh
