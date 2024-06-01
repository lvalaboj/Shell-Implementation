
#include <unistd.h>
#include <cstdio>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <cstdio>
#include <unistd.h>
#include "Command.hh"
#include "Shell.hh"
bool Shell::_srcCmd = false;
int yyparse(void);
void yyrestart(FILE * file);
extern int yylex_destroy();

#ifndef YY_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k.
 * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
 * Ditto for the __ia64__ case accordingly.
 */
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

void yyrestart(FILE * input_file );
int yyparse(void);
void yypush_buffer_state(YY_BUFFER_STATE buffer);
void yypop_buffer_state();
YY_BUFFER_STATE yy_create_buffer(FILE * file, int size);


Shell * Shell::TheShell;

Shell::Shell() {
    this->_level = 0;
    this->_enablePrompt = true;
    this->_listCommands = new ListCommands(); 
    this->_simpleCommand = new SimpleCommand();
    this->_pipeCommand = new PipeCommand();
    this->_currentCommand = this->_pipeCommand;
    if ( !isatty(0)) {
	this->_enablePrompt = false;
    }
}

void Shell::prompt() {
  if ((isatty(0)) && (_enablePrompt == true)) {
    printf("myshell>");
    fflush(stdout);
  }
  fflush(stdout);
}

void Shell::print() {
    printf("\n--------------- Command Table ---------------\n");
    this->_listCommands->print();
}

void Shell::clear() {
    this->_listCommands->clear();
    this->_simpleCommand->clear();
    this->_pipeCommand->clear();
    this->_currentCommand->clear();
    this->_level = 0;
}

void Shell::execute() {
  if (this->_level == 0 ) {
    this->_listCommands->execute();
    this->_listCommands->clear();
    this->prompt();
  }
}


/* Control C */
extern "C" void ctrlC(int sig) {
  printf("\n");
  Shell::TheShell->prompt();
}

void yyset_in (FILE *  in_str);

int 
main(int argc, char **argv) {
  /* Ctrl-c */
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = ctrlC;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }


  char * input_file = NULL;
  if ( argc > 1 ) {
    input_file = argv[1];
    FILE * f = fopen(input_file, "r");
    if (f==NULL) {
        fprintf(stderr, "Cannot open file %s\n", input_file);
        perror("fopen");
        exit(1);
    }
    yyset_in(f);
  }

  Shell::TheShell = new Shell();

  /* 3.3.4 Argument Env Variables */

  char argNum[10];
  snprintf(argNum, 10, "%d", argc - 2); // argc - 1 because argv[0] is the program name

  setenv("#", argNum, 1);

  for (int i = 1; i < argc; ++i) {
    // Construct the name of the variable (1, 2, 3, ...)
    char varName[3];
    snprintf(varName, 3, "%d", i-1);

    // Set the variable with the argument value
    setenv(varName, argv[i], 1);
  }


  if (input_file != NULL) {
    Shell::TheShell->_enablePrompt = false;
  }
  else {
    Shell::TheShell->prompt();
  }

  yyparse();

}

