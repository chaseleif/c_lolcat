#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

struct {
  int flags;
  int os;
  float spread;
  float freq;
  int seed;
  int duration;
  float speed;
}opts;
#define animate(o)      (o.flags&0x1)
#define invert(o)       (o.flags&0x2)
#define truecolor(o)    (o.flags&0x4)
#define force(o)        (o.flags&0x8)
#define setanimate(o)   (o.flags|=0x1)
#define setinvert(o)    (o.flags|=0x2)
#define settruecolor(o) (o.flags|=0x4)
#define setforce(o)     (o.flags|=0x8)

void (*println)(char*,int*);

void animationpause(const double seconds) {
  struct timespec ts;
  double sec_d = floor(seconds);
  ts.tv_nsec = (long)round((seconds - sec_d) * 1e9);
  ts.tv_sec = (time_t)sec_d;
  if (ts.tv_nsec >= 1000000000L) {
    ++ts.tv_sec;
    ts.tv_nsec -= 1000000000L;
  }
  while (nanosleep(&ts, NULL) == -1) { }
}

void rainbow(int i, int *r, int *g, int *b) {
  *r = (int)(sin(opts.freq * i + 0) * 127 + 128);
  *g = (int)(sin(opts.freq * i + 2 * M_PI / 3) * 127 + 128);
  *b = (int)(sin(opts.freq * i + 4 * M_PI / 3) * 127 + 128);
}

int is_stdout_tty() {
  return isatty(STDOUT_FILENO);
}

void hide_cursor() {
  printf("\x1b[?25l");
  fflush(stdout);
}

void show_cursor() {
  printf("\x1b[?25h");
  fflush(stdout);
}

void println_plain(char *str, int *os) {
  const int len = (int)strlen(str);
  for (int i=0,nspaces=0,tablen=0;i<len+tablen;++i) {
    int r, g, b;
    rainbow((*os) + i / opts.spread, &r, &g, &b);
    if (!nspaces && str[i-tablen]=='\t') {
      tablen += 7;
      nspaces = 8;
    }
    const char c = (nspaces)?' ':str[i-tablen];
    if (nspaces) --nspaces;
    if (truecolor(opts)) {
      if (invert(opts)) {
        printf("\x1b[48;2;%d;%d;%dm%c", r, g, b, c);
      }
      else {
        printf("\x1b[38;2;%d;%d;%dm%c", r, g, b, c);
      }
    }
    else {
      r = (r<48)?0
          :(r<114)?1
          :(r-35)/40;
      g = (g<48)?0
          :(g<114)?1
          :(g-35)/40;
      b = (b<48)?0
          :(b<114)?1
          :(b-35)/40;
      int color = 16 + (36*r) + (6*g) + b;
      if (color == 16) ++color;
      if (invert(opts))
        printf("\x1b[48;5;%dm%c", color, c);
      else
        printf("\x1b[38;5;%dm%c", color, c);
    }
  }

  *os += len / opts.spread;
  if (*os > 256) *os -= 256;
}

// Print animated line
void println_ani(char *str, int *os) {
  const int len = strlen(str);
  int newline=0;
  if (len > 0 && str[len-1] == '\n') {
    if (len==1) {
      printf("\n");
      return;
    }
    newline=1;
    str[len-1]='\0';
  }
  if (len-newline > 0) {
    printf("\x1b[s");
    println_plain(str, os);
    const int real_os = *os;
    fflush(stdout);
    animationpause(1.0 / opts.speed);
    for (int i=1;i<opts.duration;++i) {
      printf("\x1b[u");
      *os += opts.spread;
      println_plain(str, os);
      fflush(stdout);
      animationpause(1.0 / opts.speed);
    }
    *os = real_os;
  }
  if (newline) printf("\n");
}

void cat(FILE *infile) {
  char line[512];
  while (1) {
    if (!fgets(line, sizeof(line), infile)) break;
    println(line, &opts.os);
    if (invert(opts))
      printf("\x1b[49m");
    else
      printf("\x1b[39m");
    fflush(stdout);
  }

  if (is_stdout_tty()) {
    printf("\x1b[m\x1b[?25h\x1b[?1;5;2004l");
    fflush(stdout);
  }
}

void print_usage() {
  if (animate(opts)) hide_cursor();
  if (!opts.seed) srand(time(NULL));
  else srand(opts.seed);
  opts.os = rand()%256;
  char *usage[] = {
  "",
  "Usage: lolcat [OPTION]... [FILE]...",
  "",
  "Concatenate FILE(s), or standard input, to standard output",
  "With no FILE, or when FILE is -, read standard input.",
  "",
  "  -p, --spread=<f>      Rainbow spread (default: 3.0)",
  "  -F, --freq=<f>        Rainbow frequency (default: 0.1)",
  "  -S, --seed=<i>        Rainbow seed, 0 = random (default: 0)",
  "  -a, --animate         Enable psychedelics",
  "  -d, --duration=<i>    Animation duration (default: 12)",
  "  -s, --speed=<f>       Animation speed (default: 20.0)",
  "  -i, --invert          Invert fg and bg",
  "  -t, --truecolor       24-bit (truecolor)",
  "  -f, --force           Force color even when stdout is not a tty",
  "  -v, --version         Print version and exit",
  "  -h, --help            Show this message",
  "",
  "Examples:",
  "  lolcat f - g      Output f\'s contents, then stdin, then g\'s contents",
  "  lolcat            Copy standard input to standard output.",
  "  fortune | lolcat  Display a rainbow cookie.",
  "",
  "Report lolcat bugs to <https://github.com/chaseleif/lolcat/issues>",
  "lolcat home page:     <https://github.com/chaseleif/lolcat>",
  NULL
  };
  for (int i=0;usage[i];++i) {
    println(usage[i], &opts.os);
    printf("\n");
  }
  if (invert(opts))
    printf("\x1b[49m\n");
  else
    printf("\x1b[39m\n");
  if (animate(opts)) show_cursor();
}

void print_version() {
  printf("lolcat v0 (c)2026 chase@chaseleif.tech\n");
}

void bad_argument(char *arg) {
  fprintf(stderr,"Error: unknown argument \'%s\'.\n",arg);
  fprintf(stderr,"Try --help for help.\n");
}

int main(int argc,char **argv) {
  println = &println_plain;
  opts.flags=0;
  opts.os=0;
  opts.spread=3.0F;
  opts.freq=0.1F;
  opts.seed=0;
  opts.duration=12;
  opts.speed=20.0F;
  int i=1;
  for ( ;i<argc;++i) {
    if (argv[i][0] != '-') {
      break;
    }
    if (argv[i][1] && argv[i][2] == '\0') {
      if (argv[i][1] == '-') {
        ++i;
        break;
      }
      switch(argv[i][1]) {
        case 'p':
          if (++i < argc) opts.spread = atof(argv[i]);
          break;
        case 'F':
          if (++i < argc) opts.freq = atof(argv[i]);
          break;
        case 'S':
          if (++i < argc) opts.seed = atoi(argv[i]);
          break;
        case 'a':
          setanimate(opts);
          println = &println_ani;
          break;
        case 'd':
          if (++i < argc) opts.duration = atoi(argv[i]);
          break;
        case 's':
          if (++i < argc) opts.speed = atof(argv[i]);
          break;
        case 'i':
          setinvert(opts);
          break;
        case 't':
          settruecolor(opts);
          break;
        case 'f':
          setforce(opts);
          break;
        case 'v':
          print_version();
          return EXIT_SUCCESS;
        case 'h':
          print_usage();
          return EXIT_SUCCESS;
        default:
          bad_argument(argv[i]);
          return EXIT_FAILURE;
      }
      continue;
    }
    if (argv[i][1] != '-') {
      bad_argument(argv[i]);
      return EXIT_FAILURE;
    }
    argv[i]+=2;
    char *equals = strchr(argv[i], '=');
    if (equals) *equals = '\0';
    if (!strcmp(argv[i], "spread")) {
      if (equals) opts.spread = atof(equals+1);
      else if (++i < argc) opts.spread = atof(argv[i]);
    }
    else if (!strcmp(argv[i], "freq")) {
      if (equals) opts.freq = atof(equals+1);
      else if (++i < argc) opts.freq = atof(argv[i]);
    }
    else if (!strcmp(argv[i], "seed")) {
      if (equals) opts.seed = atoi(equals+1);
      else if (++i < argc) opts.seed = atoi(argv[i]);
    }
    else if (!strcmp(argv[i], "animate")) {
      setanimate(opts);
      println = &println_ani;
    }
    else if (!strcmp(argv[i], "duration")) {
      if (equals) opts.duration = atoi(equals+1);
      else if (++i < argc) opts.duration = atoi(argv[i]);
    }
    else if (!strcmp(argv[i], "speed")) {
      if (equals) opts.speed = atof(equals+1);
      else if (++i < argc) opts.speed = atof(argv[i]);
    }
    else if (!strcmp(argv[i], "invert")) {
      setinvert(opts);
    }
    else if (!strcmp(argv[i], "truecolor")) {
      settruecolor(opts);
    }
    else if (!strcmp(argv[i], "force")) {
      setforce(opts);
    }
    else if (!strcmp(argv[i], "version")) {
      print_version();
      return EXIT_SUCCESS;
    }
    else if (!strcmp(argv[i], "help")) {
      print_usage();
      return EXIT_SUCCESS;
    }
    else {
      bad_argument(argv[i]-2);
      return EXIT_FAILURE;
    }
  }
  if (!opts.seed) srand(time(NULL));
  else srand(opts.seed);
  opts.os = rand()%256;
  if (animate(opts)) hide_cursor();
  if (i==argc)
    cat(stdin);
  else for ( ;i<argc;++i) {
    if (argv[i][0]=='-' && argv[i][1]=='\0') cat(stdin);
    else {
      FILE *infile = fopen(argv[i],"r");
      if (!infile) {
        fprintf(stderr, "open(%s): %s (errno %d)\n",
                argv[i], strerror(errno), errno);
      }
      else {
        cat(infile);
        fclose(infile);
      }
    }
  }
  if (animate(opts)) show_cursor();
  return EXIT_SUCCESS;
}
