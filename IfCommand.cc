

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Command.hh"
#include "SimpleCommand.hh"
#include "IfCommand.hh"
#include <set>
#include <ostream>
#include <iostream>
#include "PipeCommand.hh"


IfCommand::IfCommand() {
    _condition = NULL;
    _listCommands =  NULL;
}


int IfCommand::runTest(SimpleCommand *condition) {
    std::vector<std::string*> arguments = condition->_arguments;
    std::vector<char*> argsForC;

    // Convert the arguments for C compatibility
    argsForC.push_back(const_cast<char*>("test")); // The command name itself as the first argument
    for (auto &arg : arguments) {
        //printf("Command %s\n", arg->c_str());
        argsForC.push_back(const_cast<char*>(arg->c_str()));
    }
    argsForC.push_back(nullptr); // null-terminate the argument list

    pid_t pid = fork();
    if (pid == -1) {
        // Handle error
        perror("fork");
        return 1; // Indicate error
    } else if (pid == 0) {
        // Child process: execute the test command
        execvp("test", argsForC.data());
        // If execvp returns, it failed
        perror("execvp");
        exit(EXIT_FAILURE); // Use a distinct error code if you like
    } else {
        // Parent process: wait for the child to finish
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            // The test command returns 0 for true (success), which is Unix convention

          return WEXITSTATUS(status);
        } else {
            // Handle cases where the child process didn't exit normally
            return 1; // Indicate error
        }
    }
}




void 
IfCommand::insertCondition( SimpleCommand * condition ) {
    _condition = condition;
}

void 
IfCommand::insertListCommands( ListCommands * listCommands) {
    _listCommands = listCommands;
}

void 
IfCommand::clear() {

}

void 
IfCommand::print() {
    printf("IF [ \n"); 
    this->_condition->print();
    printf("   ]; then\n");
    this->_listCommands->print();
}

void IfCommand::execute() {
  if (runTest(this->_condition) == 0) {
    _listCommands->execute();
    return;
  }

}
