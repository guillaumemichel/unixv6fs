CFLAGS=-std=c99 -Wall -ftrapv -Wshadow -Wextra -Wno-unused
LDLIBS += -lcrypto -lm

all: tests shell fs

tests: test-bitmap test-dirent test-file test-inodes test-bitmap-mount test-create

test-inodes: test-core.o error.o test-inodes.o mount.o bmblock.o sector.o inode.o

test-file: test-core.o error.o mount.o sector.o bmblock.o inode.o filev6.o sha.o test-file.o

test-dirent: test-core.o error.o mount.o sector.o bmblock.o inode.o filev6.o direntv6.o test-dirent.o

test-bitmap: error.o bmblock.o mount.o inode.o sector.o test-bitmap.o

test-bitmap-mount: test-core.o mount.o inode.o sector.o error.o bmblock.o test-bitmap-mount.o

test-create: mount.o sector.o error.o bmblock.o test-create.o inode.o filev6.o

shell: error.o shell.o mount.o sector.o bmblock.o inode.o filev6.o direntv6.o sha.o

fs.o: fs.c
	$(COMPILE.c) -D_DEFAULT_SOURCE $$(pkg-config fuse --cflags) -o $@ -c $<

fs: fs.o error.o direntv6.o filev6.o mount.o bmblock.o inode.o sector.o
	$(LINK.c) -o $@ $^ $(LDLIBS) $$(pkg-config fuse --libs)

clean:
	rm -rf *.o
	rm -rf *.gch
