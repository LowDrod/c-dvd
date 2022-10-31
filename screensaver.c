#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

// debug flag
// #define DEBUG

#define PROJECT_NAME "screensaver"

// colors for debuging 
#define WHITE   "\033[0;97m"
#define CYAN    "\033[0;96m"
#define RED     "\033[0;91m"
#define MAGENTA "\033[0;95m"
#define RESET   "\033[0m"             // reset color to default

#define ERASE_SCREEN "\033[2J"        // erase entire screen
#define TO_ZERO      "\033[H"         // moves cursor to home position (0, 0)
#define CURSOR_TRUE  "\033[?25h"      // cursor visiblility: TRUE
#define CURSOR_FALSE "\033[?25l"      // cursor visiblility: FALSE
#define BLACK_BCK    "\033[48;5;0m"   // black background

unsigned char last_color = 0, current_color;
unsigned char color_type = 3; // 4 == background // 3 == foreground

unsigned int SLEEP_TIME = (unsigned int)1000000 / (unsigned int)15; // time between frames

unsigned short ascii_x;   // ascii widthz
unsigned short ascii_y;   // ascii height
unsigned short ascii_px;  // ascii printable width

char **ascii_img;         // ascii image

// bool for main loop
static volatile bool running = 1;

// main loop breaker
void loop_breaker(int bb) {
    running = 0;
}

// ----- ----- ----- ----- -----        ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- ----- PARSER ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- -----        ----- ----- ----- ----- ----- //

void parser(FILE **file, int argc, char *argv[]){
    bool error = false,
        path = false,
        speed = false;

    for (int i = 1; i < argc; i++){
        // TODO: alternative to strcmp 

        // ----- HELP ----- //
        if (strcmp(argv[i],"-h") == 0){
            printf(
                "Usage: " PROJECT_NAME " [OPTIONS]...\n"
                "\n"
                "  -p [/path/to/file]     path to ascii file you wanna use\n"
                "  -h                     show this help\n"
                "  -s [number]            number of refresh per second\n"
                "  -b                     makes the background change color\n"
                "                         every time the ascii hit an edge\n"
                "  -f                     makes the foreground change color\n"
                "                         every time the ascii hit an edge\n"
            );

            if (path)
                fclose(*file);
            exit(0);
        }

        // ----- PATH TO FILE ----- //
        else if (strcmp(argv[i],"-p") == 0){
            if (i+1 >= argc){
                fprintf(stderr, "%sSYNTAX ERROR:%s Missing argument for \"%s\"\n", RED, RESET, argv[i]);
                error = true;
                continue;
            }
            path = true;
            *file = fopen(argv[++i],"r");
            if (*file == NULL){
                fprintf(stderr, "%sERROR:%s Could not open file %s\n", RED, RESET, argv[i]);
                error = true;
            }
        }

        // ----- SPEED ----- //
        else if (strcmp(argv[i],"-s") == 0){
            if (i+1 >= argc){
                fprintf(stderr, "%sSYNTAX ERROR:%s Missing argument for \"%s\"\n", RED, RESET, argv[i]);
                error = true;
                continue;
            }
            i++;
            // FIXME: strtol sucks :c
            unsigned int fps = strtol(argv[i],NULL,10);
            if (fps == 0){
                fprintf(stderr, "%sSYNTAX ERROR:%s Not valid argument for \"%s\": \"%s\"\n", RED, RESET, argv[i-1], argv[i]);
                error = true;
                continue;
            }
            speed = true;
            SLEEP_TIME = (unsigned int)1000000 / (unsigned int)fps;
        }

        // ----- BACKGROUND ----- //
        else if (strcmp(argv[i],"-b") == 0){
            color_type = 4;
        }

        // ----- FOREGROUND ----- //
        else if (strcmp(argv[i],"-f") == 0){
            color_type = 3;
        }

        // ----- DEFAULT ----- //
        else {
            fprintf(stderr,"%sSYNTAX ERROR:%s Unknown argument: %s\n", RED, RESET, argv[i]);
            error = true;
        }
    }

    #if defined(DEBUG)
    printf("%sDEBUG:%s PATH  STATUS: %d => %s\n", MAGENTA, RESET, path,  path  ? "true" : "false");
    printf("%sDEBUG:%s SPEED STATUS: %d => %s\n", MAGENTA, RESET, speed, speed ? "true" : "false");
    printf("%sDEBUG:%s ERROR STATUS: %d => %s\n", MAGENTA, RESET, error, error ? "true" : "false");
    printf("\n");
    #endif

    if (!path){
        fprintf(stderr, "%sERROR:%s Path to file is required\n", RED, RESET);
        printf("%snote:%s You can specify the path by using \"-p /path/to/file\"\n", CYAN, RESET);
        if (path)
            fclose(*file);
        exit(1);
    }
    if (error){
        if (path)
            fclose(*file);
        exit(1);
    }
}

// ----- ----- ----- ----- -----                 ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- ----- FILE_DIMENSIONS ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- -----                 ----- ----- ----- ----- ----- //

void file_dimensions(FILE *file){
    ascii_x = ascii_px = 0, ascii_y = 1;
    
    // real width && printable width
    unsigned short tmp_x = 0, tmp_px = 0;

    char c; 
    unsigned char uc;

    for (c = getc(file); c != (char) EOF; c = getc(file)){
        if (c == '\n'){
            // ascii height
            ascii_y += 1;
            if (tmp_x > ascii_x){
                ascii_x = tmp_x;
            }
            if (tmp_px > ascii_px){
                ascii_px = tmp_px;
            }
            tmp_x = tmp_px = 0;
        } else {
            // ascii width
            tmp_x += 1;

            uc = (unsigned) c;

            // ascii printable width
            if (uc >= 0xF0){ // 4 byte char
                // 1111 0000 => 240 => 0xF0
                tmp_px -= 2;
            }
            else if (uc >= 0xE0){ // 3 byte char
                // 1110 0000 => 224 => 0xE0
                tmp_px -= 1;
            }
            else if (uc >= 0xC0){ // 2 byte char
                // 1100 0000 => 192 => 0xC0
                //tmp_px -= 0;
            }
            else { // 1 byte char
                tmp_px += 1;
            }
        }
    }

    if (tmp_x > ascii_x){
        ascii_x = tmp_x;
    }
    if (tmp_px > ascii_px){
        ascii_px = tmp_px;
    }

    #if defined(DEBUG)
    printf("%sDEBUG:%s HEIGHT:          %hu\n", MAGENTA, RESET, ascii_y);
    printf("%sDEBUG:%s WIDTH:           %hu\n", MAGENTA, RESET, ascii_x);
    printf("%sDEBUG:%s PRINTABLE WIDTH: %hu\n", MAGENTA, RESET, ascii_px);
    printf("\n");
    #endif
}

// ----- ----- ----- ----- -----             ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- ----- STORE_ASCII ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- -----             ----- ----- ----- ----- ----- //

void store_ascii(FILE *file){
    #if defined(DEBUG)
    printf("%sDEBUG:%s ASCII_IMG:\n", MAGENTA, RESET);
    printf("%s>>> ----- ----- ----- ----- ----- <<<%s\n", MAGENTA, RESET);
    #endif

    ascii_img = malloc(ascii_y * sizeof(void *));
    for (unsigned short i = 0, j; i < ascii_y; i++){
        ascii_img[i] = malloc((ascii_x + 1) * sizeof(char));
    }

    unsigned short x = 0,y = 0;
    char c;
    
    while(y < ascii_y){
        c = getc(file);
        if (c != '\n' && c != (char) EOF){
            ascii_img[y][x++] = c;
            continue;
        }
        if(x < ascii_x - 1){
            ascii_img[y][x] = '\0';
        }
        x=0;
        y++;
    }
    
    #if defined(DEBUG)
    for (unsigned short i = 0, j; i < ascii_y; i++){
        printf("%s\n", ascii_img[i]);
    }
    
    printf("%s>>> ----- ----- ----- ----- ----- <<<%s\n", MAGENTA, RESET);
    printf("\n");
    #endif
}

// ----- ----- ----- ----- -----              ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- ----- CHANGE_COLOR ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- -----              ----- ----- ----- ----- ----- //

void change_color(void){
    current_color = rand() / 16;
    if (current_color == last_color)
        current_color++;

    printf("\033[%u8;5;%um", color_type, current_color); // BACKGROUND
    // printf("\033[38;5;%dm",current_color); // FOREGROUND
}

// ----- ----- ----- ----- -----      ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- ----- SHOW ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- -----       ----- ----- ----- ----- ----- //

void show(void){
    // readability
    #define term_x w.ws_col
    #define term_y w.ws_row

    struct winsize w;
    unsigned short last_ws_row, last_ws_col;
    
    // actual position
    int pos_x = 0,pos_y = 0;
    // velocity + direction
    char velocity_x = 1, velocity_y = 1;

    printf(CURSOR_FALSE BLACK_BCK);

    while (running) {
        ioctl(0, TIOCGWINSZ, &w);

        fflush(stdout);
        usleep(SLEEP_TIME);

        // if the terminal size change
        if(last_ws_col != term_x || last_ws_row != term_y){
            pos_x = pos_y = 1;
            velocity_x = velocity_y = 1;

            last_ws_row = term_y;
            last_ws_col = term_x;
            continue;
        }

        // screen too smol!
        if (term_x < ascii_px + 10 || term_y < ascii_y){
            printf(WHITE BLACK_BCK ERASE_SCREEN TO_ZERO);
            printf("\033[%u;%uH",term_y/2,(term_x-17)/2); // 17 == smol message lenght
            printf("screen too smol!!");

            pos_x = pos_y = 1;
            velocity_x = velocity_y = 1;

            last_ws_row = term_y;
            last_ws_col = term_x;
            continue;
        }

        if(pos_x < 1 || pos_x > term_x - ascii_px){
            velocity_x *= -1;
            change_color();
        }

        if(pos_y < 1 || pos_y > term_y - (ascii_y + 1)){
            velocity_y *= -1;
            change_color();
        }

        printf(ERASE_SCREEN TO_ZERO);
        for (unsigned short i = 0; i < ascii_y; i++){
            printf("\033[%u;%uH",pos_y+i+1,pos_x);
            printf("%s", ascii_img[i]);
            printf("\033[1B");
        }

        pos_x += velocity_x;
        pos_y += velocity_y;
    }
    
    printf(RESET ERASE_SCREEN TO_ZERO CURSOR_TRUE);

    #undef term_x
    #undef term_y
}

// ----- ----- ----- ----- -----            ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- ----- ASCII_FREE ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- -----            ----- ----- ----- ----- ----- //

void ascii_free(){
    for (unsigned short i = 0; i < ascii_y;i ++){
        free(ascii_img[i]);

        #if defined(DEBUG)
        printf("%sDEBUG:%s free(ascii_img[%hu])\n", MAGENTA, RESET, i);
        #endif
    }
    free(ascii_img);
    
    #if defined(DEBUG)
    printf("%sDEBUG:%s free(ascii_img)\n", MAGENTA, RESET);
    printf("\n");
    #endif
}

// ----- ----- ----- ----- -----            ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- ----- DEBUG_ARGS ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- -----            ----- ----- ----- ----- ----- //

#if defined(DEBUG)
void debug_args(int argc, char const *argv[]){
    printf("%sDEBUG:%s argc: %d\n", MAGENTA, RESET, argc);
    printf("\n");
    
    for(int i = 0; i < argc; i++){
        printf("%sDEBUG:%s argv[%d] == %s\n", MAGENTA, RESET, i, argv[i]);
    }
    printf("\n");
}
#endif

// ----- ----- ----- ----- -----      ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- ----- MAIN ----- ----- ----- ----- ----- //
// ----- ----- ----- ----- -----      ----- ----- ----- ----- ----- //

int main(int argc, char const *argv[]){

    srand(time(NULL)); // random seed

    FILE *file;
    parser(&file,argc,(char**)argv);
    
    file_dimensions(file);
    fseek(file, 0, SEEK_SET);
    store_ascii(file);
    fclose(file); // RIP, not necessary any more :c

    // handler for "ctrl + c"
    signal(SIGINT, loop_breaker);

    #if !defined(DEBUG)
    show();
    #endif

    ascii_free();
    
    #if defined(DEBUG)
    printf("%sDEBUG:%s END\n", MAGENTA, RESET);
    #endif
    return 0;
}
