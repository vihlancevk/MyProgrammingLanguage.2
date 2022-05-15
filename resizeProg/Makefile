out:
	nasm -f elf64 -l lst.txt -o prog.o prog.asm
	gcc -no-pie -o ../res/prog prog.o -lm
