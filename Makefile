default: all

all:
	gcc -O3 -o km km.c -lm ; gcc -O3 -o km_para km_para.c -lm -lpthread
clean:
	rm km ; rm km_para
