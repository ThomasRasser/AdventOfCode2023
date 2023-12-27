#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>   // int64_t
#include <time.h>       // timespec_get
#include <unistd.h>     // usleep
#include <errno.h>      // errno
#include <ctype.h>      // isdigit
#include <stdbool.h>    // bool
#include <regex.h>      // regex

// Debugging
// ################################################

#define DEBUG (0)
#define DEBUG_LEVEL (1) // Everything below this level will be printed (incl. this level)
#ifdef DEBUG
    #define DEBUG_START(level) if (DEBUG_LEVEL >= (level)) {
    #define DEBUG_END }
#else
    #define DEBUG_START(level) if(0) {
    #define DEBUG_END }
#endif

// Definitions
// ################################################

// Structs, Typedefs, Enums and Global Variables
// ################################################

char* G_PROGRAM_NAME;

typedef struct {
    int16_t x;
    int16_t y;
} position_t;

typedef struct {
    int16_t value;
    uint8_t length;
    position_t pos;
} number_t;

// Function Prototypes
// ################################################

// Basic Utility Functions
static int64_t millis();
static inline int64_t print_program_start(void);
static inline void print_program_end(int64_t start_time);
static char* rawify(const char *str);
// AoC Functions
static ssize_t decrypt_riddle_value(const char* input_file_name);
static bool try_opening_file(const char* file_name, FILE** file);
static bool is_digit(const char *c);
static bool is_symbol(const char *c);
static bool has_adjacent_symbol(const number_t* number, 
                                const char** matrix, 
                                const uint8_t matrix_number_of_rows, 
                                const uint8_t matrix_number_of_cols);
static bool position_is_in_bounds(const position_t* pos, 
                                  const uint8_t max_x_pos, 
                                  const uint8_t max_y_pos);
                                  
// Main
// ################################################

int main (int argc, char* argv[]) {
    
    G_PROGRAM_NAME = argv[0];

    if(argc != 1) {
        printf("Usage: %s\n", rawify(argv[0]));
        return EXIT_FAILURE;
    }
    int64_t start_time = print_program_start();
    // ------------------------------------------------

    /*
    char* input_file_name = "input_small.txt";
    */
    char* input_file_name = "input_big.txt";
    ssize_t result = decrypt_riddle_value(input_file_name);
    printf("\n\nResult: %ld\n", result);

    // ------------------------------------------------
    print_program_end(start_time);
    return EXIT_SUCCESS;
}

// AoC Functions
// ################################################

static bool try_opening_file(const char* file_name, FILE** file) {

    *file = fopen(file_name, "r");
    if(*file == NULL) {
        perror("Error opening file");
        return false;
    }
    return true;
}

static bool is_digit(const char* c) {
    
    if(c == NULL) {
        return false;
    }

    if(*c == '\0') {
        return false;
    }

    if((*c >= '0') && (*c <= '9')) {
        return true;
    }

    return false;
}

static bool is_symbol(const char* c) {
    
    if(c == NULL) {
        return false;
    }

    if(*c == '\0') {
        return false;
    }

    // '.': empty cell
    if(!is_digit(c) && (*c != '.') && (*c != '\n') && (*c != ' ')) {
        return true;
    }

    return false;
}

static bool position_is_in_bounds(const position_t* pos, 
                                  const uint8_t max_x_pos, 
                                  const uint8_t max_y_pos) {

    if(pos == NULL) {
        return false;
    }

    if((pos->x < 0) || (pos->x > (max_x_pos-1))) {
        return false;
    }

    if((pos->y < 0) || (pos->y > (max_y_pos-1))) {
        return false;
    }

    return true;
}

static bool has_adjacent_symbol(const number_t* number, 
                                const char** matrix, 
                                const uint8_t matrix_number_of_rows, 
                                const uint8_t matrix_number_of_cols) {

    position_t pos_to_check = {0, 0};

    // Check above (including diagonals)
    pos_to_check.y = number->pos.y-1;
    if(pos_to_check.y > 0) {
        for(int8_t i=-1; i<=number->length; ++i) {
            pos_to_check.x = number->pos.x+i;
            if(position_is_in_bounds(&pos_to_check, matrix_number_of_cols, matrix_number_of_rows)) {
                if(is_symbol(&matrix[pos_to_check.y][pos_to_check.x])) {
                    return true;
                }
            }
        }
    }

    // Check below (including diagonals)
    pos_to_check.y = number->pos.y+1;
    if(pos_to_check.y < matrix_number_of_rows) {
        for(int8_t i=-1; i<=number->length; ++i) {
            pos_to_check.x = number->pos.x+i;
            if(position_is_in_bounds(&pos_to_check, matrix_number_of_cols, matrix_number_of_rows)) {
                if(is_symbol(&matrix[pos_to_check.y][pos_to_check.x])) {
                    return true;
                }
            }
        }
    }

    // Check left
    pos_to_check.x = number->pos.x-1;
    pos_to_check.y = number->pos.y;
    if(position_is_in_bounds(&pos_to_check, matrix_number_of_cols, matrix_number_of_rows)) {
        if(is_symbol(&matrix[pos_to_check.y][pos_to_check.x])) {
            return true;
        }
    }

    // Check right
    pos_to_check.x = number->pos.x+number->length;
    pos_to_check.y = number->pos.y;
    if(position_is_in_bounds(&pos_to_check, matrix_number_of_cols, matrix_number_of_rows)) {
        if(is_symbol(&matrix[pos_to_check.y][pos_to_check.x])) {
            return true;
        }
    }

    return false;
}

static ssize_t decrypt_riddle_value(const char *file_name) {

    // Cleanup struct with bitfield
    struct cleanup {
        bool file_opened: 1;
        bool matrix_allocated: 1;
        bool lines_read: 1;
        bool numbers_allocated: 1;
        bool valid_numbers_allocated: 1;
        bool invalid_numbers_allocated: 1;
        bool successful: 1;
    } cleanup = {false, false, false, true};

    // Open file
    FILE* file;
    if(!try_opening_file(file_name, &file)) {
        goto cleanup;
    }
    cleanup.file_opened = true;

    // Read file line by line and create 2D array of its values
    number_t* numbers = NULL;
    uint16_t numbers_cnt = 0;
    cleanup.numbers_allocated = true;

    char** matrix = NULL;
    uint8_t matrix_number_of_rows = 0;
    uint8_t matrix_number_of_cols = 0;
    uint8_t matrix_allocated_number_of_rows = 0;
    uint8_t allocation_growth = 32; // Number of additional rows to be allocated, if needed
    uint8_t allocated_blocks[32];
    uint8_t allocated_blocks_cnt = 0;
    cleanup.matrix_allocated = true;

    char* line = NULL;
    size_t line_size = 0;
    ssize_t read_bytes = 0;
    cleanup.lines_read = true;

    while( (read_bytes = getline(&line, &line_size, file)) != -1 ) {

        // Test if the input text is well formed
        if(matrix_number_of_cols != 0) {
            if(read_bytes/sizeof(char) != matrix_number_of_cols) {
                fprintf(stderr, "Error: Line %d has different length than previous lines\n", matrix_number_of_rows);
                goto cleanup;
            }
        } else {
            matrix_number_of_cols = read_bytes/sizeof(char);
        }

        // Copy line into matrix
        if(matrix_number_of_rows >= matrix_allocated_number_of_rows) {
            // Reallocate the array of pointers
            // temp matrix is used, so that if realloc fails, the original matrix is not lost and can be freed
            char **temp_matrix = (char**)realloc(matrix, (matrix_number_of_rows + allocation_growth) * sizeof(char*));
            if(temp_matrix == NULL) {
                fprintf(stderr, "Error reallocating memory for matrix pointers\n");
                goto cleanup;
            }
            matrix = temp_matrix;
            matrix_allocated_number_of_rows += allocation_growth;

            // Allocate new rows in a contiguous block
            char *new_rows = (char*)malloc(allocation_growth * matrix_number_of_cols * sizeof(char));
            if(new_rows == NULL) {
                fprintf(stderr, "Error allocating memory for new rows\n");
                goto cleanup;
            }
            allocated_blocks[allocated_blocks_cnt] = matrix_number_of_rows;
            allocated_blocks_cnt++;

            // Assign new row pointers to the appropriate locations in the new block
            for(int i = 0; i < allocation_growth; ++i) {
                matrix[matrix_number_of_rows+i] = new_rows + i*matrix_number_of_cols;
            }
        }
        memcpy(matrix[matrix_number_of_rows], line, read_bytes);
        matrix_number_of_rows++;

        // Put number into list
        bool number_allocated = false;
        for(uint8_t i=0; i<line_size; ++i) {
            if(is_digit(&line[i])) {
                uint8_t current_number = (uint8_t)line[i] - '0';
                if(!number_allocated) {
                    numbers = realloc(numbers, (numbers_cnt+1) * sizeof(number_t));
                    numbers[numbers_cnt].length = 1;
                    numbers[numbers_cnt].pos.x = i;
                    numbers[numbers_cnt].pos.y = matrix_number_of_rows-1; // -1 because we already incremented the row counter
                    numbers[numbers_cnt].value = current_number;

                    numbers_cnt++;
                    number_allocated = true;
                } else {
                    numbers[numbers_cnt-1].length++;
                    if(numbers[numbers_cnt-1].value > 0) {
                        numbers[numbers_cnt-1].value = numbers[numbers_cnt-1].value*10 + current_number;
                    } else {
                        numbers[numbers_cnt-1].value = numbers[numbers_cnt-1].value*10 - current_number;
                    }
                }
            } else {
                number_allocated = false;
            }
        }
    }
    if(ferror(file)) {
        fprintf(stderr, "Error reading file %s\n", file_name);
        goto cleanup;
    }

    DEBUG_START(1) // #region DEBUG: Print results of parsing
    fprintf(stdout, "Parsed numbers: %d\n", numbers_cnt);
    fprintf(stdout, "Matrix number of rows: %d\n", matrix_number_of_rows);
    fprintf(stdout, "Matrix number of cols: %d\n", matrix_number_of_cols);
    fprintf(stdout, "\n");
    DEBUG_END // #endregion

    DEBUG_START(2) // #region DEBUG: Print numbers and matrix

    // Print numbers list
    for(size_t i=0; i<numbers_cnt; i++) {
        fprintf(stdout, "%4ld. x: %3d, y: %3d, value: %3d, length: %d\n", 
            i+1, numbers[i].pos.x, numbers[i].pos.y, numbers[i].value, numbers[i].length);
    }
    fprintf(stdout, "\n");

    // Print matrix
    fprintf(stdout, "Matrix:\n");
    for(size_t i=0; i<matrix_number_of_rows-1; i++) {
        fprintf(stdout, "%s", matrix[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
    DEBUG_END // #endregion

    // Find numbers with adjacent symbols (normal or diagonal)
    ssize_t number_sum = 0;
    number_t* valid_numbers = NULL;
    uint16_t valid_numbers_cnt = 0;
    number_t* invalid_numbers = NULL;
    uint16_t invalid_numbers_cnt = 0;
    cleanup.valid_numbers_allocated = true;
    cleanup.invalid_numbers_allocated = true;
    for(size_t i=0; i<numbers_cnt; i++) {
        if(has_adjacent_symbol(&numbers[i], (const char**)matrix, matrix_number_of_rows, matrix_number_of_cols)) {
            DEBUG_START(1)
            valid_numbers = realloc(valid_numbers, (valid_numbers_cnt+1) * sizeof(number_t));
            valid_numbers[valid_numbers_cnt] = numbers[i];
            valid_numbers_cnt++;
            DEBUG_END
            number_sum += numbers[i].value;
        } else {
            DEBUG_START(1)
            invalid_numbers = realloc(invalid_numbers, (invalid_numbers_cnt+1) * sizeof(number_t));
            invalid_numbers[invalid_numbers_cnt] = numbers[i];
            invalid_numbers_cnt++;
            DEBUG_END
        }
    }

    DEBUG_START(1) // #region DEBUG: Print results of parsing
    fprintf(stdout, "Valid numbers: %d\n", valid_numbers_cnt);
    fprintf(stdout, "Invalid numbers: %d\n", invalid_numbers_cnt);
    fprintf(stdout, "Number sum: %ld\n", number_sum);
    fprintf(stdout, "\n");
    DEBUG_END // #endregion

    DEBUG_START(2) // #region DEBUG: Print valid and invalid numbers
    
    // Print valid numbers list
    fprintf(stdout, "Valid numbers:\n");
    for(size_t i=0; i<valid_numbers_cnt; i++) {
        fprintf(stdout, "%4ld. x: %3d, y: %3d, value: %3d, length: %d\n", 
            i+1, valid_numbers[i].pos.x, valid_numbers[i].pos.y, valid_numbers[i].value, valid_numbers[i].length);
    }
    fprintf(stdout, "\n");

    // Print invalid numbers list
    fprintf(stdout, "Invalid numbers:\n");
    for(size_t i=0; i<invalid_numbers_cnt; i++) {
        fprintf(stdout, "%4ld. x: %3d, y: %3d, value: %3d, length: %d\n", 
            i+1, invalid_numbers[i].pos.x, invalid_numbers[i].pos.y, invalid_numbers[i].value, invalid_numbers[i].length);
    }
    fprintf(stdout, "\n");
    DEBUG_END // #endregion

    cleanup:
    if(cleanup.file_opened) {
        if(fclose(file) != 0) {
            fprintf(stderr, "Error closing file %s\n", file_name);
            cleanup.successful = false;
        }
    }
    if(cleanup.matrix_allocated) {
        if(matrix != NULL) {
            for(int i = 0; i < allocated_blocks_cnt; ++i) {
                if(matrix[allocated_blocks[i]] != NULL) {
                    free(matrix[allocated_blocks[i]]);
                }
            }
            free(matrix);
        }
    }
    if(cleanup.lines_read) {
        if(line != NULL) {
            free(line);
        }
    }
    if(cleanup.numbers_allocated) {
        if(numbers != NULL) {
            free(numbers);
        }
    }
    if(cleanup.valid_numbers_allocated) {
        if(valid_numbers != NULL) {
            free(valid_numbers);
        }
    }
    if(cleanup.invalid_numbers_allocated) {
        if(invalid_numbers != NULL) {
            free(invalid_numbers);
        }
    }

    return number_sum;
}

// Utility Functions
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

static char* rawify(const char *str) {
    size_t length = strlen(str);
    char *raw_str = (char*) malloc((length * 2 + 1) * sizeof(char));
    char *temp = raw_str;

    while (*str) {
        if (*str == '\n') {
            *temp++ = '\\';
            *temp++ = 'n';
        } else if (*str == '\t') {
            *temp++ = '\\';
            *temp++ = 't';
        } else if (*str == '\r') {
            *temp++ = '\\';
            *temp++ = 'r';
        } else {
            *temp++ = *str;
        }
        str++;
    }
    *temp = '\0';
    return raw_str;
}
