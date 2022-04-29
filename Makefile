out:
	g++ -c ./src/main.cpp ./src/FileOperations.cpp ./src/Tree.cpp ./src/Tokenizer.cpp ./src/Parser.cpp ./src/GenerateAsmCode.cpp
	g++ main.o FileOperations.o Tree.o Tokenizer.o Parser.o GenerateAsmCode.o -o generateAsmCode
	./generateAsmCode
