#include "config.h"
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#if defined(HAVE_WINDOWS_H) && defined(_WIN32)
#include <windows.h>
#endif
#ifdef HAVE_GL_GL_H
#include <GL/gl.h>
#elif defined(HAVE_OPENGL_GL_H) || defined(HAVE_APPLE_OPENGL_FRAMEWORK)
#include <OpenGL/gl.h>
#else
#error no gl.h
#endif
#if HAVE_WINDOWS_H && defined(_WIN32)
#include <windows.h>
#endif
#if defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#elif defined(HAVE_GLUT_GLUT_H) || defined(HAVE_APPLE_OPENGL_FRAMEWORK)
#include <GLUT/glut.h>
#else
#error no glut.h
#endif
#if !HAVE_BZERO && HAVE_MEMSET
# define bzero(buf, bytes)      ((void) memset (buf, 0, bytes))
#endif
#include <math.h>

using namespace std;

//
// LINDENMAYER SYSTEM VIEWER
// v 1.1, (c) Will Roberts   4 Apr 2005
//
// command-line based
// fast stack-based rewriting system
// graphics context autonormalisation by computing bounding box
// better documentation
//

// ============================================================
//  Header
// ============================================================

// Main menu constants--must be type int.
// Each menu item should be identified by a unique identifier.
const int A1_EXIT = 4;
const int A1_SAVE = 5;

// lindenmayer rewrite functions

char getNextChar();
char getNextGenChar ( const char*, char**, char**, int, int, int*& );
char getNextBufChar ( const char*, int& );

// opengl user interaction

void set_generation ( int gen );
void dump_frame_buffer_to_BMP();
void reshape_callback(int w, int h);


// ============================================================
//  Turtle Stack linked list structure
// ============================================================
//
//  This class is here as a simple utility to aid in the stack-based
//  turtle traversal of a Lindenmayer string.  With the allowance of
//  '[' and ']' characters in the string, the system must provide for
//  a stack-based memory system for unwinding.
//
//  The current turtle state can be completely expressed by the x and
//  y coordinates, and the current heading.  These are packed into an
//  array, which is stored in the state_data pointer.
//

class TurtleStackList {

public:
    TurtleStackList ( float* d, TurtleStackList* n );
    ~TurtleStackList();

    float* state_data;
    TurtleStackList* next;

};

TurtleStackList::TurtleStackList ( float* d, TurtleStackList* n ) {

    state_data = d;
    next = n;

}

TurtleStackList::~TurtleStackList() {

    if ( state_data ) delete state_data;

}




// ============================================================
//  Program Data (global variables)
// ============================================================

// lindenmayer system data
char* command_string = "";
char* init_string = 0;
int num_lrules = 0;
char** lhs = 0;
char** rhs = 0;

// global variables for reading a single command string
int pos = 0;

// global variables for stack-based lindenmayer production system
int rule_based = 0;
int current_depth = 0;
int* gen_string_array = 0;

// turtle control settings
// these variables are settable on the command line
float delta = 40.0;                       // angle to turn by, in degrees
float headinginit = 90.0;

// the current opengl window size
int context_width = 500;
int context_height = 500;

// for view autonormalization
float bounds_xmin = -0.5 * 0.9;
float bounds_ymin = -0.5 * 0.9;
float bounds_xmax = 0.5 * 0.9;
float bounds_ymax = 0.5 * 0.9;



// ============================================================
//  Stack-Based Lindenmayer Rewriting System
// ============================================================

//
// global wrapper function for either single-string or rule-based
// system operation.
//
char
getNextChar()
{

    if (rule_based)
        if (current_depth > 0) {
            int val =  getNextGenChar ( init_string, lhs, rhs, num_lrules,
                                        current_depth + 1, gen_string_array );
            if ( val == 0 && gen_string_array ) {
                /* cout << "\n";    DEBUG */
                delete gen_string_array;
                gen_string_array = 0;
            } /* else cout << (char)val;  DEBUG */
            return val;
        } else
            return getNextBufChar ( init_string, pos );
    else
        return getNextBufChar ( command_string, pos );

}

//
// returns the next character from a single-string system
//
char
getNextBufChar ( const char* cstr, int &pos )
{

    if (pos == strlen(cstr)) return 0;
    else return cstr[pos++];

}

//
// returns the next character from a rule-based production system.
//
// this function uses a stack to explicitly perform the recursive
// operation of rewriting the lindenmayer system.
//
// this stack is stored in the variable gen_string_array.  other
// arguments to this function are:
//
// string    - the initial string for the system
// search    - the left-hand sides of the rewrite rules, in an array
// replace   - the right-hand sides of the rules, in an array
// num_rules - the number of rules in the production system
// maxDepth  - the number of recursions to perform
//
char
getNextGenChar ( const char* string, char** search, char** replace,
                 int num_rules, int maxDepth, int* &gen_string_array )
{
    /*
      DEBUG
      cout << "getnextgenchar\n";
      if (gen_string_array)
      for (int i = 0; i < maxDepth; i++)
      cout << " " << gen_string_array[i];
      else cout << " null array";
      cout << "\n";
      if (gen_string_array) {
      for (int i = 0; i < maxDepth; i++)
      cout << " " << gen_string_array[maxDepth+i];
      cout << "\n";
      }
      `*/

    if ( gen_string_array == 0 ) {
        gen_string_array = new int[2 * maxDepth];
        for ( int i = 0; i < maxDepth * 2; i++ ) {
            gen_string_array[i] = -1;
        }
        gen_string_array[0] = 0;
    }

    int arraylen = 0;
    while ( gen_string_array[++arraylen] >= 0 && arraylen < maxDepth );
    while ( arraylen > 1 &&
            gen_string_array[arraylen - 1] ==
            strlen(replace[ gen_string_array[maxDepth+arraylen - 2] ]) ) {

        gen_string_array[--arraylen] = -1;
        gen_string_array[arraylen - 1] += strlen ( search [
                                                       gen_string_array[maxDepth+arraylen - 1]]);
    }

    int pos = gen_string_array[arraylen - 1];

    if ( arraylen == 1 ) {

        if ( pos == strlen(string) ) {

            // exit if we're finished
            return 0;

        }

        // if the current token needs to be replaced, do it
        // search all search tokens
        for ( int i = 0; i < num_rules; i++ ) {
            if ( strncmp ( string + pos, search[i], strlen(search[i])) == 0 &&
                 arraylen < maxDepth ) {

                // push another level onto the array
                gen_string_array[arraylen] = 0;
                gen_string_array[maxDepth+arraylen-1] = i;
                return getNextGenChar ( string, search, replace, num_rules,
                                        maxDepth, gen_string_array );

            }
        }

        // otherwise, return the current character and advance
        gen_string_array[0]++;
        return string[pos];

    } else {

        if ( pos == strlen (replace[ gen_string_array[maxDepth+arraylen - 2] ])) {
            cerr << "ERROR: pos is equal to the length of replace.";
            exit(1);
        }

        // if the current token needs to be replaced, do it
        // search all strings
        for ( int i = 0; i < num_rules; i++ ) {
            if ( strncmp ( replace[ gen_string_array[maxDepth+arraylen - 2 ] ] + pos,
                           search[i], strlen(search[i])) == 0 &&
                 arraylen < maxDepth ) {

                // push another level onto the array
                gen_string_array[arraylen] = 0;
                gen_string_array[maxDepth+arraylen-1] = i;
                return getNextGenChar ( string, search, replace, num_rules,
                                        maxDepth, gen_string_array );

            }
        }

        // otherwise, return the current character and advance
        gen_string_array[arraylen - 1]++;
        return replace[ gen_string_array[maxDepth+arraylen - 2] ][pos];

    }

}




// ============================================================
//  GLUT User Interaction
// ============================================================

// The function that handles main menu events.
// Whenever a menu item is requested, the system will call this
// function.
void main_menu_callback(int value)
{
    switch(value) {
    case A1_EXIT:
        exit(0);
    case A1_SAVE:
        dump_frame_buffer_to_BMP();
        break;
    default:
        cerr << "Unknown value in main_menu_callback\n";
        exit(1);
    }
    glutPostRedisplay();
}

// responds to keyboard actions
void keyboard(unsigned char key, int x, int y)
{
    if (key=='q' || key=='x') {
        exit(0);
    } else if (key=='s') {
        dump_frame_buffer_to_BMP();
    } else if (key=='+') {
        set_generation(current_depth + 1);
    } else if (key=='-') {
        set_generation(current_depth - 1);
    }
    glutPostRedisplay();
}




// ============================================================
//  Select Lindenmayer Iteration (via context menu)
// ============================================================

// sets the current lindenmayer iteration or generation
void
set_generation ( int gen )
{
    if ( gen < 0 /*|| gen > 9*/ ) return;

    pos = 0;
    current_depth = gen;

    glutPostRedisplay();
}




// ============================================================
//  Frame Buffer Dump to numbered BMP
// ============================================================

//
// dumps the current opengl frame buffer to a bitmap file in the
// current directory.
//
// the function automatically generates a filename for the image, with
// the format GL00000001.BMP.  it checks to see whether a file with
// that name exists; if one does, it then generates the next filename
// in the sequence.  in this way, it produces sequential files, never
// overwriting older data.
//
// the pixel data are read raw from the frame buffer and dumped to a
// binary file, with a true color bitmap header tacked on the top.
//
void
dump_frame_buffer_to_BMP()
{

    // get the filename

    struct stat garbage;

    char* filename;
    int index = 0;
    while ( 1 ) {

        // generate filename
        asprintf ( &filename, "GL%06d.bmp", index );

        // cout << "looking for file " << filename << " . . . ";

        if ( stat ( filename, &garbage ) != 0 ) break;
        free ( filename );
        index++;
        // cout << "found.\n";

    }

    // cout << "not found!\n";
    cout << "Saving Frame Buffer data to file " << filename << "\n";;

    // get the bitmap data

    unsigned char* byte_array;

    // allocate the array
    // cout << "Allocating array.\n";
    byte_array = new unsigned char [ context_height * context_width * 3 ];
    bzero(byte_array, context_height * context_width * 3);

    // get it from opengl
    // cout << "Copying pixel data.\n";
    glReadPixels(0, 0, context_width, context_height,
                 GL_RGB, GL_UNSIGNED_BYTE, byte_array);

    // write the file:
    // cout << "Copying file ... \n";

    // open the file
    FILE* OUTFILE = fopen ( filename, "w" );

    // file header

    putc((0x42), OUTFILE);
    putc((0x4D), OUTFILE);
    for (int i = 0; i < 8; i++)
        putc((0), OUTFILE);
    putc((0x36), OUTFILE);
    for (int i = 0; i < 3; i++)
        putc((0), OUTFILE);

    // info header

    putc((40), OUTFILE);
    for (int i = 0; i < 3; i++)
        putc((0), OUTFILE);
    for (int i = 0; i < 4; i++)
        putc(((context_width & (255 << (i*8))) >> (i*8)), OUTFILE);
    for (int i = 0; i < 4; i++)
        putc(((context_height & (255 << (i*8))) >> (i*8)), OUTFILE);
    putc((1), OUTFILE);
    putc((0), OUTFILE);
    putc((24), OUTFILE);
    for (int i = 0; i < 25; i++)
        putc((0), OUTFILE);

    // palette
    // none (RGB)

    // pixel info
    //int[] bitMasks = {0x000000FF, 0x0000FF00, 0x00FF0000 };
    //int rgb = 0;
    int linebuf = (int)((context_width * 3L) % 4);

    int x, y;

    //  for (y = context_height - 1; y >= 0; y--) {
    for (y = 0; y < context_height; y++) {
        for (x = 0; x < context_width; x++) {

            // blue
            putc(byte_array[(y * context_width + x) * 3 + 2], OUTFILE);

            // green
            putc(byte_array[(y * context_width + x) * 3 + 1], OUTFILE);

            // red
            putc(byte_array[(y * context_width + x) * 3 + 0], OUTFILE);

        }

        for (x = 0; x < linebuf; x++)
            putc((0), OUTFILE);
    }

    // close the file

    // cout << "Done.\nCleaning Up.\n";

    fflush(OUTFILE);
    fclose(OUTFILE);

    delete byte_array;

    free ( filename );

}


// ======================================================================
//  Bounding Box
// ======================================================================

//
// computes the bounding box of the current turtle curve and stores
// the result in global variables bounds_[x/y][min/max]
//
// like the display function, this function must iterate through all
// of the current lindenmayer string to compute the bounds.
//
void bounding_box()
{
    float xpos = 0;
    float ypos = 0;
    float heading = headinginit;   // heading is in degrees

    bounds_xmin = xpos;
    bounds_xmax = xpos;
    bounds_ymin = ypos;
    bounds_ymax = ypos;

    // turtle stack
    TurtleStackList* stack = 0;

    // reset the stack-based production system
    pos = 0;
    if ( gen_string_array ) delete gen_string_array;
    gen_string_array = 0;

    char c;
    while ( 1 ) {
        // get a character off the command string
        c = getNextChar();
        if ( !c ) break;
        switch ( c ) {

        case 'F':
        case 'f':
            // calculate the new point
            xpos += cos ( heading * M_PI / 180.0 );
            ypos += sin ( heading * M_PI / 180.0 );
            if ( xpos < bounds_xmin ) bounds_xmin = xpos;
            if ( xpos > bounds_xmax ) bounds_xmax = xpos;
            if ( ypos < bounds_ymin ) bounds_ymin = ypos;
            if ( ypos > bounds_ymax ) bounds_ymax = ypos;
            break;

        case '-':
            // turn left, i.e. by adding to the heading
            heading += delta;
            break;

        case '+':
            // turn right, i.e. by subtracting from the heading
            heading -= delta;
            break;

        case '[':
            // push the current state onto the stack
            float* state;
            state = new float[3];
            state[0] = xpos;
            state[1] = ypos;
            state[2] = heading;
            stack = new TurtleStackList ( state, stack );
            break;

        case ']':
            // pop the current state from the stack
            if ( stack == 0 ) {
                cerr << "Error popping turtle state from empty stack.\n";
                exit(1);
            }
            TurtleStackList* restore_state;
            restore_state = stack;
            stack = stack->next;

            xpos = restore_state->state_data[0];
            ypos = restore_state->state_data[1];
            heading = restore_state->state_data[2];
            delete restore_state;

            break;

        default:
            break;

        }
    }

    // clean up the turtle stack
    if ( stack ) {

        while ( stack ) {
            TurtleStackList* temp = stack;
            stack = stack->next;
            delete temp;
        }

    }

    reshape_callback(context_width, context_height);

}




// ============================================================
//  Display
// ============================================================

//
// The function that handles drawing the display.  Iterates through
// the current lindenmayer string, interpreting the characters as
// turtle commands.
//
// Turtle output is drawn to the OpenGL context.
//
void display_callback(void)
{
    // recompute bounds
    bounding_box();
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0, 0.0, 0.0);

    // run through the instruction string

    // the current turtle state
    // default starting turtle location is at
    // the bottom facing up
    float xpos = 0;
    float ypos = 0;
    float heading = headinginit; // heading is in degrees

    // this is the turtle state stack
    // initially empty (0)
    TurtleStackList* stack = 0;

    // a flag to show whether we're currently drawing a line
    // strip or not
    int drawing = 0;

    // get a character off the command string
    pos = 0;
    if ( gen_string_array ) delete gen_string_array;
    gen_string_array = 0;
    char c;
    while ( 1 ) {
        c = getNextChar();
        if ( !c ) break;
        switch ( c ) {

        case 'F':
            // move forwards, drawing a point
            if ( !drawing ) glBegin ( GL_LINE_STRIP );
            drawing = 1;

            // the current point
            glVertex3d ( xpos, ypos, 0 );

            // calculate the new point
            xpos += cos ( heading * M_PI / 180.0 );
            ypos += sin ( heading * M_PI / 180.0 );

            glVertex3d ( xpos, ypos, 0 );

            break;

        case 'f':
            // move forwards, without drawing a point
            if ( drawing ) glEnd();
            drawing = 0;

            // calculate the new point
            xpos += cos ( heading * M_PI / 180.0 );
            ypos += sin ( heading * M_PI / 180.0 );

            break;

        case '-':
            // turn left, i.e. by adding to the heading
            heading += delta;

            break;

        case '+':
            // turn right, i.e. by subtracting from the heading
            heading -= delta;

            break;

        case '[':
            // push the current state onto the stack
            float* state;
            state = new float[3];
            state[0] = xpos;
            state[1] = ypos;
            state[2] = heading;
            stack = new TurtleStackList ( state, stack );
            break;

        case ']':
            // pop the current state from the stack
            if ( stack == 0 ) {
                cerr << "Error popping turtle state from empty stack.\n";
                exit(1);
            }
            TurtleStackList* restore_state;
            restore_state = stack;
            stack = stack->next;
            if ( drawing ) glEnd();
            drawing = 0;

            xpos = restore_state->state_data[0];
            ypos = restore_state->state_data[1];
            heading = restore_state->state_data[2];
            delete restore_state;

            break;

        default:

            /*
              SILENT: allows node rewriting

              cerr << "Error reading character " << c << " from command string.\n";
              exit(1);
            */
            break;

        }
    }
    if ( drawing ) glEnd();

    glFlush();
    glutSwapBuffers();

    // clean up the turtle stack
    if ( stack ) {

        while ( stack ) {
            TurtleStackList* temp = stack;
            stack = stack->next;
            delete temp;
        }

    }

}




// ============================================================
//  Window Resize
// ============================================================

//
// The function that handles window resizes.  Uses the currently
// computed bounds to set up the graphics context properly
//
void reshape_callback(int w, int h)
{
    glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));

    float mtop;
    float mbot;
    float mleft;
    float mright;

    if ( (bounds_xmax - bounds_xmin) /
         (float)(bounds_ymax - bounds_ymin) <
         w / (float)h ) {

        mtop = bounds_ymax + (bounds_ymax - bounds_ymin) * 0.1;
        mbot = bounds_ymin - (bounds_ymax - bounds_ymin) * 0.1;
        float mw = w * (mtop - mbot) / h;
        mleft = bounds_xmin - (mw - (bounds_xmax - bounds_xmin)) / 2.0;
        mright = bounds_xmax + (mw - (bounds_xmax - bounds_xmin)) / 2.0;

    } else {

        mleft = bounds_xmin - (bounds_xmax - bounds_xmin) * 0.1;
        mright = bounds_xmax + (bounds_xmax - bounds_xmin) * 0.1;
        float mh = h * (mright - mleft) / w;
        mtop = bounds_ymax + (mh - (bounds_ymax - bounds_ymin)) / 2.0;
        mbot = bounds_ymin - (mh - (bounds_ymax - bounds_ymin)) / 2.0;

    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho ( mleft, mright, mbot, mtop, -0.5, 0.5 );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    context_width = w;
    context_height = h;
}




// ============================================================
//  Initialization and Main Program Loop
// ============================================================

// OpenGL initialization.
void OpenGLInit(void)
{
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glShadeModel(GL_FLAT);
    glPointSize(4.0);
}

//
// main function
//
// 1. read command line arguments and print syntax if there are any
//    mistakes
//
// 2. parse out lindenmayer rules or command string and initialise
//    lindenmayer production system
//
// 3. set up open gl context using glut.  register callback functions
//    for context menu and keyboard.
//
// 4. after program completes, clean up program data.
//
int main(int argc, char **argv)
{

    // parse the command-line arguments
    int bflag, ch, fd;
    int print_syntax = 0;

    bflag = 0;
    while ((ch = getopt(argc, argv, "r:a:")) != -1)
        switch (ch) {
        case 'r':
            headinginit = strtod(optarg,0) + 90.0;
            break;
        case 'a':
            delta = strtod(optarg,0);
            break;
        case '?':
        default:

            print_syntax = 1;

        }
    argc -= optind;
    argv += optind;

    cout << PACKAGE_STRING << " - Lindenmayer System Visualizer\n";
    cout << "(c) 2005, 2014 Will Roberts\n\n";

    if (print_syntax || !argc) {
        cout << "Draws and recursively generates turtle strings using a\n";
        cout << "Lindenmayer grammar specified by the user.\n";
        cout << "Drawing space autosizes.\n";
        cout << "\nSyntax:\n\nlmv [-a delta] [-r rot] LSTRING [INIT]\n\n";
        cout << "  delta:   the angle, in degrees, to turn the turtle by.\n";
        cout << "  rot:     the amount to rotate the picture by, in degrees.\n";
        cout << "  LSTRING: the turtle string to draw or a Lindenmayer rule.\n";
        cout << "  INIT:    the initial turtle string to draw, if using rules.\n";
        cout << "           if not specified, the LHS of the rule is used.\n";
        exit(0);
    }

    command_string = argv[0];

    if ( argc > 1 ) {
        rule_based = 1;
        num_lrules = argc - 1;
        lhs = new char* [ num_lrules ];
        rhs = new char* [ num_lrules ];
        for ( int i = 0; i < argc - 1; i++ ) {
            // extract the left-hand and right-hand sides from the argument
            char* sptr = strstr ( argv[i], ":" );
            if ( sptr == 0 ) {
                cout << "ERROR: malformed L-rule #" << i << " \"" << argv[i]
                     << "\": Missing colon.\n";
                exit(1);
            }
            lhs[i] = new char [ sptr - argv[i] + 1 ];
            bzero(lhs[i], sptr - argv[i] + 1);
            strncpy(lhs[i], argv[i], sptr - argv[i] );

            rhs[i] = new char [ strlen(argv[i]) - (sptr - argv[i]) ];
            bzero(rhs[i], strlen(argv[i]) - (sptr - argv[i]) );
            strncpy(rhs[i], sptr + 1, strlen(argv[i]) - (sptr - argv[i]) - 1);
        }
        init_string = argv[argc - 1];
    }

    cout << "Heading: " << headinginit << "\nDelta: " << delta << "\n";
    if ( init_string ) cout << "Init: " << init_string << "\n";

    // check if we've got a simple rule system
    char* rule_break;
    rule_break = strstr(command_string, ":");
    if (rule_break && !rule_based) {
        rule_based = 1;

        num_lrules = 1;
        lhs = new char* [1];
        rhs = new char*[1];

        lhs[0] = new char [ rule_break - command_string + 1];
        rhs[0] = new char [ strlen(command_string) -
                            (rule_break - command_string) - 1 + 1 ];
        if (!init_string) init_string = lhs[0];

        bzero ( lhs[0], rule_break - command_string + 1);
        strncpy ( lhs[0], command_string, rule_break - command_string );
        bzero ( rhs[0], strlen(command_string) - (rule_break - command_string) );
        strncpy ( rhs[0], rule_break + 1,
                  strlen(command_string) - (rule_break - command_string) );

    }

    if ( rule_based ) {

        for ( int i = 0; i < num_lrules; i++ )
            cout << "Rule " << i << ": " << lhs[i] << " => " << rhs[i] << "\n";
        cout << "\n";

    } else {

        cout << "Command: " << command_string << "\n\n";

    }

    glutInit(&argc, (char **)argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // Make a 500 x 500 window and place it at 100 x 100.
    glutInitWindowSize(context_width, context_height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("L-System Visualizer");

    int generation_submenu = 0;
    if ( rule_based ) {
        generation_submenu = glutCreateMenu ( set_generation );
        glutAddMenuEntry("0", 0);
        glutAddMenuEntry("1", 1);
        glutAddMenuEntry("2", 2);
        glutAddMenuEntry("3", 3);
        glutAddMenuEntry("4", 4);
        glutAddMenuEntry("5", 5);
        glutAddMenuEntry("6", 6);
        glutAddMenuEntry("7", 7);
        glutAddMenuEntry("8", 8);
        glutAddMenuEntry("9", 9);
    }

    // Create the main menu and bind it to the left mouse button.
    glutCreateMenu(main_menu_callback);
    if (rule_based)
        glutAddSubMenu("L-System Generation", generation_submenu );
    glutAddMenuEntry("Save current frame to BMP", A1_SAVE);
    glutAddMenuEntry("Exit", A1_EXIT);
    glutAttachMenu(GLUT_LEFT_BUTTON);

    // Tell glut which function is to handle drawing events.  Whenever
    // the system determines that the window must be redrawn, the
    // callback function "display_callback" will be called.  You can
    // force the function to be called by using glutPostRedisplay().
    glutDisplayFunc(display_callback);

    // Tell glut which function is to handle reshape events.
    glutReshapeFunc(reshape_callback);

    // Tell glut to call this keyboard callback to allow frames to be
    // printed to bmp files as the user desires
    glutKeyboardFunc ( keyboard );

    // initialize
    OpenGLInit();

    // go into an infinite loop (basically)
    glutMainLoop();

    // clean up

    if (rule_based) {
        for (int i = 0; i < num_lrules; i++) {
            delete lhs[i];
            delete rhs[i];
        }
        delete lhs;
        delete rhs;
    }

    return 0;
}
