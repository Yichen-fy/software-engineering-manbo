CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
TARGET = display
SRC = display.c game_init.c main.c map.c ustil.c buy_house.c
OBJ = $(SRC:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)