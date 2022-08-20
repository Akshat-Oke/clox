CC   = gcc
CFLAGS = -Wall
LDFLAGS = 
OBJFILES = table.o object.o scanner.o compiler.o vm.o value.o debug.o memory.o chunk.o main.o
TARGET = clox

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
clear:
	-rm -f *.o
run:
	./$(TARGET)
test:
	./${TARGET} z_test.lox