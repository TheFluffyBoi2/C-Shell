#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>

#define MAX_SIZE 50
#define MAX_COMMANDS 5000
#define MAX_ARGUMENTS 10

char* current_dir() {
	static char cwd[1024];

	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		return cwd;
	}
	else {
		perror("Error while getting the current directory");
		return NULL;
	}
}

int cd(int argument_nr, char* arguments[]) {
	printf("%s", arguments[1]);
	if (argument_nr != 2) {
		printf("Too many arguments");
		return -1;
	}
	else {
		if (chdir(arguments[1]) == 0) {
			printf("Changed the current working directory to: %s\n", arguments[1]);
			return 0;
		}
		else {
			perror("Invalid working directory");
			return errno;
		}
	}
}

int execute_command(int argument_nr, char* arguments[]) {
	pid_t pid = fork();
	if (pid < 0) {
		perror("Fork failed");
		return errno;
	}
	else if (pid == 0) {
		if (execvp(arguments[0], arguments) < 0) {
			perror("Invalid command");
			exit(0);
		}
	}
	else {
		wait(NULL);
	}
	return 0;
}

int execute_pipe_commands(int pipe_nr, int argument_nr, char* arguments[]) {
	char* commands[argument_nr + 1];

	int fd[2];
	int prev_fd = -1;

	int index = 0;
	for (int i = 0; i < argument_nr; ++i) {
		if (strcmp(arguments[i], "|") != 0) {
			commands[index] = arguments[i];
			index++;
		}
		else {
			commands[index] = NULL;
			if (pipe(fd) < 0) {
				perror("Pipe error");
				return errno;
			}	
			pid_t pid = fork();
			if (pid < 0) {
				perror("Fork failed");
				return errno;
			}
			else if (pid == 0) {
				if (prev_fd != -1) {
					dup2(prev_fd, 0);
					close(prev_fd);
				}
				dup2(fd[1], 1);
				close(fd[0]);
				close(fd[1]);
				if (execvp(commands[0], commands) < 0) {
					perror("Invalid command");
					exit(0);
				}
			}
			else {
				wait(NULL);
				close(fd[1]);
				if (prev_fd != -1) {
					close(prev_fd);
				}
				prev_fd = fd[0];
				index = 0;
			}
		}
	}

	commands[index] = NULL;
	pid_t pid = fork();
	if (pid < 0) {
        perror("Fork failed");
        return errno;
    }

    if (pid == 0) {
        if (prev_fd != -1) {
            dup2(prev_fd, 0);
            close(prev_fd);
		}
        if (execvp(commands[0], commands) < 0) {
            perror("Invalid command");
            exit(errno);
        }
    }
	else {
        wait(NULL);
        if (prev_fd != -1) {
            close(prev_fd);
        }
    }
	return 0;
}

int main(int argc, char* argv[]) {
	printf("-----------------------------------\n");
	printf("|Dumitru Marius-Sebastian SO Shell|\n");
	printf("-----------------------------------\n");

	char* all_commands[MAX_COMMANDS];
	char command[MAX_SIZE];
	int command_nr = 0;
	int retry;

	while (1) {
		retry = 0;

		if (command_nr > MAX_COMMANDS) {
			printf("Limita de comenzi depasita");
			break;
		}

		printf("%s > ", current_dir());

		fgets(command, MAX_SIZE, stdin);
		command[strcspn(command, "\n")] = '\0';
		if (strcmp(command, "exit") == 0 || strcmp(command, "Exit") == 0 || strcmp(command, "EXIT") == 0) {
			for (int i = 0; i <= command_nr; ++i) {
				free(all_commands[i]);
			}
			break;
		}

		all_commands[command_nr] = malloc(strlen(command) + 1);
		strcpy(all_commands[command_nr], command);

		if (strcmp(command, "history") == 0 || strcmp(command, "History") == 0 || strcmp(command, "HISTORY") == 0) {
			printf("HISTORY:\n");
			for (int i = 0; i <= command_nr; ++i) {
				printf("%d %s\n", i, all_commands[i]);
			}
		}

		char* arguments[MAX_ARGUMENTS];
		int argument_nr = 0;

		char* tocken = strtok(command, " ");
		while (tocken != NULL) {
			if (argument_nr > MAX_ARGUMENTS) {
				printf("Too many arguments\n");
				retry = 1;
				break;
			}
			arguments[argument_nr] = malloc(strlen(tocken) + 1);
			strcpy(arguments[argument_nr], tocken);
			argument_nr++;
			tocken = strtok(NULL, " ");
		}

		int pipe_nr = 0;
		for (int i = 0; i < argument_nr; ++i) {
			if (strcmp(arguments[i], "|") == 0) {
				pipe_nr++;
			}
		}

		arguments[argument_nr + 1] = NULL;

		command_nr++;

		if (retry == 1) {
			for (int i = 0; i < argument_nr; ++i) {
				free(arguments[i]);
			}
			continue;
		}
		
		if (strcmp(arguments[0], "cd") == 0) {
			cd(argument_nr, arguments);
		}
		 else if (pipe_nr > 0) {
		 	execute_pipe_commands(pipe_nr, argument_nr, arguments);
		 }
		else {
			execute_command(argument_nr, arguments);
		}

		for (int i = 0; i < argument_nr; ++i) {
			free(arguments[i]);
		}

	}
	return 0;
}
