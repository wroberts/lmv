
TARGET = lmv

CC = g++

OBJECTS = lmv.o

CC=g++
# INCLUDE= -I/Users/wroberts/dev/lm/math++
FRAMEWORKS= -F/System/Library/Frameworks/OpenGL.framework \
      -F/System/Library/Frameworks/GLUT.framework \
      -F/System/Library/Frameworks/Foundation.framework
LIBS= -lstdc++

# LOCALLIB= /Users/wroberts/dev/lm/math++.a

CFLAGS = $(FRAMEWORKS) $(INCLUDE)

$(TARGET): $(OBJECTS)
	g++ $(FRAMEWORKS) -o $(TARGET) $(INCLUDE) $(OBJECTS) $(LIBS) $(LOCALLIB) -framework OpenGL -framework GLUT -framework Foundation

.SUFFIXES: .cpp .o
.cpp.o:
	$(CC) $(CFLAGS) -c $<
