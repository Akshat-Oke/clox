CC   = gcc
CFLAGS = -Wall
LDFLAGS = 
OBJFILES = table.o object.o scanner.o compiler.o vm.o value.o debug.o memory.o chunk.o common.o main.o
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
#Build and run
go:
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
	./${TARGET} z_test.lox

test:
	./${TARGET} z_test.lox