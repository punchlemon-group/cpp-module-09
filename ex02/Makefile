NAME = PmergeMe

CC = c++
# Strict C++98, pedantic to enforce standard rules
CFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic

SRCS = main.cpp PmergeMe.cpp Utils.cpp
OBJDIR = obj
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

# compile .cpp -> obj/*.o, create obj dir if needed
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all
