#include <fcntl.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>    
#include <unistd.h>   
#include <errno.h>
#include <X11/X.h>    
#include <X11/Xlib.h> 
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include <sys/soundcard.h>

#include "main.xpm"

#define calc(x) (((sndval[x] & 0x7f)+((sndval[x] >>8) & 0x7f))/2)
#define FSAMPLE   8000   /* sampling rate, 8KHz */
#define FLOAT_TO_SAMPLE(x)    ((char)((x + 1.0) * 127.0))
#define SOUND_DEV  "./8bit.fifo"
typedef char sample;


void draw_switches(int, int);
void draw_slider(int, int);
void pressEvent(XButtonEvent *);
void releaseEvent(XButtonEvent *);
void keyEvent(XKeyEvent *);
void setmix(int,int);

int mixtype[8]={SOUND_MIXER_VOLUME,SOUND_MIXER_BASS,SOUND_MIXER_TREBLE,\
		SOUND_MIXER_PCM,SOUND_MIXER_SYNTH,SOUND_MIXER_CD,\
		SOUND_MIXER_LINE,SOUND_MIXER_MIC};
int recsource;



int screenwidth;
int screenheight;
int screen;
int depth;
char *dispname;
Window rootwin;
Window win;
Display *display;        
GC maingc;

Pixmap pixmap;
Pixmap mainpanel;

Pixmap dialer;
XImage *handle;

int devmask;
int sndval[SOUND_MIXER_NRDEVICES];


int dragging=0;
int currslider=0;

typedef struct stereo
{
  u_char left;
  u_char right;
  u_char pad1;
  u_char pad2;
} StereoVolume;

/*
 * take the sine of x, where x is 0 to 65535 (for 0 to 360 degrees)
 */
float mysine(in)
short in;
{
  static coef[] = {
     3.140625, 0.02026367, -5.325196, 0.5446778, 1.800293 };
  float x,y,res;
  int sign,i;
 
  if(in < 0) {       /* force positive */
    sign = -1;
    in = -in;
  } else
    sign = 1;
  if(in >= 0x4000)      /* 90 degrees */
    in = 0x8000 - in;   /* 180 degrees - in */
  x = in * (1/32768.0); 
  y = x;               /* y holds x^i) */
  res = 0;
  for(i=0; i<5; i++) {
    res += y * coef[i];
    y *= x;
  }
  return(res * sign); 
}

/*
 * play tone1 and tone2 (in Hz)
 * for 'length' milliseconds
 * outputs samples to sound_out
 */
two_tones(sound_out,tone1,tone2,length)
int sound_out;
unsigned int tone1,tone2,length;
{
#define BLEN 128
  sample cout[BLEN];
  float out;
  unsigned int ad1,ad2;
  short c1,c2;
  int i,l,x;
   
  ad1 = (tone1 << 16) / FSAMPLE;
  ad2 = (tone2 << 16) / FSAMPLE;
  l = (length * FSAMPLE) / 1000;
  x = 0;
  for( c1=0, c2=0, i=0 ;
       i < l;
       i++, c1+= ad1, c2+= ad2 ) {
    out = (mysine(c1) + mysine(c2)) * 0.5;
    cout[x++] = FLOAT_TO_SAMPLE(out);
    if (x==BLEN) {
      write(sound_out, cout, x * sizeof(sample));
      x=0;
    }
  }
  write(sound_out, cout, x);
}

/*
 * silence on 'sound_out'
 * for length milliseconds
 */
silence(sound_out,length)
int sound_out;
unsigned int length;
{
  int l,i,x;
  static sample c0 = FLOAT_TO_SAMPLE(0.0);
  sample cout[BLEN];

  x = 0;
  l = (length * FSAMPLE) / 1000;
  for(i=0; i < l; i++) {
    cout[x++] = c0;
    if (x==BLEN) {
      write(sound_out, cout, x * sizeof(sample));
      x=0;
    }
  }
  write(sound_out, cout, x);
}

/*
 * play a single dtmf tone
 * for a length of time,
 * input is 0-9 for digit, 10 for * 11 for #
 */
dtmf(digit, length)
int digit, length;
{
  /* Freqs for 0-9, *, # */
  static int row[] = {
    941, 697, 697, 697, 770, 770, 770, 852, 852, 852, 941, 941 };
  static int col[] = {
    1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1447,
    1209, 1477 };
 static int sound_fd = 0;
 sound_fd = (!sound_fd) ? open(SOUND_DEV,O_RDWR) : sound_fd;
  if(sound_fd<0) {
    perror(SOUND_DEV);
    return(-1);
  }
  //usleep(10000);
  two_tones(sound_fd, row[digit], col[digit], length);
  silence(sound_fd,50);
  
}

int main(int argc, char *argv[]) {
	char ch;
	if (!((argc == 2) && (argv[1][0]=='c'))) {
		printf("usage: './dialer' for x11 './dialer c' for cli\n");
	}

	if ((argc == 2) && (argv[1][0]=='c')) {
		printf ("write a couple of keys [0-9], *, # and end the sequence with enter\n");
		setbuf(stdin, NULL);
		setbuf(stdout, NULL);
		while ((ch = getc(stdin)) != 27) {
			switch (ch) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					ch -= 48;
					dtmf(ch, 220);
					break;
				case '#':
					dtmf(11, 220);
					break;
				case '*':
					dtmf(10, 220);
					break;
			}
			
		}
		return 0;
	}
 XEvent Event;

 XWMHints *wmhints;
 XSizeHints *sizehints;
 XClassHint classhint;
 XTextProperty winname;
 XTextProperty iconname;
 int j,sfd;
 
 dispname=NULL;

 if(!(display = XOpenDisplay(dispname))) {
  fprintf(stderr, "Unable to open display '%s', dipshit!\n",dispname);
  exit(EXIT_FAILURE);
 }

 screen = DefaultScreen(display);
 rootwin = DefaultRootWindow(display);
 depth = DefaultDepth(display, screen);
 maingc = XCreateGC(display, rootwin, 0, 0);

 XpmCreatePixmapFromData(display,rootwin,main_xpm,&mainpanel,NULL,NULL);

 dialer = XCreatePixmap(display, rootwin, 117, 116, depth);
 XCopyArea(display,mainpanel,dialer,maingc,0,0,117,116,0,0);

 win=XCreateSimpleWindow(display,rootwin,5,5,117,116,1,0,0);
 XSelectInput(display, win, KeyPressMask | KeyReleaseMask | ButtonPressMask | ExposureMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);
 XSetWindowBackgroundPixmap(display, win, dialer);

 classhint.res_name="dialer"; classhint.res_class="DIALER";
 XSetClassHint(display, win, &classhint);
 XStoreName(display, win, "Dialer");
 XSetIconName(display, win, "dialer");
 XMapWindow(display,win);
 XFlush(display);

 while(1) {
  while (XPending(display)) {
   XNextEvent(display, &Event);
   switch (Event.type) {
    case Expose:
     XFlush(display);
    break;
    case DestroyNotify:
     XCloseDisplay(display);
     exit(0);
    break;
    case ButtonPress:
     pressEvent(&Event.xbutton);
    break;
    case ButtonRelease:
     releaseEvent(&Event.xbutton);
    break;
    case KeyPress:
     keyEvent(&Event.xkey);
    case KeyRelease:
     XClearArea(display,win,0,0,116,117,0);
    break;
   }
  }
  usleep(100000);
 }
 exit(0);
}

void releaseEvent(XButtonEvent *button)
{
 dragging=0;
 XClearArea(display,win,0,0,116,117,0);
}

void keyEvent(XKeyEvent *vector)
{
 int kc=vector->keycode;
 switch(kc) {
 case 10: draw_switches(1,1); break;
 case 11: draw_switches(1,2); break;
 case 12: draw_switches(1,3); break;
 case 13: draw_switches(1,4); break;
 case 14: draw_switches(1,5); break;
 case 15: draw_switches(1,6); break;
 case 16: draw_switches(1,7); break;
 case 17: draw_switches(1,8); break;
 case 18: draw_switches(1,9); break;
 case 19: draw_switches(1,0); break;
 case 115: draw_switches(1,10); break;
 case 117: draw_switches(1,11); break;
 default: dtmf(kc-9, 200); break;
 }
}

void draw_switches(int state,int type)
{
 XImage *temp;
 XImage *btn;

 btn=XGetImage(display,mainpanel,122,8,101,100,AllPlanes,ZPixmap);

switch (type) {
  case 0:
   XPutImage(display,win,maingc,btn,34,75,42,83,33,25);
  break;
  case 1:
   XPutImage(display,win,maingc,btn,0,0,8,8,33,25);
  break;
  case 2:                                               
   XPutImage(display,win,maingc,btn,34,0,42,8,33,25);
  break;                
  case 3:
   XPutImage(display,win,maingc,btn,68,0,76,8,33,25);
  break;
  case 4:
   XPutImage(display,win,maingc,btn,0,25,8,33,33,25);
  break;
  case 5:
   XPutImage(display,win,maingc,btn,34,25,42,33,33,25);
  break;
  case 6:
   XPutImage(display,win,maingc,btn,68,25,76,33,33,25);
  break;
  case 7:
   XPutImage(display,win,maingc,btn,0,50,8,58,33,25);
  break;
  case 8:
   XPutImage(display,win,maingc,btn,34,50,42,58,33,25);
  break;
  case 9:
   XPutImage(display,win,maingc,btn,68,50,76,58,33,25);
  break;
  case 10:
   XPutImage(display,win,maingc,btn,0,75,8,83,33,25);
  break;
  case 11:
   XPutImage(display,win,maingc,btn,68,75,76,83,33,25);
  break;
 }	 
 // silence(sound_fd,50);
 XFlush(display);
 dtmf(type, 220);
 // silence(sound_fd,100);
}

int checkreg(x,x1,y,y1,posx,posy)
{
 if (posx>=x && posx<=x1 && posy>=y && posy<=y1)
  return 1;
 else
  return 0;
}

void pressEvent(XButtonEvent *button)
{
 int x=button->x;
 int y=button->y;
 int i;
 dragging=1;
 
 if ( checkreg(42,75,83,108,x,y) ) // "0"
  draw_switches(1,0);
 if ( checkreg(9,41,9,33,x,y) ) // "1"
  draw_switches(1,1);
 if ( checkreg(42,75,9,33,x,y) ) // "2"
  draw_switches(1,2);
 if ( checkreg(76,109,9,33,x,y) ) // "3"
  draw_switches(1,3);
 if ( checkreg(9,41,33,58,x,y) ) // "4"
  draw_switches(1,4);
 if ( checkreg(42,75,33,58,x,y) ) // "5"
  draw_switches(1,5);
 if ( checkreg(76,109,33,58,x,y) ) // "6"
  draw_switches(1,6);
 if ( checkreg(9,41,58,83,x,y) ) // "7"
  draw_switches(1,7);
 if ( checkreg(42,75,58,83,x,y) ) // "8"
  draw_switches(1,8);
 if ( checkreg(76,109,58,83,x,y) ) // "9"
  draw_switches(1,9);
 if ( checkreg(9,41,83,108,x,y) ) // "*"
  draw_switches(1,10);
 if ( checkreg(76,109,83,108,x,y) ) // "#"
  draw_switches(1,11);
}
