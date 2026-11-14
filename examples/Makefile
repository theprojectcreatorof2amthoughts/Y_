GYY = gyy
CFLAGS = -Wall -Wextra -O2

all: test

clean: 
	rm -f test

test: adequit_demo.yc quicktools.yaq
	$(GYY) adequit_demo.yc quicktools.yaq $(CFLAGS) -o test