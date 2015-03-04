/* @(#)xsetprop.c
 * Copyright 2012 Constantin Kulikov
 *
 * Author: Constantin Kulikov (Bad_ptr) <zxnotdead@gmail.com>
 * Date: 2013/07/09 21:11:44
 * License: GPL either version 2 or any later version
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <getopt.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>


#define CONCAT_S_N(x,y) x #y
#define EVAL_CONCAT_S_V(x,y)  CONCAT_S_N(x,y)


#define MAXELEMENTS 64

static int verbose_flag = 0;
static int remap_flag = 0;
static int id_flag = 0;

static struct option long_opts[] =
  {
    {"verbose",  no_argument,       &verbose_flag, 1},
    {"remap",    no_argument,       &remap_flag, 1},
    {"help",     no_argument,       NULL, 'h'},
    {"id",       optional_argument, NULL, 'i'},
    {"propname", required_argument, NULL, 'p'},
    {"value",    required_argument, NULL, 'v'},
    {"format",   required_argument, NULL, 'f'},
    {"mode",     required_argument, NULL, 'm'},
    {"atom",     required_argument, NULL, 'a'},
    {"string",   required_argument, NULL, 's'},
    {0, 0, 0, 0}
  };

static const char * opt_str = "hi:p:v:f:m:";

void help()
{
  printf("%s", "Usage: xsetprop [--id=[window_id]] (--format=<32a> --propname=<WM_ICON_NAME>|--atom <ATOMNAME>|--string <PROP_NAME>) --value=<value> [--mode=[replace|append|prepend]] [--remap]\
\n --id or -i (optional): window id, which you can get from xwininfo. If omitted -- you will be prompted to select a window with mouse.\
\n --format or -f : format of a property. 32a for atoms, 8s for strings, etc(See man xprop).\
\n --propname or -p : name of a property you want to set.\
\n --value or -v : A new value or a value to append/prepend of property(see --mode option).\
\n --mode or -m (optional): 'replace' or no specify for discarding the old value, append/prepend to append/prepend to the old value.\
\n --remap (optional): if this flag is specified the window will be unmapped and then mapped again after changing the propertyes(It helps WMs to see changes).\
\nA shorter form: xsetprop [--id=[window_id]] (--atom <ATOMNAME>|--string <PROP_NAME>) [--value <VALUE>], (you don't need to specify --format and --propname).\n");
  fflush(stdout);
}

size_t skip_seps( const char * _str, const char * _seps )
{
  size_t ret = 0;
  while( *_str != '\0' )
    {
      if( strchr( _seps, *_str ) == NULL )
        break;

      _str += 1;
      ret += 1;
    }
  return ret;
}


char ** split_str( char * _str, const char * _seps )
{
  size_t
    len = strlen( _str ),
    n_ret = 0;

  char ** ret = malloc(sizeof(char*) * 8);

  char * tptr = _str;

  while( *tptr != '\0' )
    {
      size_t tmp = skip_seps( tptr, _seps );
      tptr += tmp;
      len -= tmp;

      if( len == 0 )
        {
          ret[n_ret] = NULL;
          return ret;
        }

      tmp = strcspn( tptr, _seps );

      ret[n_ret] = malloc(sizeof(char)*(tmp+1));
      memcpy(ret[n_ret], tptr, tmp);
      ret[n_ret][tmp] = '\0';
      n_ret += 1;

      if( n_ret % 8 == 0 )
        {
          ret = realloc(ret, n_ret + 8);
        }

      len -= tmp;

      tptr += tmp;
    }

  ret[n_ret] = NULL;
  return ret;
}

void free_splits( char ** sa )
{
  if( !sa )
    return;

  void * r = sa;
  while(*sa != NULL)
    {
      free(*sa);
      sa+=1;
    }
  free(r);
}

void print_splits( char ** sa )
{
  if( !sa )
    return;

  while(*sa != NULL)
    {
      printf("%s\n", *sa);
      sa+=1;
    }
}

size_t splits_num( char **sa )
{
  size_t ret = 0;

  if( !sa )
    return ret;

  while( sa[ret] != NULL )
    {
      ret+=1;
    }

  return ret;
}

Atom * splits_to_atomsarray( Display * _dpy, char ** sa )
{
  if( !sa )
    return NULL;

  size_t
    n = splits_num( sa ),
    i = 0;

  Atom * ret = malloc(sizeof(Atom)*n);

  while( i < n )
    {
      ret[i] = XInternAtom(_dpy, sa[i], 0);
      i+=1;
    }

  return ret;
}

void free_atomsarray( Atom * aa )
{
  free(aa);
}


void FatError( const char * _str )
{
  fprintf(stderr, "[Fatal Error] %s\n", _str);
  fflush(stderr);
  exit(1);
}

void Warning( const char * _str )
{
  fprintf(stderr, "[Warning] %s\n", _str);
  fflush(stderr);
}


void set_property( Display * _dpy, Window _wid, const char * _format_str,
                   const char * _prop_name_str, char * _prop_value_str, int _mode )
{
  int s = 0;
  char c = '\0';
  sscanf( _format_str, "%d%c", &s, &c );


  Atom prop = XInternAtom(_dpy, _prop_name_str, 0);
  Atom type = XA_ATOM; // XA_STRING XA_CARDINAL XA_INTEGER XA_ATOM textprop.encoding
  int format = 32; // size

  unsigned char * data = NULL;
  int nelements = 0;


  if( s != 0 )
    format = s;

  if( c != '\0' )
    switch( c ) /* This part is mostly copied from the xprop :p */
      {
      case 's':
        {
          if( format != 8 )
            FatError("the 'size' must be 8 for the 's'(string type), check the '--format' commandline parameter;");

          type = XA_STRING;
          data = (unsigned char *)_prop_value_str;
          nelements = strlen(_prop_value_str);

          break;
        }

      case 't':
        {
          if( format != 8 )
            FatError("the 'size' must be 8 for the 't'(text type), check the '--format' commandline parameter;");

          XTextProperty tproperty;

          if( XmbTextListToTextProperty(_dpy, &_prop_value_str, 1,
                                        XStdICCTextStyle, &tproperty) != Success )
            {
              char errstr[1024];
              snprintf(errstr, 1023,
                       "cannot use the <%s> value as the 'STRING' or the 'COMPOUND_TEXT' type.",
                       _prop_value_str);
              FatError(errstr);
            }

          type = tproperty.encoding;
          data = tproperty.value;
          nelements = tproperty.nitems;

          break;
        }

      case 'x':
      case 'c':
        {
          static unsigned char data8[MAXELEMENTS];
          static unsigned short data16[MAXELEMENTS];
          static unsigned long data32[MAXELEMENTS];
          unsigned long intvalue;
          char * value2 = strdup(_prop_value_str);
          char * tmp = strtok(value2,",");
          nelements = 1;
          intvalue = strtoul(tmp, NULL, 0);
          switch(format)
            {
            case 8:
              data8[0] = intvalue; data = (unsigned char *) data8; break;
            case 16:
              data16[0] = intvalue; data = (unsigned char *) data16; break;
            case 32:
              data32[0] = intvalue; data = (unsigned char *) data32; break;
            }
          tmp = strtok(NULL,",");
          while(tmp != NULL)
            {
              intvalue = strtoul(tmp, NULL,0);
              switch(format)
                {
                case 8:
                  data8[nelements] = intvalue; break;
                case 16:
                  data16[nelements] = intvalue; break;
                case 32:
                  data32[nelements] = intvalue; break;
                }
              nelements++;
              if(nelements == MAXELEMENTS)
                {
                  Warning( EVAL_CONCAT_S_V("the maximum number of elements(", MAXELEMENTS)
                           ") is reached -- the value list will be truncated.");
                  break;
                }
              tmp = strtok(NULL,",");
            }

          type = XA_CARDINAL;
          free(value2);
          break;
        }

      case 'i':
        {
          static unsigned char data8[MAXELEMENTS];
          static unsigned short data16[MAXELEMENTS];
          static unsigned long data32[MAXELEMENTS];
          unsigned long intvalue;
          char * value2 = strdup(_prop_value_str);
          char * tmp = strtok(value2,",");
          nelements = 1;
          intvalue = strtoul(tmp, NULL, 0);
          switch(format)
            {
            case 8:
              data8[0] = intvalue; data = (unsigned char *) data8; break;
            case 16:
              data16[0] = intvalue; data = (unsigned char *) data16; break;
            case 32:
              data32[0] = intvalue; data = (unsigned char *) data32; break;
            }
          tmp = strtok(NULL,",");
          while(tmp != NULL)
            {
              intvalue = strtoul(tmp, NULL,0);
              switch(format)
                {
                case 8:
                  data8[nelements] = intvalue; break;
                case 16:
                  data16[nelements] = intvalue; break;
                case 32:
                  data32[nelements] = intvalue; break;
                }
              nelements++;
              if(nelements == MAXELEMENTS)
                {
                  Warning( EVAL_CONCAT_S_V("the maximum number of elements(", MAXELEMENTS)
                           ") is reached -- the value list will be truncated.");
                  break;
                }
              tmp = strtok(NULL,",");
            }

          type = XA_INTEGER;
          free(value2);
          break;
        }
      case 'b':
        {
          unsigned long boolvalue;
          static unsigned char data8;
          static unsigned short data16;
          static unsigned long data32;

          if (!strcmp(_prop_value_str, "True"))
            {
              boolvalue = 1;
            }
          else if (!strcmp(_prop_value_str, "False"))
            {
              boolvalue = 0;
            }
          else
            {
              char errstr[256];
              snprintf(errstr, 255, "can not use the <%s> value as the 'boolean' type.", _prop_value_str);
              FatError(errstr);
              return;
            }
          type = XA_INTEGER;

          switch (format)
            {
            case 8:
              data8 = boolvalue; data = (unsigned char *) &data8; break;
            case 16:
              data16 = boolvalue; data = (unsigned char *) &data16; break;
            case 32: default:
              data32 = boolvalue; data = (unsigned char *) &data32; break;
            }
          nelements = 1;
          break;
        }

      case 'a':
        {
          char ** sa = split_str(_prop_value_str, ", ");
          nelements = splits_num( sa );
          Atom * aa = splits_to_atomsarray(_dpy, sa);
          free_splits(sa);
          type = XA_ATOM;
          data = (unsigned char*)aa;
          break;
        }

      case 'm':

      default:
        {
          char errstr[256];
          snprintf(errstr, 255, "The wrong '--format' option: %s", _format_str);
          FatError(errstr);
        }
      }

  XChangeProperty(_dpy, _wid, prop, type, format, _mode, data, nelements);

}

Window select_window(Display * dpy, int screen)
{
  int status;
  Cursor cursor;
  XEvent event;
  Window target_win = None, root = RootWindow(dpy,screen);
  int buttons = 0;

  /* Make the target cursor */
  cursor = XCreateFontCursor(dpy, XC_crosshair);

  /* Grab the pointer using target cursor, letting it room all over */
  status = XGrabPointer(dpy, root, False,
                        ButtonPressMask|ButtonReleaseMask, GrabModeSync,
                        GrabModeAsync, root, cursor, CurrentTime);

  if (status != GrabSuccess)
    FatError("can't grab the mouse.");

  /* Let the user select a window... */
  while ((target_win == None) || (buttons != 0))
    {
      /* allow one more event */
      XAllowEvents(dpy, SyncPointer, CurrentTime);
      XWindowEvent(dpy, root, ButtonPressMask|ButtonReleaseMask, &event);
      switch (event.type)
        {
        case ButtonPress:
          if (target_win == None)
            {
              target_win = event.xbutton.subwindow; /* window selected */
              if (target_win == None)
                target_win = root;
            }
          buttons++;
          break;
        case ButtonRelease:
          if (buttons > 0) /* there may have been some down before we started */
            buttons--;
          break;
        }
    }

  XUngrabPointer(dpy, CurrentTime);/* Done with pointer */

  return target_win;
}


int main (int argc, char **argv)
{
  Window window_id = None;
  char * prop_name_str = NULL;
  char * prop_value_str = NULL;
  char * format_str = NULL;
  char * mode_str = NULL;

  int mode = PropModeReplace; // PropModeReplace, PropModePrepend, PropModeAppend


  int lindex;
  int opt;
  while( -1 != (opt = getopt_long_only( argc, argv, opt_str, long_opts, &lindex)) )
    {
      switch( opt )
        {
        case 'h':
          help();
          return 0;
          break;
        case 'i':
          if (optarg && strnlen(optarg, 1) > 0)
            {
              window_id = strtoul(optarg, NULL, 0);
            }
          else
            {
              id_flag = 1;
            }
          break;
        case 'p':
          if (optarg)
            prop_name_str = optarg;
          break;
        case 'v':
          if (optarg)
            prop_value_str = optarg;
          break;
        case 'f':
          if(optarg)
            format_str = optarg;
          break;
        case 'm':
          if( optarg )
            {
              mode_str = optarg;
              if( strncmp( mode_str, "append", strnlen("append", strnlen(mode_str,8)) ) == 0 )
                {
                  mode = PropModeAppend;
                }
              else if( strncmp( mode_str, "prepend", strnlen("prepend", strnlen(mode_str,8)) ) == 0 )
                {
                  mode = PropModePrepend;
                }
            }
          break;
        case 'a':
          if( optarg )
            prop_name_str = optarg;

          format_str = "32a";
          break;
        case 's':
          if( optarg )
            prop_name_str = optarg;

          format_str = "8s";
          break;

        default:
          break;
        }
    }

  if ( NULL == prop_name_str )
    {
      help();
      FatError("you must specify --propname arg;");
    }
  if( NULL == prop_value_str )
    prop_value_str = "\0";

  if( NULL == format_str )
    format_str = "32a";

  Display * dpy = XOpenDisplay(NULL);
  if(!dpy)
    FatError("failed to connect to a display");

  int screen = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);

  if( None == window_id )
    {
      if (1 == id_flag)
        {
          window_id = root;
        }
      else
        {
          printf("%s", "Please select the window you want to set properties for by \
clicking the mouse in that window.\n");
          fflush(stdout);
          window_id = select_window(dpy, screen);
        }
    }
  else
    {
      window_id = XmuClientWindow(dpy, window_id);
    }

  set_property(dpy, window_id, format_str, prop_name_str, prop_value_str, mode);

  if( remap_flag == 1 )
    {
      XUnmapWindow(dpy, window_id);
      XMapWindow(dpy, window_id);
    }

  //XSync(dpy,0);
  XFlush(dpy);
  XCloseDisplay(dpy);

  return 0;
}

