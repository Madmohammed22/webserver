SRC_DIR1 = helper

SRC_DIR2 = methods

SRC = main.cpp multiplexer.cpp parser.cpp server.cpp $(SRC_DIR2)/get.cpp $(SRC_DIR2)/post.cpp $(SRC_DIR2)/delete.cpp \
	$(SRC_DIR1)/httpResponsHeaders.cpp $(SRC_DIR1)/parseRequest.cpp $(SRC_DIR1)/Binary_String.cpp \
	$(SRC_DIR1)/establishingServer.cpp\
	$(SRC_DIR1)/utils.cpp\
	$(SRC_DIR1)/processMethodNotAllowed.cpp\
	request.cpp ConfigData.cpp ConfigParsing.cpp multipart.cpp Cgi.cpp $(SRC_DIR1)/generate_error_page.cpp\

OBJ = $(SRC:.cpp=.o)

NAME = webserver

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3 #-fsanitize=address  

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f ${OBJ}

fclean: clean
	rm -f ${NAME}
re: fclean all

.PHONY: all clean fclean re
