#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int32_t i32;
typedef int64_t i64;
typedef unsigned char u8;

#include "generated/encoding.hxx"

const char *nextWord(const char *word, const char *end) {
    bool seenNull = false;
    while(word < end) {
        if(*word == 0) {
            seenNull = true;
        } else {
            if(seenNull) {
                return word;
            }
        }
        word++;
    }
    return nullptr;
}

#define MAX_FUNCS 256
#define MAX_FUNCCALLS (1024 * 1024)
int main(int argc, const char **argv) {
    if(argc != 3) {
        printf("Usage: compiler <source file> <output file>\n");
        return 1;
    }

    // Read file
    FILE *fd = fopen(argv[1], "r");
    fseek(fd, 0, SEEK_END);
    int fileSize = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    u8 *data = (u8 *)calloc(fileSize + 1, 1);
    fread(data, 1, fileSize, fd);
    fclose(fd);

    // Output file
    fd = fopen(argv[2], "w");

    // Replace spaces with null terminators
    u8 *s = data;
    for(i32 i = 0; i < fileSize; i++) {
        u8 c = *s;
        if(c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            *s = 0;
        }
        ++s;
    }

    struct FuncInfo {
        const char *name;
        i32 address;
    };
    FuncInfo *functions = (FuncInfo *)malloc(sizeof(FuncInfo) * MAX_FUNCS);
    FuncInfo *functionCalls = (FuncInfo *)malloc(sizeof(FuncInfo) * MAX_FUNCCALLS);
    i32 functionCallCnt = 0;
    i32 functionCnt = 0;
    i32 writtenBytes = 0;

    // Parse words
    const char *end = (const char *)&data[fileSize];
    const char *w = (const char *)data;

    while(w) {
        // Check comments
        if(w[0] == '(' && w[1] == 0) {
            // This is a comment, skip words until comment end
            do {
                w = nextWord(w, end);
                if(!w) {
                    printf("Error, expected literal but found EOF\n");
                    return 1;
                }
            } while(strcmp(w, ")") != 0);
            goto wordRead;
        }

        // Look for word in list of instructions
        for(int i = 0; i < Instr_CNT; i++) {
            const Instruction *instr = &g_instructions[i];
            if(strcmp(w, instr->name) == 0) {
                int bytesToWrite = instr->size - instr->literalBytes;
                fwrite(instr->data, 1, bytesToWrite, fd);
                writtenBytes += bytesToWrite;

                if(instr->literalBytes > 0) {
                    // If instr has literal, read next word and interpret as literal number
                    w = nextWord(w, end);
                    if(!w) {
                        printf("Error, expected literal but found EOF\n");
                        return 1;
                    }

                    // Parse word as number, write literal to file with specified number of bytes
                    if(instr->literalBytes == 8) {
                        i64 literal = strtoll(w, nullptr, 10);
                        fwrite(&literal, 1, 8, fd);
                        writtenBytes += 8;
                    } else if(instr->literalBytes == 4) {
                        i32 literal = strtol(w, nullptr, 10);
                        fwrite(&literal, 1, 8, fd);
                        writtenBytes += 4;
                    } else {
                        printf("Wrong number of literal bytes!!\n");
                        return 1;
                    }
                }
                goto wordRead;
            }
        }

        // Check for function definition
        if(strcmp(w, "func") == 0) {
            // Next word should be the function name
            w = nextWord(w, end);
            if(!w) {
                printf("Error, expected function name, but found EOF\n");
                return 1;
            }
            // printf("Adding function definition %s\n", w);
            functions[functionCnt].name = w;
            functions[functionCnt].address = writtenBytes;
            functionCnt++;
            goto wordRead;
        }

        // Assume it's a function, write function call
        const Instruction *callInstr = &g_instructions[Instr_call];
        fwrite(callInstr->data, 1, callInstr->size, fd);
        writtenBytes += callInstr->size;

        i32 callAddress = writtenBytes - callInstr->literalBytes;
        functionCalls[functionCallCnt].name = w;
        functionCalls[functionCallCnt].address = callAddress;
        functionCallCnt++;
        goto wordRead;

    wordRead:
        w = nextWord(w, end);
    }

    // Now fix all function calls
    for(int i = 0; i < functionCallCnt; i++) {
        const char *funcName = functionCalls[i].name;
        i32 patchAddress = functionCalls[i].address;
        bool found = false;

        for(int j = 0; j < functionCnt; j++) {
            if(strcmp(funcName, functions[j].name) == 0) {
                fseek(fd, patchAddress, SEEK_SET);
                // Calculate relative address
                i32 relAddr = functions[j].address - patchAddress;
                // printf("Relative address: %d patch address: %d\n", relAddr, patchAddress);
                fwrite(&relAddr, 4, 1, fd);
                found = true;
                break;
            }
        }
        if(!found) {
            printf("Error, function not found: %s\n", funcName);
            return 1;
        }
    }

    fclose(fd);
}
