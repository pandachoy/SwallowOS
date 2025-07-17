main:
	i686-linux-gnu-as src/boot.s -o boot.o
	i686-linux-gnu-gcc -c src/kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
	i686-linux-gnu-gcc -T src/linker.ld -o SwallowOS.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc

clean:
	rm *.o