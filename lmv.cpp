#include <iostream.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <OpenGL/gl.h>
#include <GLUT/glut.h>

// ============================================================
//  Declarations (embryonic header file)
// ============================================================

void dump_frame_buffer_to_BMP();


// ============================================================
//  Turtle Stack linked list structure
// ============================================================

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
//  Program Constants, Global Variables, etc.
// ============================================================

// this was used to generate lsystems in character arrays
const int BUFFER_SIZE = 10 * 1024;

int context_width = 500;
int context_height = 500;

// Main menu constants--must be type int.
// Each menu item should be identified by a unique identifier.
const int A1_EXIT = 4;
const int A1_SAVE = 5;

// these variables should also be settable on the command line
float delta = 40.0;                       // angle to turn by, in degrees
float d = 0.25;                           // the distance to travel
float dinit = d;
float scale = 1;

float xinit = 0;                          // initial x position of turtle
float yinit = -0.5;                       // initial y position of turtle
float headinginit = 90.0;

// "F -> F[+F]F[-F]F", generation 3.
char* command_string = "";
char* init_string = 0;
int num_lrules = 0;
char** lhs = 0;
char** rhs = 0;

// global variables for using a lindenmeyer production system
int rule_based = 0;
int current_depth = 0;
int* gen_string_array = 0;

// global variables for reading a single command string
int pos = 0;


// ============================================================
//  Context-Sensitive Menu
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




// ============================================================
//  Lindenmeyer rewrite rules
// ============================================================

char getNextChar();
char getNextGenChar ( const char*, char**, char**, int, int, int*& );
char getNextBufChar ( const char*, int& );

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

char
getNextBufChar ( const char* cstr, int &pos )
{

  if (pos == strlen(cstr)) return 0;
  else return cstr[pos++];

}

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
//  Lindenmeyer Generation (via context menu)
// ============================================================

void
set_generation ( int gen )
{
  if ( gen < 0 || gen > 9 ) return;

  pos = 0;
  current_depth = gen;

  d = dinit / pow ( scale, gen );

  glutPostRedisplay();
}




// ============================================================
//  Frame Buffer dump to numbered BMP
// ============================================================

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

void keyboard(unsigned char key, int x, int y)
{
	if (key=='q' || key=='x') {
		exit(0);
	} else if (key=='s') {
	  dump_frame_buffer_to_BMP();
	}
	glutPostRedisplay();
}





// ============================================================
//  Display Function
// ============================================================

//
// The function that handles drawing the display.
// This function must be defined somewhere.
//
void display_callback(void)
{
  glClear(GL_COLOR_BUFFER_BIT);

  glColor3f(0.0, 0.0, 0.0);

  // run through the instruction string

  // the current turtle state
  // default starting turtle location is at
  // the bottom facing up
  float xpos = xinit;
  float ypos = yinit;
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
      xpos += d * cos ( heading * M_PI / 180.0 );
      ypos += d * sin ( heading * M_PI / 180.0 );

      glVertex3d ( xpos, ypos, 0 );

      break;

    case 'f':
      // move forwards, without drawing a point
      if ( drawing ) glEnd();
      drawing = 0;
      
      // calculate the new point
      xpos += d * cos ( heading * M_PI / 180.0 );
      ypos += d * sin ( heading * M_PI / 180.0 );
      
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

}




// ============================================================
//  Window Resize Callback
// ============================================================

//
// The function that handles window resizes.
// Not necessary.
//
void reshape_callback(int w, int h)
{
  glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if(w <= h)
    glOrtho(-0.5,
            0.5,
            -0.5 * static_cast<GLfloat>(h) / static_cast<GLfloat>(w),
            0.5 * static_cast<GLfloat>(h) / static_cast<GLfloat>(w),
            -0.5,
            0.5);
  else
    glOrtho(-0.5 * static_cast<GLfloat>(w) / static_cast<GLfloat>(h),
            0.5 * static_cast<GLfloat>(w) / static_cast<GLfloat>(h),
            -0.5,
            0.5,
            -0.5,
            0.5);

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

int main(int argc, char **argv)
{

  // parse the command-line arguments
  int bflag, ch, fd;
  int print_syntax = 0;

  bflag = 0;
  while ((ch = getopt(argc, argv, "x:y:h:d:a:s:")) != -1)
    switch (ch) {
    case 'x':
      xinit = strtod(optarg,0);
      break;
    case 'y':
      yinit = strtod(optarg,0);
      break;
    case 'h':
      headinginit = strtod(optarg,0);
      break;
    case 'd':
      d = dinit = strtod(optarg,0);
      break;
    case 'a':
      delta = strtod(optarg,0);
      break;
    case 's':
      scale = strtod(optarg,0);
      break;
    case '?':
    default:

      print_syntax = 1;

    }
  argc -= optind;
  argv += optind;

  cout << "lmv - Lindenmeyer System Visualizer\n";
  cout << "(c) 2005 Will Roberts\n\n";

  if (print_syntax || !argc) {
    cout << "Draws and recursively generates turtle strings using a\n";
    cout << "Lindenmeyer grammar specified by the user.\n";
    cout << "Drawing space ranges from at least -0.5 to +0.5 in both x and y.\n";
    cout << "\nSyntax:\n\nlmv [-x xin] [-y yin] [-a del]";
    cout << " [-h hed] [-d dis] [-s sca] LSTRING [INIT]\n\n";
    cout << "  xin:     initial x-location of the turtle as a real coord.\n";
    cout << "  yin:     initlal y-location of the turtle as a real coord.\n";
    cout << "  del:     the angle, in degrees, to turn the turtle by.\n";
    cout << "  hed:     the initial heading, in degrees, of the turtle.\n";
    cout << "  dis:     the distance to travel at each forward step.\n";
    cout << "  sca:     the size of the RHS divided by the size of the LHS.\n";
    cout << "  LSTRING: the turtle string to draw or a Lindenmeyer rule.\n";
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

  cout << "Xpos: " << xinit << "\nYpos: " << yinit;
  cout << "\nHeading: " << headinginit << "\nDelta: " << delta;
  cout << "\nDist: " << d << "\nScale: " << scale << "\n";
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
  glutInitWindowSize(500, 500);
  glutInitWindowPosition(100, 100);
  glutCreateWindow("L-Sytem Visualizer");

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
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  // Tell glut which function is to handle drawing events.
  // Whenever the system determines that the window must be
  // redrawn, the callback function "display_callback" will
  // be called.  You can force the function to be called by
  // using glutPostRedisplay().
  glutDisplayFunc(display_callback);

  // Tell glut which function is to handle reshape events.
  glutReshapeFunc(reshape_callback);

  // Tell glut to call this keyboard callback
  // to allow frames to be printed to bmp files as the user desires
  glutKeyboardFunc ( keyboard );

  // initialize
  OpenGLInit();

  // go into an infinite loop (basically)
  glutMainLoop();

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

