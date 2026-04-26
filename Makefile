SHELLOBJS	= shell.o ext2.o disksim.o ext2_shell.o entrylist.o

# 모던 clang(C99 묵시적 선언 금지, char/unsigned char 엄격) 환경에서도 빌드되도록 일부 경고를 에러에서 풀어줌.
# 또한 사용되지 않는 변수와 MSVC pragma는 원본 코드의 의도된 노이즈라 무시.
CFLAGS += -Wall \
	-Wno-error=implicit-function-declaration \
	-Wno-pointer-sign \
	-Wno-incompatible-pointer-types \
	-Wno-unused-variable \
	-Wno-unknown-pragmas

all: $(SHELLOBJS)
	$(CC) -o shell $(SHELLOBJS) $(CFLAGS)

clean:
	rm -f *.o shell
