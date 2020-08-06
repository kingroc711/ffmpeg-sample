CFLAGS=-c -Wall

all: package_pcm

package_pcm: package_pcm.c
	gcc package_pcm.c  -o  package_pcm ${CFLAGS} -lavcodec  -lavutil 


clean:
	rm package_pcm  audiofile.wav -rf
