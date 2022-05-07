out:
	g++ -c src/main.cpp            -o src/main.o
	g++ -c src/FileOperations.cpp  -o src/FileOperations.o
	g++ -c src/Tree.cpp            -o src/Tree.o
	g++ -c src/Tokenizer.cpp       -o src/Tokenizer.o
	g++ -c src/Parser.cpp          -o src/Parser.o
	g++ -c src/GenerateAsmCode.cpp -o src/GenerateAsmCode.o
	g++ src/main.o src/FileOperations.o src/Tree.o src/Tokenizer.o src/Parser.o src/GenerateAsmCode.o -o src/generateAsmCode
	src/generateAsmCode
	time -p res/prog
