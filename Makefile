CC = gcc
AS = as
CFLAGS = -ffreestanding -O2 -Wall -Wextra -fmax-errors=0
LDFLAGS = -T linker.ld -nostdlib

# List all your project object files
OBJS = boot/boot.o kernel/main.o kernel/cbasis.o kernel/security.o

# Target binary
os.bin: $(OBJS)
	$(CC) $(LDFLAGS) -o os.bin $(OBJS)

# Compile assembly boot files
%.o: %.s
	$(AS) $< -o $@

# Compile C kernel and library files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f boot/*.o kernel/*.o libs/*.o os.bin
