.PHONY = clean

CC := gcc
CFLAGS := -Wall -Wextra -pedantic -O2

all: v4l2_video_capture

v4l2_video_capture: v4l2_video_capture.o
	$(CC) $(CFLAGS) -o $@ $^

v4l2_video_capture.o: Makefile v4l2_video_capture.c
	$(CC) $(CFLAGS) -c v4l2_video_capture.c

clean:
	@rm -f v4l2_video_capture v4l2_video_capture.o > /dev/null 2>&1

