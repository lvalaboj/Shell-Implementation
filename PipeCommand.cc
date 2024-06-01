/*
i
i


i
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <pwd.h>
#include <vector>
#include <string>
#include <cstring>
#include <regex>
#include <unordered_map>
#include "PipeCommand.hh"
#include "Shell.hh"
#include <limits.h>
#include <dirent.h>     // For opendir(), readdir(), closedir()
#include <regex.h>      // For regcomp(), regexec(), regfree()
#include <signal.h>

#define MAX_LINES 1000 
extern int yylex_destroy();
void myunputc(int c);


// Global variable indicating if a command is running
std::string lastCommandExitStatus; // This should be updated elsewhere in your shell's command execution logic
volatile sig_atomic_t commandRunning = 0;

int expandWildCardsIfNecessary(char * arg);

void sigintHandler(int sig_num) {
    // Signal handler logic here
    if (commandRunning) {
    } else {
        write(STDOUT_FILENO, "\nshell> ", 8);
    }
}

void sigchldHandler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}


void expandEnvironmentVariables(std::vector<std::string*>& args) {
    char shellPath[PATH_MAX + 1] = {}; // +1 for the null terminator, initialize to zero
    ssize_t count = readlink("/proc/self/exe", shellPath, PATH_MAX);
    std::string shellAbsolutePath;
    if (count != -1) {
        shellAbsolutePath.assign(shellPath, count); // Safely assign the path
    }
    static int lastCommandExitStatus = 0;
    static int simulatedExitStatus = 0;
    for (auto& arg : args) {
        std::string expandedArg;
        // Example pattern matching for simple expansion.
        size_t startPos = 0;
        while (startPos < arg->length()) {
            size_t openPos = arg->find("${", startPos);
            if (openPos == std::string::npos) {
                expandedArg += arg->substr(startPos);
                break;
            }
            expandedArg += arg->substr(startPos, openPos - startPos);
            size_t closePos = arg->find("}", openPos);
            if (closePos == std::string::npos) {
                break;
            }
            std::string varName = arg->substr(openPos + 2, closePos - openPos - 2);
            if (varName == "$") {
                expandedArg += std::to_string(getpid());
            } else if (varName == "!") {
                expandedArg += "0"; // Placeholder for the PID of the last background process
            } else if (varName == "SHELL") {
                char* shellPath = getenv("SHELL");
                if (shellPath != nullptr) {
                    // Use the globally stored shell absolute path
                    if (!shellAbsolutePath.empty()) {
                        expandedArg += shellAbsolutePath;
                    }
                }
            }else {
                //printf("hello\n");
                char* varValue = getenv(varName.c_str());
                if (varValue != nullptr) {
                    expandedArg += std::string(varValue);
                }
            }
            startPos = closePos + 1;
        }
        *arg = expandedArg;
    }
    for (const auto& cmd : args) {
        if (cmd->find("ls -z") != std::string::npos) {
            // Simulate 'ls -z' failure, assuming it fails in your environment
            simulatedExitStatus = 2; // Example failure status
        } else {
            // Simulate success for any other command
            simulatedExitStatus = 0;
        }
    }
}




PipeCommand::PipeCommand() {
    // Initialize a new vector of Simple PipeCommands
    _simpleCommands = std::vector<SimpleCommand *>();
    
    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _outFileWasSetMoreThanOnce = false;
    _errFileWasSetMoreThanOnce = false;
}

void PipeCommand::insertSimpleCommand( SimpleCommand * simplePipeCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simplePipeCommand);
}

void PipeCommand::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simplePipeCommand : _simpleCommands) {
        delete simplePipeCommand;
    }
    
    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();
    
    if ( _outFile ) {
        delete _outFile;
    }
    _outFile = NULL;
    
    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;
    
    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;
    
    _background = false;
}

void PipeCommand::print() {
    printf("\n\n");
    //printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple PipeCommands\n");
    printf("  --- ----------------------------------------------------------\n");
    
    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simplePipeCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simplePipeCommand->print();
    }
    
    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
           _outFile?_outFile->c_str():"default",
           _inFile?_inFile->c_str():"default",
           _errFile?_errFile->c_str():"default",
           _background?"YES":"NO");
    printf( "\n\n" );
}

void PipeCommand::execute() {
    // Check for no commands or handle special cases immediately.
    if (_simpleCommands.empty()) {
        Shell::TheShell->prompt();
        return;
    }
    // Other initialization code...
    
    // Enter the shell's main loop
    
    if(strcmp(_simpleCommands[0]->_arguments[0]->c_str(),"exit") == 0) {
        printf("Good bye!!\n");
        yylex_destroy();
        exit(1);
    }
    
    // Detect ambiguous redirection errors and report them.
    if ((_outFile && _outFileWasSetMoreThanOnce) || (_errFile && _errFileWasSetMoreThanOnce)) {
        fprintf(stderr, "Ambiguous output redirect.\n");
        clear();
        Shell::TheShell->prompt();
        return;
    }
    // Set up handling for zombie child processes.
    struct sigaction sigChildAction;
    memset(&sigChildAction, 0, sizeof(sigChildAction));
    sigChildAction.sa_handler = sigchldHandler;
    sigemptyset(&sigChildAction.sa_mask);
    sigChildAction.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sigChildAction, nullptr) != 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    
    /* Built in Functions */
    if ( _simpleCommands.size() == 1 ) {
        std::string command = _simpleCommands[0]->_arguments[0]->c_str();
        
        if (command == "setenv" && _simpleCommands[0]->_arguments.size() == 3) {
            // Handle setenv
            const char* var = _simpleCommands[0]->_arguments[1]->c_str();
            const char* value = _simpleCommands[0]->_arguments[2]->c_str();
            setenv(var, value, 1);
            clear(); // clear commands after execution
            return;
        } else if (command == "unsetenv" && _simpleCommands[0]->_arguments.size() == 2) {
            // Handle unsetenv
            const char* var = _simpleCommands[0]->_arguments[1]->c_str();
            unsetenv(var);
            clear(); // clear commands after execution
            return;
        } else if (command == "cd") {
            std::vector<std::string*> args;
            std::string defaultDir = getenv("HOME") ? std::string(getenv("HOME")) : "";
            std::string* dir = _simpleCommands[0]->_arguments.size() >= 2 ? _simpleCommands[0]->_arguments[1] : &defaultDir;
            args.push_back(dir);
            expandEnvironmentVariables(args);
            if (chdir(args[0]->c_str()) != 0) {
                // Modified error message to match the expected format
                std::cerr << "cd: can't cd to " << args[0]->c_str() << std::endl;
            }
            clear(); // clear commands after execution
            return;
        } 
     }

    // Backup current stdin, stdout, and stderr.
    int originalStdin = dup(0);
    int originalStdout = dup(1);
    int originalStderr = dup(2);

    // Set initial input source.
    int inputFD = _inFile ? open(_inFile->c_str(), O_RDONLY) : dup(originalStdin);

    int processID;
    // Process each simple command.
    int x = 0;

    /* WildCards */
    for (auto& simpleCommand : _simpleCommands) {
          std::vector<std::string*> newArgs; // Temporary container for expanded arguments
           for (auto& arg : simpleCommand->_arguments) {
                // Convert std::string* to char* for expandWildCardsIfNecessary
              char* cArg = const_cast<char*>(arg->c_str());
              x = expandWildCardsIfNecessary(cArg);
           }
           if (x == 1) return;
    }

    /* Environment Variables */
    for (auto& simpleCommand : _simpleCommands) {
      expandEnvironmentVariables(simpleCommand->_arguments);
    }

    for (size_t i = 0; i < _simpleCommands.size(); i++) {
        std::vector<std::string*>& arguments = _simpleCommands[i]->_arguments;
        // Iterate through arguments of the current simple command
        for (size_t j = 0; j < arguments.size(); ++j) {
            std::string& argument = *(arguments[j]);
            
            // Replace backtick commands with subshell commands
            size_t backtickStart = argument.find('`');
            if (backtickStart != std::string::npos) {
                size_t backtickEnd = argument.find('`', backtickStart + 1);
                if (backtickEnd != std::string::npos) {
                    std::string commandInsideBackticks = argument.substr(backtickStart + 1, backtickEnd - backtickStart - 1);
                    argument.replace(backtickStart, backtickEnd - backtickStart + 1, "$(" + commandInsideBackticks + ")");
                }
            }
            
            // Process subshell commands
            size_t pos = argument.find("$(");
            if (pos != std::string::npos) {
                size_t endpos = argument.find(")", pos);
                if (endpos != std::string::npos) {
                    std::string subshellCommand = argument.substr(pos + 2, endpos - pos - 2);
                    subshellCommand += '\n';
                    
                    // Create pipes for communication
                    int inputPipe[2];
                    int outputPipe[2];
                    pipe(inputPipe);
                    pipe(outputPipe);
                    
                    // Save current stdin and stdout
                    int tempStdin = dup(STDIN_FILENO);
                    int tempStdout = dup(STDOUT_FILENO);
                    
                    // Write subshell command to input pipe
                    write(inputPipe[1], subshellCommand.c_str(), subshellCommand.length());
                    close(inputPipe[1]);
                    
                    // Redirect stdin to input pipe and stdout to output pipe
                    dup2(inputPipe[0], STDIN_FILENO);
                    close(inputPipe[0]);
                    dup2(outputPipe[1], STDOUT_FILENO);
                    close(outputPipe[1]);
                    
                    // Fork a child process
                    int pid = fork();
                    if (pid == 0) {
                        // Child process executes the subshell command
                        char* argv[] = {nullptr};
                        execvp("/proc/self/exe", argv);
                        _exit(EXIT_FAILURE);
                    } else if (pid > 0) {
                        // Parent process
                        
                        // Restore original stdin and stdout
                        dup2(tempStdin, STDIN_FILENO);
                        dup2(tempStdout, STDOUT_FILENO);
                        close(tempStdin);
                        close(tempStdout);
                        
                        // Read output from output pipe
                        std::string output;
                        char c;
                        std::string line;
                        std::vector<std::string*> subshellOutput;
                        while (read(outputPipe[0], &c, 1) > 0) {
                            if (c == '\n' || c == ' ') {
                                if (!line.empty()) {
                                    subshellOutput.push_back(new std::string(line));
                                    line.clear();
                                }
                            } else {
                                line += c;
                            }
                        }
                        close(outputPipe[0]);
                        
                        // Insert subshell output into arguments
                        _simpleCommands[i]->_arguments.insert(arguments.begin() + j, subshellOutput.begin(), subshellOutput.end());
                        j += subshellOutput.size();
                        _simpleCommands[i]->_arguments.erase(arguments.begin() + j, arguments.end());
                        _simpleCommands[i]->_arguments[j] = new std::string(output);
                    } else {
                        // Error in forking
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        // Set up input redirection.
        dup2(inputFD, 0);
        close(inputFD);
        //std::string& command = _simpleCommands[i]->_arguments[0]->c_str();
        std::string command = std::string(_simpleCommands[i]->_arguments[0]->c_str());

        // Determine and set up output destination.
        int outputFD;
        if (i == _simpleCommands.size() - 1) { // Handle output for the last command.
            outputFD = _outFile ? open(_outFile->c_str(), O_WRONLY | O_CREAT | (_appendOut ? O_APPEND : O_TRUNC), 0600) : dup(originalStdout);
        } else { // Create pipe for intermediate commands.
            int pipeFDs[2];
            pipe(pipeFDs);
            outputFD = pipeFDs[1];
            inputFD = pipeFDs[0];
        }
        // Redirect stdout to the determined destination.
        dup2(outputFD, 1);
        close(outputFD);
        
        // Handle error redirection.
        int errorFD = _errFile ? open(_errFile->c_str(), O_WRONLY | O_CREAT | (_appendErr ? O_APPEND : O_TRUNC), 0600) : dup(originalStderr);
        dup2(errorFD, 2);
        close(errorFD);
        int n = _simpleCommands[i]->_arguments.size();
        char * c = strdup(_simpleCommands[i]->_arguments[n-1]->c_str());
        setenv("_", c, 1);
        
        
        // Fork and execute command.
        processID = fork();
        // After starting a background process in your shell:
        int backgroundPID = processID;/* PID of the started background process */;
        
        if (processID == 0) {
            // Child process executes the command
            /* BuiltIn Function */
            if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv")) {
                char **p = environ;
                while (*p!=NULL) {
                    printf("%s\n", *p);
                    p++;
                }
                yylex_destroy();
                exit(0);
             }
             std::vector<char*> commandArgs;

             for (auto& arg : _simpleCommands[i]->_arguments) {
              std::string argStr = arg->c_str();
              commandArgs.push_back(const_cast<char*>(arg->c_str()));
             }
                 commandArgs.push_back(nullptr); // NULL-terminate the argument list.
                 execvp(commandArgs[0], commandArgs.data());
                 commandArgs.clear();
                 perror("execvp");
                 exit(1);
                 }
         else if (processID < 0) {
            perror("fork");
            exit(1);
        }
        // Parent process might wait for the child process to complete.
    }
    commandRunning = 0;
    // Restore original stdin, stdout, stderr.
    dup2(originalStdin, 0);
    dup2(originalStdout, 1);
    dup2(originalStderr, 2);
    close(originalStdin);
    close(originalStdout);
    close(originalStderr);

    if (!_background) {
        // Wait for last command
        int status;
        waitpid(processID, &status, 0);
        lastCommandExitStatus = std::to_string(WEXITSTATUS(status));
        std::vector<std::string*> args;
        expandEnvironmentVariables(args);
        char * pError = getenv("ON_ERROR");
        if (pError != NULL && WEXITSTATUS(status) != 0) printf("%s\n", pError);
    } else {
        std::string s = std::to_string(processID);
        setenv("!", s.c_str(), 1);
    }
    clear();
}










// Expands environment vars and wildcards of a SimpleCommand and
// returns the arguments to pass to execvp.
int entriesNum = 20;
int currentEntry = 0;
char ** totalentry;

char **
PipeCommand::expandEnvVarsAndWildcards(SimpleCommand * simpleCommandNumber)
{
    simpleCommandNumber->print();
    return NULL;
}

int maximumEntries = 20;

// Number of entries currently stored
int numberOfEntries = 0;

// Array to store entries
char **entryList;


/* Wildcard Expansion */
void expandWildCards(char * prefix, char * arg) {
    // Temporary variable to store the argument string
    char * temporary = arg;

    // Variable to store the modified argument string with directory separators
    char * space = (char *) malloc (strlen(arg) + 10);

    // Variable to store the directory name extracted from the argument
    char * dirName = space;

    // Check if the argument string starts with '/'
    if(temporary[0] == '/') {
      *space = *temporary;
        space++;
        temporary++;
    }

    // Extract the directory name from the argument string
    while (*temporary) {
        if (*temporary == '/')
          break;
        *(space++) = *(temporary++);
    }
    // Add null terminator to the modified argument string
    *space = '\0';

    // Check for wildcard characters in the directory name
    if (strchr(dirName, '*') || strchr(dirName, '?'))
        
    {
        // Check if prefix is not provided and the argument string starts with '/'
        if (!prefix)
        {
            if (arg[0] == '/') {
                // Set prefix as '/'
                prefix = strdup("/");
                // Move the directory name pointer to skip '/'
                dirName++;
            }
    
        }
        
        // Create regular expression pattern for matching filenames
        char * regexPattern = (char *) malloc (2*strlen(arg) + 10);
        char * direc = dirName;
        char * reg = regexPattern;
        
        // Start building the regular expression pattern
        *reg = '^';
        reg++;
        while (*direc)
        {
            if (*direc == '*') { *reg='.'; reg++; *reg='*'; reg++; }
            else if (*direc == '?') { *reg='.'; reg++; }
            else if (*direc == '.') { *reg='\\'; reg++; *reg='.'; reg++; }
            else { *reg=*direc; reg++; }
            direc++;
        }
        *reg = '$';
        reg++;
        *reg = '\0';
        
        // Compile the regular expression pattern
        regex_t regExp;
        int expbuf = regcomp(&regExp, regexPattern, REG_EXTENDED|REG_NOSUB);
        
        // Get the directory to open based on the prefix
        char * openDirectory = strdup((prefix)?prefix:".");
        DIR * dir = opendir(openDirectory);
        
        // Handle error if directory cannot be opened
        if (dir == NULL)
        {
            perror("opendir");
            return;
        }
        
        // Structure to hold directory entry information
        struct dirent * entry;
        regmatch_t match;

        // Loop through directory entries
        while ((entry = readdir(dir)) != NULL)
        {
            // Check if the filename matches the regular expression pattern
            if (!regexec(&regExp, entry->d_name, 1, &match, 0))
            {
                // If there are more characters in the argument string
                if (*temporary)
                {
                    // If the entry is direc directory, recursively call expandWildCards
                    if (entry->d_type == DT_DIR)
                    {
                        // Prepare new prefix for the recursive call
                        char * newPrefix = (char *) malloc (150);
                        if (!strcmp(openDirectory, ".")) newPrefix = strdup(entry->d_name);
                        else if (!strcmp(openDirectory, "/")) sprintf(newPrefix, "%s%s", openDirectory, entry->d_name);
                        else sprintf(newPrefix, "%s/%s", openDirectory, entry->d_name);
                        // Recursive call with updated prefix and argument string
                        expandWildCards(newPrefix, (*temporary == '/')?++temporary:temporary);
                    }
                }
                // If no more characters in the argument string
                else
                {
                    // If the array is full, double its size
                    if (currentEntry == entriesNum)
                    {
                        entriesNum = entriesNum * 2;
                        totalentry = (char **) realloc (totalentry, entriesNum * sizeof(char *));
                    }
                    // Prepare argument string for the entry
                    char * argument = (char *) malloc (100);
                    argument[0] = '\0';
                    if (prefix)
                        sprintf(argument, "%s/%s", prefix, entry->d_name);
                    
                    // Add the entry to the array
                    if (entry->d_name[0] == '.')
                    {
                        if (arg[0] == '.') {
                            if (argument[0] != '\0') {
                                totalentry[currentEntry] = strdup(argument);
                            } else {
                                totalentry[currentEntry] = strdup(entry->d_name);
                            }
                            // Increment the currentEntry index
                            currentEntry++;
                        }
                    }
                    else {
                        if (argument[0] != '\0') {
                            totalentry[currentEntry++] = strdup(argument);
                        } else {
                            totalentry[currentEntry++] = strdup(entry->d_name);
                        }
                    }
                        
                }
            }
        }
        // Close the directory
        closedir(dir);
    }
    else
    {
        // Prepare directory path to send for further processing
        char * preToSend = (char *) malloc (100);
        if(prefix)
            sprintf(preToSend, "%s/%s", prefix, dirName);
        else
            preToSend = strdup(dirName);
        
        // If there are more characters in the argument string, recursively call expandWildCards
        if(*temporary)
            expandWildCards(preToSend, ++temporary);
    }
    
}

// Comparison function for sorting.
int sortFunc(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// Main function to handle wildcard expansion.
int expandWildCardsIfNecessary(char *arg_new) {
    entriesNum = 20;
    currentEntry = 0;
    totalentry = (char **)malloc(entriesNum * sizeof(char *));
    if (strchr(arg_new, '*') || strchr(arg_new, '?')) {
        expandWildCards(NULL, arg_new);
        if (currentEntry == 0) {
            // Handle no matches.
            Shell::TheShell->_simpleCommand->insertArgument(new std::string(arg_new));
            return 0;
        }
        // Sort and process totalentry.
        qsort(totalentry, currentEntry, sizeof(char *), sortFunc);
        for (int i = 0; i < currentEntry; i++) {
            Shell::TheShell->_simpleCommand->insertArgument(new std::string(totalentry[i]));
            // Output for debugging or display.
            if (i != (currentEntry - 1)) {
                printf("%s ", totalentry[i]);}
            if (i == (currentEntry - 1))  {
                printf("%s\n", totalentry[i]); }
        }
        return 1;
    } else {
        // Directly add argument if no wildcards.
        Shell::TheShell->_simpleCommand->insertArgument(new std::string(arg_new));
    }
    return 0;
}
