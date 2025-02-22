#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main() {
    FILE *old_code;
    FILE *changes;
    FILE *translation;

    old_code = fopen("[OLD_CODEBASE_PATH]", "rb");
    changes = fopen("[DELTA_COMPRESSED_FILE_PATH]", "rb");
    translation = fopen("[WHERE TO STORE NEW_CODEBASE]", "wb");

    if(old_code == NULL || translation == NULL || changes == NULL) {
        printf("Unable to open necessary files!");
        exit(-1);
    }

    fseek(changes, 0L, SEEK_END);
    long changes_size = ftell(changes);
    fseek(changes, 0L, SEEK_SET);

    char change_byte;
    char old_byte;

    const int SKIP = 127;
    const int READ = 6;
    const int REPEAT = 26;

    long cursor_loc = 0;
    while (cursor_loc < changes_size) {
        fread(&change_byte, sizeof(char), 1, changes);
        cursor_loc++;
        
        int repeat_num = 1;
        if((int)change_byte == REPEAT) {
            fread(&change_byte, sizeof(char), 1, changes);
            cursor_loc++;

            repeat_num = 0;
            int digits_to_read = change_byte - '0';

            // Digits_to_read
            for(int i = digits_to_read - 1; i >= 0; i--) {
                fread(&change_byte, sizeof(char), 1, changes);
                cursor_loc++;
                repeat_num += (pow(10, i) * (change_byte - '0'));
            }

            // 
            fread(&change_byte, sizeof(char), 1, changes);
            cursor_loc++;
        }

        for(int repeat = 0; repeat < repeat_num; repeat++) {
            //if(change_byte == '-') {
            if((int)change_byte == SKIP) {
                fread(&old_byte, sizeof(char), 1, old_code);
            }
            //else if(change_byte == '_') {
            else if((int)change_byte == READ) {
                fread(&old_byte, sizeof(char), 1, old_code);
                fwrite(&old_byte, sizeof(char), 1, translation);
            }
            else {
                fwrite(&change_byte, sizeof(char), 1, translation);
            }
            
            printf("%c", change_byte);
        }
    }
 
    fclose(old_code);
    fclose(changes);
    fclose(translation);

    printf("\n Codebase Update Successful");

    return 0;
}

