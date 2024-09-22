#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>

void lsh_loop();
char* lsh_read_line();
char** lsh_split_line(char *line);
int lsh_execute(char **args);

int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

void lsh_loop()
{
  char *line;
  char **args;
  int status;
  char *dir[100];

  do {
   char *cwd =  _getcwd(*dir, 100);
    printf("lsh_shell-> ");
    printf("%s", cwd);
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

#define LSH_RL_BUFSIZE 1024
char *lsh_read_line()
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUFFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char** lsh_split_line(char* line){
  int token_buffer_size = LSH_TOK_BUFFSIZE;
  char** tokens = malloc(sizeof(char*) * token_buffer_size);
  char* token;
  int position = 0;

  if (!tokens){
    fprintf(stderr, "lsh: token allocation error");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL){
    tokens[position] = token;
    position++;

    if (position >= token_buffer_size){
      token_buffer_size += LSH_TOK_BUFFSIZE;
      tokens = realloc(tokens, token_buffer_size);
      if (!tokens){
        fprintf(stderr, "lsh: token allocation error");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;

  return tokens;
}

int lsh_launch(char **args)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD dwExitCode;

    // Initialize memory for the STARTUPINFO structure
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create command line string from args array
    // Windows expects a single string for the command line
    // so we need to concatenate the args into a single string.
    char command[1024] = {0};
    for (int i = 0; args[i] != NULL; i++) {
        strcat(command, args[i]);
        strcat(command, " ");
    }

    // Create a new process
    if (!CreateProcess(
        NULL,         // No module name (use command line)
        command,      // Command line
        NULL,         // Process handle not inheritable
        NULL,         // Thread handle not inheritable
        FALSE,        // Set handle inheritance to FALSE
        0,            // No creation flags
        NULL,         // Use parent's environment block
        NULL,         // Use parent's starting directory 
        &si,          // Pointer to STARTUPINFO structure
        &pi))         // Pointer to PROCESS_INFORMATION structure
    {
        // Failed to create process
        printf("lsh: CreateProcess failed (%d).\n", GetLastError());
    }

    // Wait until the child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Get the exit code
    GetExitCodeProcess(pi.hProcess, &dwExitCode);

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 1;
}

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}