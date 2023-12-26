#include "stdlib.h"
#include "stdio.h"
#include "string.h"

unsigned int reverseBytes(unsigned int n) {
    return ((n >> 24) & 0x000000ff) |
           ((n >> 8) & 0x0000ff00) |
           ((n << 8) & 0x00ff0000) |
           ((n << 24) & 0xff000000);
}

typedef union TagHeader {
    char buffer[12];                  // 12 total

    struct {
        unsigned short empty;        // 2
        unsigned char version[3];   // 3
        unsigned char v1;           // 1
        unsigned char v2;           // 1
        unsigned char flags;        // 1
        unsigned int size;         // 4
    } data;
} header;

typedef union FrameHeader {
    char buffer[10];                  // 10 total

    struct {
        unsigned char name[4];      // 4
        unsigned int size;         // 4
        unsigned short flags;        // 2
    } data;
} frame;

void copyFile(FILE *inp, FILE *outp) {
    int c;
    while ((c = getc(inp)) != EOF)
        putc(c, outp);
}

void show(char *fileName) {
    FILE *file;
    file = fopen(fileName, "rb");
    if (file == NULL) {
        printf("Read file error occurred!\n");
        return;
    }

    fseek(file, 0, SEEK_SET);

    header tagHeader;
    fread(tagHeader.buffer + 2, sizeof(char), 10, file);

    unsigned int tagSize = reverseBytes(tagHeader.data.size);

    printf("%sv%d.%d\n", tagHeader.data.version,
           tagHeader.data.v1,
           tagHeader.data.v2);
    while (ftell(file) < tagSize) {
        frame frameHeader;
        fread(frameHeader.buffer, sizeof(char), 10, file);
        if (frameHeader.data.name[0] == 0)
            break;
        printf("%s: ", frameHeader.data.name);

        unsigned int frameSize = reverseBytes(frameHeader.data.size);

        unsigned char *frameContent;
        frameContent = malloc(sizeof(char) * frameSize);
        fread(frameContent, sizeof(char), frameSize, file);
        unsigned int i;
        for (i = 0; i < frameSize; ++i) {
            printf("%c", frameContent[i]);
        }
        printf("\n");
        free(frameContent);
    }
    fclose(file);
}

void get(char *fileName, char frameName[4]) {
    FILE *file;
    file = fopen(fileName, "rb");

    header tagHeader;
    fread(tagHeader.buffer + 2, sizeof(char), 10, file);

    unsigned int tagSize = reverseBytes(tagHeader.data.size);

    while (ftell(file) < tagSize) {
        frame frameHeader;
        fread(frameHeader.buffer, sizeof(char), 10, file);

        unsigned int frameSize = reverseBytes(frameHeader.data.size);

        if (strcmp(frameHeader.data.name, frameName) == 0) {
            printf("%s: ", frameHeader.data.name);

            unsigned char *frameContent;
            frameContent = malloc(sizeof(char) * frameSize);
            fread(frameContent, sizeof(char), frameSize, file);
            unsigned int i;
            for (i = 0; i < frameSize; ++i) {
                printf("%c", frameContent[i]);
            }
            printf("\n");
            free(frameContent);
            fclose(file);
            return;
        }

        fseek(file, frameSize, SEEK_CUR);
    }
    fclose(file);
    printf("No value found for %s!", frameName);
}

void set(char *fileName, char frameName[4], char *frameValue) {
    FILE *file;
    file = fopen(fileName, "rb");

    header tagHeader;
    fread(tagHeader.buffer + 2, sizeof(char), 10, file);

    unsigned int tagSize = reverseBytes(tagHeader.data.size);

    unsigned int oldFramePos = 0;
    unsigned int oldFrameSize = 0;

    while (ftell(file) < tagSize) {
        frame frameHeader;
        fread(frameHeader.buffer, sizeof(char), 10, file);

        unsigned int frameSize = reverseBytes(frameHeader.data.size);

        if (strcmp(frameHeader.data.name, frameName) == 0) {
            oldFramePos = ftell(file) - 10;
            oldFrameSize = frameSize;
            break;
        }
        fseek(file, frameSize, SEEK_CUR);
    }

    unsigned int valueSize = strlen(frameValue);

    unsigned int newTagSize = tagSize - oldFrameSize + valueSize + 10 * (oldFramePos != 0);

    if (oldFramePos == 0) {
        oldFramePos = ftell(file);
    }
    if (valueSize == 0) {
        newTagSize -= 10;
    }

    FILE *fileCopy;
    fileCopy = fopen("temp.mp3", "wb");

    fseek(file, 0, SEEK_SET);
    fseek(fileCopy, 0, SEEK_SET);
    copyFile(file, fileCopy);

    fclose(file);
    fclose(fileCopy);

    fileCopy = fopen("temp.mp3", "rb");
    file = fopen(fileName, "wb");

    tagHeader.data.size = reverseBytes(newTagSize);

    fwrite(tagHeader.buffer + 2, sizeof(char), 10, file);

    fseek(fileCopy, 10, SEEK_SET);
    unsigned int i;
    for (i = 0; i < oldFramePos - 10; ++i) {
        int c;
        c = getc(fileCopy);
        putc(c, file);
    }

    if (valueSize > 0) {
        frame frameHeader;
        unsigned int i;
        for (i = 0; i < 4; ++i) {
            frameHeader.data.name[i] = frameName[i];
        }
        frameHeader.data.size = reverseBytes(valueSize);
        frameHeader.data.flags = 0;
        fwrite(frameHeader.buffer, sizeof(char), 10, file);
    }

    fwrite(frameValue, sizeof(char), valueSize, file);

    fseek(fileCopy, oldFramePos + 10 + oldFrameSize, SEEK_SET);
    for (i = ftell(file); i < newTagSize; ++i) {
        unsigned short int c;
        c = getc(fileCopy);
        putc(c, file);
    }

    copyFile(fileCopy, file);

    fclose(file);
    fclose(fileCopy);
    remove("temp.mp3");
}

int main() {
    char command[200] = " ";
    int flag = 1;
    char *file_path;
    char *value;
    char *frame_name;

    while (flag) {
        scanf("%s", command);
        if (strcmp(command, "stop") == 0) {
            return 0;
        }else if (strcmp(command, "--show") == 0) {
            show(file_path);
            return 1;
        } else if (strncmp("--get", command, 5) == 0) {
            char *after_eq = strchr(command, '=') + 1;
            frame_name = (char *) malloc(sizeof(char) * strlen(after_eq));
            strcpy(frame_name, after_eq);
            get(file_path, frame_name);
            free(frame_name);
            return 1;

        } else if (strncmp("--set", command, 5) == 0) {
            printf("Read command set\n");

            char *after_eq = strchr(command, '=') + 1;
            frame_name = (char *) malloc(sizeof(char) * strlen(after_eq));
            strcpy(frame_name, after_eq);
            scanf("%s", command);

            if (strncmp("--value", command, 7) == 0) {
                printf("Read command value\n");
                char *after_eq = strchr(command, '=') + 1;
                value = (char *) malloc(sizeof(char) * strlen(after_eq));
                strcpy(value, after_eq);
                set(file_path, frame_name, value);
                free(value);
                return 1;
            } else {
                printf("Error\n");
                return 0;
            }

        } else if (strncmp("--filepath", command, 10) == 0) {

            char *after_eq = strchr(command, '=') + 1;

            file_path = (char *) malloc(sizeof(char) * strlen(after_eq));
            strcpy(file_path, after_eq);
        }
    }
    free(file_path);
}//app.exe --filepath=files\file2.mp3 --show
//app.exe --filepath=files\file2.mp3 --get=TIT2
//app.exe --filepath=files\file2.mp3 --set=COMM --value=Test
