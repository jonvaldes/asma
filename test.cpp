#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
typedef int32_t i32;
typedef int64_t i64;
typedef unsigned char u8;

typedef i64 (ExecFunc)(i64 dummy, void * stackAddress);

#define STACK_SIZE (1024 * 1024)

int main(int argc, const char **argv) {

	// TODO -- It should be possible to directly map the file for execution instead of doing this 
	// read + copy + map dance, but it's been giving me trouble so far

    // Read file
    FILE *fd = fopen(argv[1], "r");
    fseek(fd, 0, SEEK_END);
    int fileSize = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    u8 *fileData = (u8 *)calloc(fileSize + 1, 1);
    fread(fileData, 1, fileSize, fd);
    fclose(fd);

	// Create a file mapping
    HANDLE mapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 0, fileSize, NULL);
    if(mapping == NULL) {
        printf("%d\n", GetLastError());
        return 1;
    }

    LPVOID data = MapViewOfFileEx(mapping, FILE_MAP_WRITE | FILE_MAP_EXECUTE, 0, 0, fileSize, (void *)0xFF000000LL);
    if(data == NULL) {
        printf("%d\n", GetLastError());
        return 1;
    }

    // Copy data to addr
    memcpy(data, fileData, fileSize);

	// Allocate stack memory
	void * stack = malloc(STACK_SIZE);

	// Execute code
	// We need to pass the address of the stack in rdx, which is the second 
	// parameter to the function in the x64 calling convention
    ExecFunc *func = (ExecFunc*)data;
    i64 result = func(0, stack);
    printf("Result: %I64d\n", result);
}
