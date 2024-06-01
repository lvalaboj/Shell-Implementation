#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>

#include <pwd.h> // Include for getpwnam
#include <unistd.h> // Include for other UNIX/Linux specific functions
#include <cstdlib>
#include "SimpleCommand.hh"


//C++
#include <string>
#include <algorithm>
#include <iterator>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "y.tab.hh"

//C
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

SimpleCommand::SimpleCommand() {
  _arguments = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
  for (auto & arg : _arguments) {
    delete arg;
  }
}




void SimpleCommand::insertArgument( std::string * argument ) {
  // Check if the argument starts with '~'
  if ((*argument)[0] == '~') {
    // Check for a simple "~" or "~/", which indicates the HOME directory
    if (argument->length() == 1 || ((*argument)[1] == '/' && argument->length() > 1)) {
        *argument = std::string(getenv("HOME")) + (argument->length() > 1 ? argument->substr(1) : "");
    } else {
        // Handle cases where the path includes a username (e.g., ~username/path)
        size_t firstSlashPosition = argument->find('/');
        std::string username = argument->substr(1, firstSlashPosition - 1);
        struct passwd *userRecord = getpwnam(username.c_str());

        if (!userRecord) {
          return;
        }

        // Construct the new path using the user's home directory
        *argument = std::string(userRecord->pw_dir) + (firstSlashPosition != std::string::npos ? argument->substr(firstSlashPosition) : "");
    }
  }
  _arguments.push_back(argument);
}

// Print out the simple command
void SimpleCommand::print() {
  for (auto & arg : _arguments) {
    std::cout << "\"" << *arg << "\" \t";
  }
  std::cout << std::endl;
}

void SimpleCommand::clear() {
  for (auto & arg : _arguments) {
    delete arg;
  }
  _arguments.clear();
}
