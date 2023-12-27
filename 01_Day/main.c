#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>   // int64_t
#include <time.h>       // timespec_get
#include <unistd.h>     // usleep
#include <errno.h>      // errno
#include <ctype.h>      // isdigit
#include <stdbool.h>    // bool

#define DEBUG (1)

static int64_t millis();
static inline int64_t print_program_start(void);
static inline void print_program_end(int64_t start_time);
static ssize_t decrypt_calibration_value(char* input_file_name);
static uint8_t isWrittenDigit(char* word, uint8_t word_len);

char* G_PROGRAM_NAME;

// ################################################

int main (int argc, char* argv[]) {
    
    G_PROGRAM_NAME = argv[0];
    int64_t start_time = print_program_start();
    // ------------------------------------------------

    char* input_file_name = "input_big_letters.txt";
    ssize_t result = decrypt_calibration_value(input_file_name);
    printf("\n\nResult: %ld\n", result);

    // ------------------------------------------------
    print_program_end(start_time);
    return EXIT_SUCCESS;
}

// ################################################

static ssize_t decrypt_calibration_value(char* input_file_name) {

    // Open file in read-mode
    FILE* input_file = fopen(input_file_name, "r");
    if (input_file == NULL) {
        perror("Error opening file");
        return -1;
    }

    // Read file line by line
    ssize_t result = 0;
    char* line = NULL;
    size_t len = 0;
    ssize_t read_bytes = 0;
    ssize_t first_digit_in_line, last_digit_in_line;
    size_t line_counter = 1; 
    while ((read_bytes = getline(&line, &len, input_file))) {
        
        if(read_bytes == -1) {
            if(errno != 0) {
                perror("Error reading line");
                result = -1;
                goto cleanup;
            }
            // EOF
            goto cleanup;
        }

        // Get first digit in line
        first_digit_in_line = -1;
        for(size_t i=0; i<read_bytes; ++i) {
            if(isdigit(line[i])) {
                first_digit_in_line = strtoul(&line[i], NULL, 10);
                if(errno != 0) {
                    perror("Error converting string to number");
                    result = -1;
                    goto cleanup;
                }
                // strtoul returns the first number in line
                // 123test456 => 123
                // But we want only the first digit of line
                while(first_digit_in_line >= 10) {
                    first_digit_in_line /= 10;
                }
                break;
            }
            uint8_t word_length = (read_bytes-1-i) < 7 ? (read_bytes-1-i) : 7;
            uint8_t written_digit = isWrittenDigit(&line[i], word_length);
            if(written_digit) {
                first_digit_in_line = written_digit;
                break;
            }
        }
        if(first_digit_in_line == -1) {
            perror("No digit in line");
            result = -1;
            goto cleanup;
        }

        // Get last digit in line
        for(ssize_t i=read_bytes-1; i>=0; --i) {
            if(isdigit(line[i])) {
                last_digit_in_line = strtoul(&line[i], NULL, 10);
                if(errno != 0) {
                    perror("Error converting string to number");
                    result = -1;
                    goto cleanup;
                }
                break;
            }
            uint8_t word_length = (read_bytes-1-i < 7) ? (read_bytes-1-i) : 7;
            uint8_t written_digit = isWrittenDigit(&line[i], word_length);
            if(written_digit) {
                last_digit_in_line = written_digit;
                break;
            }
        }
        // There has to be a last-digit in line, if there was a first-digit
        // So it must not be checked here

        #ifdef DEBUG
        printf("%5.ld. (%ld, %ld): %s",
            line_counter,
            first_digit_in_line, 
            last_digit_in_line, 
            line);
        line_counter++;
        #endif

        // Add concatenation of first and last digit to result
        result += (10*first_digit_in_line + last_digit_in_line);
    }

    cleanup:
        free(line);
        fclose(input_file);
        return result;
}

static uint8_t isWrittenDigit(char* word, uint8_t word_len) {

    if(word_len < 3) {
        return 0;
    }

    if(word[0] == 'o' 
    && word[1] == 'n' 
    && word[2] == 'e') {
        return 1;
    }

    else if(word[0] == 't') {
         if(word[1] == 'w' 
         && word[2] == 'o') {
             return 2;
         }
         else if(word_len >= 5
         && word[1] == 'h' 
         && word[2] == 'r' 
         && word[3] == 'e' 
         && word[4] == 'e') {
             return 3;
         }
    }

    else if(word[0] == 'f') {
         if(word_len >= 4
         && word[1] == 'o' 
         && word[2] == 'u' 
         && word[3] == 'r') {
             return 4;
         }
         else if(word_len >= 4
         && word[1] == 'i' 
         && word[2] == 'v' 
         && word[3] == 'e') {
             return 5;
         }
    }

    else if(word[0] == 's') {
         if(word[1] == 'i' 
         && word[2] == 'x') {
             return 6;
         }
         else if(word_len >= 5
         && word[1] == 'e' 
         && word[2] == 'v' 
         && word[3] == 'e' 
         && word[4] == 'n') {
             return 7;
         }
    }

    else if(word[0] == 'e') {
         if(word_len >= 5
         && word[1] == 'i' 
         && word[2] == 'g' 
         && word[3] == 'h' 
         && word[4] == 't') {
             return 8;
         }
    }

    else if(word[0] == 'n') {
         if(word_len >= 4
         && word[1] == 'i' 
         && word[2] == 'n' 
         && word[3] == 'e') {
             return 9;
         }
    }
    
    return 0;
}

// ################################################

static int64_t millis() {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return ((int64_t) now.tv_sec) * 1000 + ((int64_t) now.tv_nsec) / 1000000;
}

static inline int64_t print_program_start(void) {
    int64_t current_time = millis();
    printf("\n");
    printf("Started\n");
    printf("---------------------------------\n");
    return current_time;
}

static inline void print_program_end(int64_t start_time) {
    int64_t end_time = millis();
    int64_t elapsed_time_ms = end_time - start_time;
    printf("---------------------------------\n");
    printf("Finished in %ld ms\n", elapsed_time_ms);
    printf("\n");
}