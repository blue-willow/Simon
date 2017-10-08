
#include "simon.h"

#include <iostream>
#include <list>
#include <cstdlib>
#include <vector>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h> // needed for sleep
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;



/* global variables */
const double pi = M_PI; // pi
const int BufferSize = 10;
int width = 800;  // width of window
int height = 400;  // height of window
int FPS = 60;   // frames per second to run animation loop
bool iswaiting = true; // boolean to indicate if game is waiting for user to press space



// get microseconds
unsigned long now() {
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}


/* Information to draw on the window */
struct XInfo {
    Display* display;
    Window window;
    int screen;
    GC gc;
};


/* An abstract class representing displayable things. */
class Displayable {
public:
    virtual void paint(XInfo& xinfo) = 0;
};


/* A text displayable */
class Text : public Displayable {
    
    int x;
    int y;
    string s; // string to show
    
public:
    
    virtual void paint(XInfo& xinfo) {
        XDrawImageString( xinfo.display, xinfo.window, xinfo.gc,
                         this->x, this->y, this->s.c_str(), this->s.length() );
    }
    /* constructor */
    Text(int x, int y, string s): x(x), y(y), s(s)  {}
    int getx(){return this->x;}
    int gety(){return this->y;}
    void setx(int x){this->x = x;}
    void sety(int y){this->y = y;}
    void sets(string s){this->s = s;}
};


class Button : public Displayable {
    
    int x;
    int y;
    int d; // diameter
    bool highlight;
    
public:
    virtual void paint(XInfo& xinfo) {
        if(ishighlight()){
            XSetLineAttributes(xinfo.display, xinfo.gc, 4, LineSolid, CapButt, JoinRound);
        }else{
            XSetLineAttributes(xinfo.display, xinfo.gc, 1, LineSolid, CapButt, JoinRound);
        }
        XDrawArc(xinfo.display, xinfo.window, xinfo.gc,
                 x - (d / 2),
                 y - (d / 2),
                 d, d, 0, 360 * 64);
    }
    /* constructor */
    Button(int x, int y, int d): x(x), y(y), d(d), highlight(false) {}
    void setx(int x){this->x = x;}
    void sety(int y){this->y = y;}
    void sethighlight(bool h){this->highlight = h;}
    int getx(){return this->x;}
    int gety(){return this->y;}
    bool ishighlight(){return this->highlight;};
};


/* Function to put out a message on error exits */
void error( string str ) {
    cerr << str << endl;
    exit(0);
}


/* Create a window, initialize x */
void initX(int argc, char* argv[], XInfo& xinfo) {
    
    /* Display opening uses the DISPLAY  environment variable.
     * It can go wrong if DISPLAY isn't set, or you don't have permission.*/
    xinfo.display = XOpenDisplay( "" );
    if ( !xinfo.display ) {
        error( "Can't open display." );
    }
    
    /*Find out some things about the display you're using.
     *DefaultScreen is as macro to get default screen index*/
    int screen = DefaultScreen( xinfo.display );
    xinfo.screen = screen;
    unsigned long background = WhitePixel( xinfo.display, screen );
    unsigned long foreground = BlackPixel( xinfo.display, screen);
    
    XSizeHints hints;
    hints.x = 100;
    hints.y = 100;
    hints.width = 800;
    hints.height = 400;
    hints.flags = PPosition | PSize;
    xinfo.window = XCreateSimpleWindow(
                                       xinfo.display,                      // display where window appears
                                       DefaultRootWindow( xinfo.display ), // window's parent in window tree
                                       hints.x, hints.y,                   // upper left corner location
                                       hints.width, hints.height,          // size of the window
                                       5,                                  // width of window's border
                                       foreground,                         // window border colour
                                       background );                       // window background colour
    
    // extra window properties like a window title
    XSetStandardProperties(
                           xinfo.display,    // display containing the window
                           xinfo.window,     // window whose properties are set
                           "a1-basic",       // window's title
                           "a1-basic",       // icon's title
                           None,             // pixmap for the icon
                           argv, argc,       // applications command line args
                           &hints );         // size hints for the window
    
    /* create a simple graphics context */
    xinfo.gc = XCreateGC(xinfo.display, xinfo.window, 0, 0);
    XSetForeground(xinfo.display, xinfo.gc, BlackPixel(xinfo.display, screen));
    XSetBackground(xinfo.display, xinfo.gc, WhitePixel(xinfo.display, screen));
    XSetLineAttributes(xinfo.display, xinfo.gc, 1, LineSolid, CapButt, JoinRound);
    
    /* Tell the window manager what input events you want */
    XSelectInput( xinfo.display, xinfo.window,
                 ButtonPressMask | KeyPressMask |
                 ExposureMask | ButtonMotionMask |
                 PointerMotionMask | StructureNotifyMask);
    
    /* Put the window on the screen. */
    XMapRaised( xinfo.display, xinfo.window );
    XFlush(xinfo.display);
    
    /* give server time to setup */
    sleep(1);
}


/*
 * Function to repaint a display list
 */
void repaint( list<Displayable*> dList, XInfo& xinfo) {
    list<Displayable*>::const_iterator begin = dList.begin();
    list<Displayable*>::const_iterator end = dList.end();
    //XSetLineAttributes(xinfo.display, xinfo.gc, 1, LineSolid, CapButt, JoinRound);
    XClearWindow(xinfo.display, xinfo.window);
    while ( begin != end ) {
        Displayable* d = *begin;
        d->paint(xinfo);
        begin++;
    }
    XFlush(xinfo.display);
}


int main( int argc, char* argv[] ) {
    XInfo xinfo;
    initX(argc, argv, xinfo);
    list<Displayable *> dList;  // list of Displayables
    unsigned long lastRepaint = 0;
    unsigned long lastRepaint2 = 0;
    
    // load a larger font
    XFontStruct * font;
    font = XLoadQueryFont (xinfo.display, "12x24");
    XSetFont (xinfo.display, xinfo.gc, font->fid);
    
    // display users initial score and a message.(basic 1)
    Text *score = new Text(50, 50, "0");
    Text *message = new Text(50, 100, "Press SPACE to play");
    dList.push_back(score);
    dList.push_back(message);
    
    // draw N buttons in the window. (basic 3)
    // get the number of buttons from args
    int n = 4;  //(default to 4 if no args)
    if (argc > 1) {
        n = atoi(argv[1]);
        n = max(1, min(n, 6));  // adjust n s.t 1<=n<=6
    }
    
    Displayable *buttons[2*n];  // storing all buttons and all numbers inside buttons
    int d = 100;    // diameter of a button
    int y = height/2;    // column cordinate of the center of a button
    int distance = (width - 100*n) / (n + 1);     // vertical distance between each button
    int x = -50;    // x is the row cordinate of the center of the current button
    string number = "0";
    for(int k=0; k<n;k++){
        x += distance + d;
        number[0]++;
        buttons[2*k] = new Button(x, y, d);
        buttons[2*k+1] = new Text(x-5, y+8, number);
        dList.push_back(buttons[2*k]);   // draw button
        dList.push_back(buttons[2*k+1]); // draw number inside button
    }
    // paint everything in the display list
    repaint(dList, xinfo);
    XFlush(xinfo.display);
    
    // create the Simon game object
    Simon simon = Simon(n, true);
    cout << "Playing with " << simon.getNumButtons() << " buttons." << endl;
    
    // The loop responding to events from the user.
    XEvent event;
    char text[BufferSize];
    KeySym key;
    while ( true ) {
        if (XPending(xinfo.display) > 0) {
            XNextEvent( xinfo.display, &event );
            switch ( event.type ) {
                    
                case KeyPress:
                {
                    int j = XLookupString((XKeyEvent*)&event, text, BufferSize, &key, 0 );
                    if ( j == 1 && text[0] == 'q' ) {
                        // quit the program when q is pressed.
                        cout << "Terminated normally." << endl;
                        XCloseDisplay(xinfo.display);
                        return 0;
                    }else if ( j == 1 && text[0] == ' ' ){
                        // stop the waiting mode
                        iswaiting = false;
                        // move all buttons to original positon
                        for(int k=0; k<n;k++){
                            ((Button *)buttons[2*k])->sety(height/2);    // update y coordinate of the button
                            ((Text *)buttons[2*k+1])->sety(height/2+8);    // update y coordinate of the text
                        }
                        // change message and play buttons when SPACE is pressed.
                        // change the message context
                        message->sets("Watch what I do ...");
                        // clear the window before repainting
                        XClearWindow(xinfo.display, xinfo.window);
                        // paint everything in the display list
                        repaint(dList, xinfo);
                        XFlush(xinfo.display);
                        // play buttons, highlight each button for about half a second
                        // start new round with a new sequence
                        simon.newRound();
                        // highlight buttons that computer played
                        while (simon.getState() == Simon::COMPUTER) {
                            // keep a short time gap between highlighting
                            usleep(300000);
                            Button *play = (Button*)(buttons[2*simon.nextButton()]);
                            x = play->getx();
                            y = play->gety();
                            // draw a white ring animates from the outside of the circle to the centre.
                            int d_ring = d;
                            while(d_ring >= 0) {
                                // change the button color to black
                                XFillArc(xinfo.display, xinfo.window, xinfo.gc,
                                         x - (d / 2),
                                         y - (d / 2),
                                         d, d, 0, 360 * 64);
                                XSetForeground(xinfo.display, xinfo.gc, WhitePixel(xinfo.display, xinfo.screen));
                                unsigned long end = now();
                                if (end - lastRepaint > 1000000/FPS) {
                                    if(d_ring != d) {
                                        XSetLineAttributes(xinfo.display, xinfo.gc, 1, LineSolid, CapButt, JoinRound);
                                    }
                                    XDrawArc(xinfo.display, xinfo.window, xinfo.gc,
                                             x - (d_ring / 2),
                                             y - (d_ring / 2),
                                             d_ring, d_ring, 0, 360 * 64);
                                    lastRepaint = now();
                                }
                                XSetForeground(xinfo.display, xinfo.gc, BlackPixel(xinfo.display, xinfo.screen));
                                d_ring -= 3;
                                // give the system time to do other things
                                if (XPending(xinfo.display) == 0) {
                                    usleep(1000000/FPS - (end - lastRepaint));
                                }
                            }
                            usleep(50000);
                            // paint everything in the display list
                            repaint(dList, xinfo);
                            XFlush(xinfo.display);
                        }
                        // change the message context
                        message->sets("Now it's your turn");
                        // clear the window before repainting
                        XClearWindow(xinfo.display, xinfo.window);
                        // paint everything in the display list
                        repaint(dList, xinfo);
                        XFlush(xinfo.display);
                    }
                    break;
                }
                case ConfigureNotify:
                {
                    XConfigureEvent xce = event.xconfigure;
                    if (xce.width != width || xce.height != height) {
                        width = xce.width;
                        height = xce.height;
                    }
                    y = height/2;    // column cordinate of the center of a button
                    distance = (width - 100*n) / (n + 1);     // vertical distance between each button
                    x = -50;    // x is the row cordinate of the center of the current button
                    number = "0";
                    for(int k=0; k<n;k++){
                        x += distance + d;
                        ((Button *)buttons[2*k])->setx(x);    // update x coordinate of the button
                        ((Button *)buttons[2*k])->sety(y);    // update y coordinate of the button
                        ((Text *)buttons[2*k+1])->setx(x-5);    // update x coordinate of the button
                        ((Text *)buttons[2*k+1])->sety(y+8);    // update y coordinate of the button
                    }
                    
                    XClearWindow(xinfo.display, xinfo.window);
                    // paint everything in the display list
                    repaint(dList, xinfo);
                    XFlush(xinfo.display);
                    break;
                }
                case MotionNotify:
                {
                    for(int k=0; k<n;k++){
                        //((Button *)buttons[2*k])->sethighlight(true);
                        int xdiff = event.xmotion.x - ((Button *)buttons[2*k])->getx();    // calculate the horizontal distance
                        int ydiff = event.xmotion.y - ((Button *)buttons[2*k])->gety();    // calculate the vertical distace
                        if((xdiff*xdiff + ydiff*ydiff) <= 50*50&& !((Button *)buttons[2*k])->ishighlight()){
                            // if pointer is inside the button, highlight the boarder.
                            ((Button *)buttons[2*k])->sethighlight(true);
                        }else if((xdiff*xdiff + ydiff*ydiff) > 50*50 && ((Button *)buttons[2*k])->ishighlight()){
                            //if pointer leaves the button, remove the highlight
                            ((Button *)buttons[2*k])->sethighlight(false);
                        }
                        XClearWindow(xinfo.display, xinfo.window);
                        // paint everything in the display list
                        repaint(dList, xinfo);
                        XFlush(xinfo.display);
                    }
                    break;
                }
                case ButtonPress:
                {
                    for(int k=0; k<n;k++){
                        int xdiff = event.xmotion.x - ((Button *)buttons[2*k])->getx();    // calculate the horizontal distance
                        int ydiff = event.xmotion.y - ((Button *)buttons[2*k])->gety();    // calculate the vertical distace
                        
                        if((xdiff*xdiff + ydiff*ydiff) <= 50*50){
                            x = ((Button *)buttons[2*k])->getx();
                            y = ((Button *)buttons[2*k])->gety();
                            
                            // draw a white ring animates from the outside of the circle to the centre.
                            int d_ring = d;
                            while(d_ring >= 3) {
                                d_ring -= 3;
                                // change the button color to black
                                XFillArc(xinfo.display, xinfo.window, xinfo.gc,
                                         x - (d / 2),
                                         y - (d / 2),
                                         d, d, 0, 360 * 64);
                                XSetForeground(xinfo.display, xinfo.gc, WhitePixel(xinfo.display, xinfo.screen));
                                unsigned long end = now();
                                if (end - lastRepaint > 1000000/FPS) {
                                    if(d_ring != d) {
                                        XSetLineAttributes(xinfo.display, xinfo.gc, 1, LineSolid, CapButt, JoinRound);
                                    }
                                    XDrawArc(xinfo.display, xinfo.window, xinfo.gc,
                                             x - (d_ring / 2),
                                             y - (d_ring / 2),
                                             d_ring, d_ring, 0, 360 * 64);
                                    lastRepaint = now();
                                }
                                XSetForeground(xinfo.display, xinfo.gc, BlackPixel(xinfo.display, xinfo.screen));
                                // give the system time to do other things
                                if (XPending(xinfo.display) == 0) {
                                    usleep(1000000/FPS - (end - lastRepaint));
                                }
                            }
                            
                            // paint everything in the display list
                            repaint(dList, xinfo);
                            XFlush(xinfo.display);
                            
                            // highlight the boarder at the end of the ring animation
                            XSetLineAttributes(xinfo.display, xinfo.gc, 4, LineSolid, CapButt, JoinRound);
                            ((Button *)buttons[2*k])->paint(xinfo);
                            ((Button *)buttons[2*k])->sethighlight(true);
                            
                            if (simon.getState() == Simon::HUMAN) {
                                if (!simon.verifyButton(k)) {
                                    // start the waiting mode
                                    iswaiting = true;
                                    // change the message context
                                    message->sets("You lose. Press SPACE to play again");
                                    score->sets("0");
                                    // clear the window before repainting
                                    XClearWindow(xinfo.display, xinfo.window);
                                    // paint everything in the display list
                                    repaint(dList, xinfo);
                                    XFlush(xinfo.display);
                                }else{
                                    if(simon.getState() == Simon::WIN) {
                                        // start the waiting mode
                                        iswaiting = true;
                                        // they won last round
                                        // score is increased by 1, sequence length is increased by 1
                                        // change the message context
                                        message->sets("You won! Press SPACE to continue");
                                        // increase the score
                                        number = "0";
                                        number[0] += simon.getScore();
                                        score->sets(number);
                                        // clear the window before repainting
                                        XClearWindow(xinfo.display, xinfo.window);
                                        // paint everything in the display list
                                        repaint(dList, xinfo);
                                        XFlush(xinfo.display);
                                    }
                                }
                            }
                        }
                    } // for
                } // case
            }   // switch
        }else{
            
            if (iswaiting) {
                double startangle = 0;
                double betweenangle = 2*pi/n;
                while(startangle < 2*pi){
                    startangle += pi/24;
                    y = height/2;    // column cordinate of the center of a button
                    for(int k=0; k<n;k++){
                        // update y coordinate of the button
                        ((Button *)buttons[2*k])->sety(y+10*sin(startangle+k*betweenangle));
                        // update y coordinate of the text
                        ((Text *)buttons[2*k+1])->sety(y+8+10*sin(startangle+k*betweenangle));
                    }
                    unsigned long end = now();
                    XClearWindow(xinfo.display, xinfo.window);
                    // paint everything in the display list
                    repaint(dList, xinfo);
                    XFlush(xinfo.display);
                    lastRepaint2 = now();
                    
                    if (XPending(xinfo.display) != 0) {
                        break;
                    } else {
                        //give the system time to do other things
                        //usleep(1000000/FPS - (end - lastRepaint2));
                        usleep(25000);
                    }
                }
            }
            
        }
    } // while
} // main
