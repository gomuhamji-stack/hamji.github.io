CC = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LDFLAGS = -lm

SRCS = main.c i1d3_api.c display_calibration_api.c
OBJS = $(SRCS:.c=.o)
TARGET = display_cal_with_i1d3

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
