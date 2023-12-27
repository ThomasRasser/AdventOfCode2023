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

// ################################################

// #define DEBUG (0)
#define DEBUG_LEVEL (1)
#ifdef DEBUG
    #define DEBUG_START(level) if (DEBUG_LEVEL >= (level)) {
    #define DEBUG_END }
#else
    #define DEBUG_START(level) if(0) {
    #define DEBUG_END }
#endif

// ################################################

#define RED (0)
#define GREEN (1)
#define BLUE (2)
#define COLOR_CNT (3)

#define RED_MAX_DICE (12)
#define GREEN_MAX_DICE (13)
#define BLUE_MAX_DICE (14)

#define PATTERN_LEN_RED (18)
#define PATTERN_LEN_GREEN (20)
#define PATTERN_LEN_BLUE (19)

// ################################################

typedef struct {
    char* round_string;
    size_t number_of_dice[COLOR_CNT];
} round_t;

typedef struct {
    size_t id;
    size_t round_cnt;
    round_t* rounds;
    size_t max_number_of_dice[COLOR_CNT];
} single_game_t;

typedef struct {
    size_t game_cnt;
    single_game_t* all_games;
    size_t max_dice[COLOR_CNT];
} games_t;

// ################################################

// Basic Utility Functions
static int64_t millis();
static inline int64_t print_program_start(void);
static inline void print_program_end(int64_t start_time);
static char* rawify(const char *str);
// AoC Functions
static ssize_t decrypt_riddle_value(const char* input_file_name);
static bool try_opening_file(const char *file_name, FILE** file);
static bool try_setting_up_regex(regex_t** regex);
static bool try_splitting_rounds(char* line, size_t* round_cnt, round_t** rounds);
static bool try_parsing_rounds(size_t* round_cnt, round_t* rounds, regex_t* regexes, size_t* max_number_of_dice);

char* G_PROGRAM_NAME;

// ################################################

int main (int argc, char* argv[]) {
    
    G_PROGRAM_NAME = argv[0];
    int64_t start_time = print_program_start();
    // ------------------------------------------------

    char* input_file_name = "input_big.txt";
    ssize_t result = decrypt_riddle_value(input_file_name);
    printf("\n\nResult: %ld\n", result);

    // ------------------------------------------------
    print_program_end(start_time);
    return EXIT_SUCCESS;
}

// ################################################

static bool try_opening_file(const char *file_name, FILE** file) {
    
    *file = fopen(file_name, "r");
    if (*file == NULL) {
        perror("Error opening file");
        return false;
    }

    DEBUG_START(1)
        fprintf(stderr, "Opened file \"%s\" in read-mode\n\n", file_name);
    DEBUG_END

    return true;
}

static bool try_setting_up_regex(regex_t** regex) {

    // Define the patterns
    const char* patterns[COLOR_CNT] = {
        "([0-9]{1,4}\\sred)",  
        "([0-9]{1,4}\\sgreen)",
        "([0-9]{1,4}\\sblue)"  
    };

    // Compile regex for each pattern
    for (size_t i = 0; i < COLOR_CNT; ++i) {
        if (regcomp(&((*regex)[i]), patterns[i], REG_EXTENDED) != 0) {
            perror("Error compiling regex");
            return false;
        }
    }

    DEBUG_START(1)
        fprintf(stderr, "Compiled regex\n\n");
    DEBUG_END

    return true;
}

static bool try_parsing_game_id(const ssize_t* read_bytes, const char* line, size_t* game_id) {

    if(*read_bytes < 7) {
        perror("Invalid line format");
        return false;
    }

    *game_id = strtoul(&line[5], NULL, 10);
    if(errno == ERANGE) {
        perror("Error parsing game id");
        return false;
    }

    DEBUG_START(2)
        fprintf(stderr, "Parsed game ID: %ld\n\n", *game_id);
    DEBUG_END

    return true;
}

static bool try_splitting_rounds(char* line, size_t* round_cnt, round_t** rounds) {

    char* round_string;
    const char delimiter[2] = ";";
    char* rest = line;

    round_string = strtok_r(line, delimiter, &rest);
    while (round_string != NULL) {

        // Reallocate memory for round struct pointer
        *rounds = realloc(*rounds, ((*round_cnt)+1) * sizeof(round_t));
        if (*rounds == NULL) {
            perror("Error reallocating memory for rounds");
            return false;
        }

        // Allocate memory for round string
        round_t* round = &((*rounds)[*round_cnt]);
        round->round_string = malloc(strlen(round_string) + 1);
        if (round->round_string == NULL) {
            perror("Error allocating memory for round string");
            return false;
        }

        strcpy(round->round_string, round_string);
        DEBUG_START(2)
            fprintf(stderr, "Parsed round: %s\n", rawify(round->round_string));
        DEBUG_END

        (*round_cnt)++;

        round_string = strtok_r(NULL, delimiter, &rest);
    }

    return true;
}

static bool try_parsing_rounds(size_t* round_cnt, round_t* rounds, regex_t* regexes, size_t* max_number_of_dice) {

    for(int round_index=0; round_index<*round_cnt; ++round_index) {
        round_t* round = &rounds[round_index];
        round->number_of_dice[RED] = 0;
        round->number_of_dice[GREEN] = 0;
        round->number_of_dice[BLUE] = 0;
        for(int color_index=0; color_index<COLOR_CNT; ++color_index) {
            regex_t* regex = &regexes[color_index];
            regmatch_t match_pos[1];
            if(regexec(regex, round->round_string, 1, match_pos, 0) == 0) {
                size_t parsed_dice_amount = strtoul(&round->round_string[(size_t)match_pos[0].rm_so], NULL, 10);
                if(parsed_dice_amount > 0 && parsed_dice_amount < 10000) {
                    round->number_of_dice[color_index] = parsed_dice_amount;
                    if(parsed_dice_amount > max_number_of_dice[color_index]) {
                        max_number_of_dice[color_index] = parsed_dice_amount;
                    }
                }
            }
        }

        DEBUG_START(2)
            fprintf(stderr, "Round: %s\n", rawify(round->round_string));
            fprintf(stderr, "R:%ld G:%ld B:%ld\n", round->number_of_dice[RED], round->number_of_dice[GREEN], round->number_of_dice[BLUE]);
        DEBUG_END
    }

    return true;
}

static ssize_t decrypt_riddle_value(const char *file_name) {

    bool failure = false;

    // Declare counting variables for the end results
    size_t sum_valid_game_ids = 0;
    size_t* valid_game_ids = NULL;
    size_t valid_game_ids_cnt = 0;
    // -------------
    size_t cur_game_power = 1;
    size_t sum_game_powers = 0;
    size_t* game_powers = NULL;
    size_t game_powers_cnt = 0;

    // Open the file
    FILE *file;
    if (!try_opening_file(file_name, &file)) {
        failure = true;
        goto cleanup_stage_0;
    }

    // Set up regex
    regex_t* regexes = malloc(COLOR_CNT * sizeof(regex_t));
    if(!try_setting_up_regex(&regexes)) {
        failure = true;
        goto cleanup_stage_1;
    }

    // Set up games struct
    games_t* games = malloc(sizeof(games_t));
    if(games == NULL) {
        perror("Error allocating memory for games");
        failure = true;
        goto cleanup_stage_2;
    }
    games->game_cnt = 0;
    games->all_games = NULL;
    games->max_dice[RED] = RED_MAX_DICE;
    games->max_dice[GREEN] = GREEN_MAX_DICE;
    games->max_dice[BLUE] = BLUE_MAX_DICE;

    // Read each line of the file
    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes;
    while ((read_bytes = getline(&line, &len, file)) != -1) {

        // Set up single game struct
        games->all_games = realloc(games->all_games, (games->game_cnt+1) * sizeof(single_game_t));
        if(games->all_games == NULL) {
            perror("Error allocating memory for game");
            failure = true;
            goto cleanup_stage_3;
        }
        single_game_t* single_game = &games->all_games[games->game_cnt];
        single_game->id = 0;
        single_game->round_cnt = 0;
        single_game->rounds = NULL;
        single_game->max_number_of_dice[RED] = 0;
        single_game->max_number_of_dice[GREEN] = 0;
        single_game->max_number_of_dice[BLUE] = 0;
        games->game_cnt++;
        
        // Get single game id
        if(!try_parsing_game_id(&read_bytes, line, &single_game->id)) {
            fprintf(stderr, "Error parsing game id at line %ld", games->game_cnt);
            failure = true;
            goto cleanup_stage_3;
        }
        
        // Split rounds via ";"
        if(!try_splitting_rounds(line, &single_game->round_cnt, &single_game->rounds)) {
            fprintf(stderr, "Error parsing rounds at line %ld", games->game_cnt);
            failure = true;
            goto cleanup_stage_3;
        }

        // Parse rounds via regex and update max number of dice
        if(!try_parsing_rounds(&single_game->round_cnt, single_game->rounds, regexes, single_game->max_number_of_dice)) {
            fprintf(stderr, "Error parsing rounds at line %ld", games->game_cnt);
            failure = true;
            goto cleanup_stage_3;
        }

        // Calculate sum of invalid game ids and game powers
        bool game_is_valid = true;
        for(int color_id=0; color_id<COLOR_CNT; ++color_id) {
            if(single_game->max_number_of_dice[color_id] > games->max_dice[color_id]) {
                game_is_valid = false; 
            }
            cur_game_power *= single_game->max_number_of_dice[color_id];
        }

        // Update end result - valid games
        if(game_is_valid) {
            valid_game_ids = realloc(valid_game_ids, (valid_game_ids_cnt+1) * sizeof(size_t));
            if(valid_game_ids == NULL) {
                perror("Error reallocating memory for invalid game ids");
                failure = true;
                goto cleanup_stage_3;
            }
            valid_game_ids[valid_game_ids_cnt] = single_game->id;
            valid_game_ids_cnt++;
            sum_valid_game_ids += single_game->id;
        }

        // Update end result - game powers
        game_powers = realloc(game_powers, (game_powers_cnt+1) * sizeof(size_t));
        if(game_powers == NULL) {
            perror("Error reallocating memory for game powers");
            failure = true;
            goto cleanup_stage_3;
        }
        game_powers[game_powers_cnt] = cur_game_power;
        game_powers_cnt++;
        sum_game_powers += cur_game_power;
        cur_game_power = 1;
    }

    DEBUG_START(1)
    // Print invalid ids and their game strings
    for(size_t i=0; i<valid_game_ids_cnt; ++i) {
        fprintf(stderr, "%ld. Valid game ID: %ld\n", (i+1), valid_game_ids[i]);
    }
    fprintf(stderr, "----------------------\n");
    fprintf(stderr, "Sum of valid game IDs: %ld\n\n", sum_valid_game_ids);


    // Print game powers per game
    for(size_t i=0; i<game_powers_cnt; i++) {
        fprintf(stderr, "%ld. Game Power: %ld\n", (i+1), game_powers[i]);
    }
    fprintf(stderr, "----------------------\n");
    fprintf(stderr, "Sum of Game Powers: %ld\n\n", sum_game_powers);
    DEBUG_END

    // Check for an error in getline (other than EOF)
    if (feof(file) == 0) {
        perror("Error reading file");
        failure = true;
    }

    // Clean up
    cleanup_stage_3:
        free(valid_game_ids);
        free(game_powers);

        free(line);

        for(size_t i=0; i<games->game_cnt; ++i) {
            for(size_t j=0; j<games->all_games[i].round_cnt; ++j) {
                free(games->all_games[i].rounds[j].round_string);
            }
            free(games->all_games[i].rounds);
        }
        free(games->all_games);
        free(games);
    cleanup_stage_2:
        for(size_t i=0; i<COLOR_CNT; ++i) {
            regfree(&regexes[i]);
        }
        free(regexes);
    cleanup_stage_1:
        fclose(file);
    cleanup_stage_0:
        return failure ? -1 : sum_valid_game_ids;
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
