#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef union tagheader {

    char buffer[12];
    struct {
        unsigned short empty;
        unsigned char version[3];
        unsigned char flag1;
        unsigned char flag2;
        unsigned char flags;
        unsigned int size;
    } data;

} Tag;

typedef union frameheader {

    char buffer[10];

    struct {

        unsigned char name[4];
        unsigned int size;
        unsigned short flags;

    } data;

} Frame;

unsigned int countsize(unsigned int n) {

    return ((n >> 24) & 0x000000ff)
           | ((n >> 8) & 0x0000ff00)
           | ((n << 8) & 0x00ff0000)
           | ((n << 24) & 0xff000000);

}

void get(char *file_name, char frame_name[4]) {

    FILE *file;
    file = fopen(file_name, "rb");
    Tag tag;

    fread(tag.buffer + 2, sizeof(char), 10, file);
    unsigned int tag_size = countsize(tag.data.size);

    while (ftell(file) < tag_size + 10) {

        Frame frame;
        fread(frame.buffer, sizeof(char), 10, file);
        unsigned int frame_size = countsize(frame.data.size);

        if (strcmp(frame.data.name, frame_name) == 0) {

            printf("%s: ", frame.data.name);
            unsigned char *frame_content;
            frame_content = malloc(sizeof(char) * frame_size);
            fread(frame_content, sizeof(char), frame_size, file);

            for (unsigned int i = 0; i < frame_size; ++i)
                printf("%c", frame_content[i]);

            printf("\n");
            free(frame_content);
            fclose(file);

            return;

        }

        fseek(file, frame_size, SEEK_CUR);

    }

    fclose(file);
    printf("There is no frame: %s!", frame_name);

}

void show(char *file_name) {

    FILE *file;
    file = fopen(file_name, "rb");
    fseek(file, 0, SEEK_SET);
    Tag tag;
    fread(tag.buffer + 2, sizeof(char), 10, file);
    unsigned int tag_size = countsize(tag.data.size);

    printf("%sv%d.%d\n", tag.data.version, tag.data.flag1, tag.data.flag2);

    while (ftell(file) < tag_size + 10) {

        Frame frame;
        fread(frame.buffer, sizeof(char), 10, file);

        if (frame.data.name[0] == 0)
            continue;

        printf("%s: ", frame.data.name);
        unsigned int frame_size = countsize(frame.data.size);
        unsigned char *frame_content;
        frame_content = malloc(sizeof(char) * frame_size);
        fread(frame_content, sizeof(char), frame_size, file);

        for (unsigned int i = 0; i < frame_size; ++i)
            printf("%c", frame_content[i]);

        printf("\n");
        free(frame_content);

        if ((frame.data.name[0] == 'T') && ((frame.data.name[1] == 'L')))
            break;

    }

    fclose(file);

}

void copyFile(FILE *inp, FILE *outp) {

    int c;

    while ((c = getc(inp)) != EOF)
        putc(c, outp);

}

void set(char *file_name, char frame_name[4], char *frame_value) {

    FILE *file;
    file = fopen(file_name, "rb");

    Tag tag;
    fread(tag.buffer + 2, sizeof(char), 10, file);

    unsigned int tag_size = countsize(tag.data.size), old_position = 0,
            old_size = 0;

    while (ftell(file) < tag_size + 10) {

        Frame frame;
        fread(frame.buffer, sizeof(char), 10, file);
        unsigned int frame_size = countsize(frame.data.size);

        if (strcmp(frame.data.name, frame_name) == 0) {

            old_position = ftell(file) - 10;
            old_size = frame_size;
            break;

        }

        fseek(file, frame_size, SEEK_CUR);

    }

    unsigned int size_value = strlen(frame_value);
    unsigned int tag_new_size = tag_size - old_size + size_value + 10 * (old_position != 0);

    if (old_position == 0)
        old_position = ftell(file);

    if (size_value == 0)
        tag_new_size -= 10;

    FILE *fileCopy;
    fileCopy = fopen("temp.mp3", "wb");
    fseek(file, 0, SEEK_SET);
    fseek(fileCopy, 0, SEEK_SET);
    copyFile(file, fileCopy);
    fclose(file);
    fclose(fileCopy);
    fileCopy = fopen("temp.mp3", "rb");
    file = fopen(file_name, "wb");
    tag.data.size = countsize(tag_new_size);
    fwrite(tag.buffer + 2, sizeof(char), 10, file);
    fseek(fileCopy, 10, SEEK_SET);

    for (unsigned int i = 0; i < old_position - 10; ++i) {

        int c;
        c = getc(fileCopy);
        putc(c, file);

    }

    if (size_value > 0) {

        Frame frame;

        for (unsigned int i = 0; i < 4; ++i)
            frame.data.name[i] = frame_name[i];

        frame.data.size = countsize(size_value);
        frame.data.flags = 0;

        fwrite(frame.buffer, sizeof(char), 10, file);

    }

    fwrite(frame_value, sizeof(char), size_value, file);
    fseek(fileCopy, old_position + 10 + old_size, SEEK_SET);

    for (unsigned int i = ftell(file); i < tag_new_size; ++i) {

        unsigned short int c;
        c = getc(fileCopy);
        putc(c, file);

    }

    printf("New value has been set");

    copyFile(fileCopy, file);
    fclose(file);
    fclose(fileCopy);

    remove("temp.mp3");

}

int main(int argc, char *argv[]) {

    char *file_name, *frame_name, *value;
    int f1 = 0, f3 = 0, f2 = 0;

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "--show") == 0)
            f1 = 1;

        if (argv[i][2] == 'f')
            file_name = strpbrk(argv[i], "=") + 1;

        if (argv[i][2] == 'g') {

            f2 = 1;
            frame_name = strpbrk(argv[i], "=") + 1;

        }

        if (argv[i][2] == 's') {

            f3 = 1;
            frame_name = strpbrk(argv[i], "=") + 1;

        }

        if (argv[i][2] == 'v')
            value = strpbrk(argv[i], "=") + 1;

    }

    if (f1)
        show(file_name);

    if (f2)
        get(file_name, frame_name);

    if (f3)
        set(file_name, frame_name, value);

    return 0;

}