#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <poll.h>
#include <sys/epoll.h>

void fill_child_argv(int argc, char *argv[], int child_index, char *child_argv[]);
void print_argv(char *argv[]);
float sequential_mechanism(int fds[][2], int n);
float select_mechanism(int fds[][2], int n);
float poll_mechanism(int fds[][2], int n);
float epoll_mechanism(int fds[][2], int n);

const int MAX_CHILDREN = 32;

int main(int argc, char *argv[]) {
	//print_argv(argv);
	if (argc < 9) {
		printf("%s\n", "not enough arguements.");
	}

	char mechanism[32];
	int n = 10;
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "-n") == 0 && i < argc - 1) {
			n = atoi(argv[i + 1]);
		} else if (strcmp(argv[i], "--wait_mechanism") == 0 && i < argc - 1) {
			strcpy(mechanism, argv[i + 1]);
		}
	}

	pid_t pids[MAX_CHILDREN];
	int fds[MAX_CHILDREN][2];
	for (int i = 0; i < n; ++i) {
		if (pipe(fds[i]) < 0) {
			perror("pipe");
			exit(1);
		}
		if ((pids[i] = fork()) == -1) {
			perror("fork");
			exit(1);
		} else if (pids[i] == 0) {
			//printf("In child: %d\n", i);
			if (close(fds[i][0]) < 0) {
				perror("close");
			}

			//initialize stack memory for child argv
			char mem[5][100];
			char *child_argv[6];
			for (int i = 0; i < 5; ++i) {
				child_argv[i] = &mem[i][0];
			}
			child_argv[5] = NULL;
			fill_child_argv(argc, argv, i, child_argv);
			//print_argv(child_argv);

			if (dup2(fds[i][1], STDOUT_FILENO) < 0) {
				perror("dup2");
			}
			execv(child_argv[0], child_argv);

			exit(0);
		}
	}

	float result = 0;
	if (strcmp(mechanism, "sequential") == 0) {
		result = sequential_mechanism(fds, n);
	} else if (strcmp(mechanism, "select") == 0) {
		result = select_mechanism(fds, n);
	} else if (strcmp(mechanism, "poll") == 0) { 
		result = poll_mechanism(fds, n);
	} else {
		result = epoll_mechanism(fds, n);
	}

	printf("Final result : %f\n", result);

	return 0;
}

float sequential_mechanism(int fds[][2], int n) {
	float result = 0;

	for (int i = 0; i < n; ++i) {
		if (close(fds[i][1]) < 0) {
			perror("close");
		}
		
		int nbytes;
		char output[1024];
		if ((nbytes = read(fds[i][0], output, 1024)) > 0) {
			float child_result = 0;
			int tmp = 0;
			sscanf(output, "%d^%d / %d! : %f\n", &tmp, &tmp, &tmp, &child_result);
			output[nbytes] = 0;
			printf("Worker %d: %s", i, output);
			result += child_result;
		}
	}

	return result;
}

float select_mechanism(int fds[][2], int n) {
	float result = 0;
	for (int i = 0; i < n; ++i) {
		if (close(fds[i][1]) < 0) {
			perror("close");
		}
	}

	fd_set readset;

	int cnt = n;
	while (cnt > 0) {
		// initialize the set
		FD_ZERO(&readset);
		pid_t maxfd = 0;
		for (int i = 0; i < n; ++i) {
	   		FD_SET(fds[i][0], &readset);
	   		maxfd = (maxfd > fds[i][0]) ? maxfd : fds[i][0];
		}
		// now, check for readability
		int rc = select(maxfd + 1, &readset, NULL, NULL, NULL);
		if (rc == -1) {
			perror("select");
			exit(1);
		} else if (rc == 0) {
			break;
		} else {
		   	for (int i = 0; i < n; ++i) {
		   		// check if fd is ready
		      	if (FD_ISSET(fds[i][0], &readset)) {
			        int nbytes;
					char output[1024];
					if ((nbytes = read(fds[i][0], output, 1024)) > 0) {
						float child_result = 0;
						int tmp = 0;
						sscanf(output, "%d^%d / %d! : %f\n", &tmp, &tmp, &tmp, &child_result);
						output[nbytes] = 0;
						printf("Worker %d: %s", i, output);
						result += child_result;
						--cnt;
					}
		      	}
		   	}
		}
	}

	return result;
}

float poll_mechanism(int fds[][2], int n) {
	float result = 0;
	for (int i = 0; i < n; ++i) {
		if (close(fds[i][1]) < 0) {
			perror("close");
		}
	}

	struct pollfd poll_set[MAX_CHILDREN];
	for (int i = 0; i < n; ++i) {
		poll_set[i].fd = fds[i][0];
		poll_set[i].events = POLLIN;
	}

	int cnt = n;
	while (cnt > 0) {
		poll(poll_set, n, -1);
		for (int i = 0; i < n; ++i) {
	   		// check if fd is ready
	      	if (poll_set[i].revents & POLLIN) {
		        int nbytes;
				char output[1024];
				if ((nbytes = read(fds[i][0], output, 1024)) > 0) {
					float child_result = 0;
					int tmp = 0;
					sscanf(output, "%d^%d / %d! : %f\n", &tmp, &tmp, &tmp, &child_result);
					output[nbytes] = 0;
					printf("Worker %d: %s", i, output);
					result += child_result;
					--cnt;
					poll_set[i].fd = -1;
				}
	      	}
	   	}
	}

	return result;
}

float epoll_mechanism(int fds[][2], int n) {
	float result = 0;
	for (int i = 0; i < n; ++i) {
		if (close(fds[i][1]) < 0) {
			perror("close");
		}
	}

  	int efd = epoll_create1(0);
  	if (efd == -1) {
  		perror("epoll_create1");
  		exit(1);
  	}

  	struct epoll_event events[MAX_CHILDREN];
  	for (int i = 0; i < n; ++i) {
  		events[i].data.fd = fds[i][0];
  		events[i].events = EPOLLIN;
  		if (epoll_ctl(efd, EPOLL_CTL_ADD, fds[i][0], &events[i]) == -1) {
  			perror("epoll_ctl");
  			exit(1);
  		}
  	}

  	struct epoll_event test_events[MAX_CHILDREN];
  	int cnt = n;
  	while (cnt > 0) {
  		int ret = epoll_wait(efd, test_events, MAX_CHILDREN, -1);
  		if (ret > 0) {
			for (int i = 0; i < ret; ++i) {
		   		// check if fd is ready
		      	if (test_events[i].events & EPOLLIN) {
			        int nbytes;
					char output[1024];
					if ((nbytes = read(test_events[i].data.fd, output, 1024)) > 0) {
						float child_result = 0;
						int nv = 0;
						int xv = 0;
						sscanf(output, "%d^%d / %d! : %f\n", &xv, &nv, &nv, &child_result);
						output[nbytes] = 0;
						printf("Worker %d: %s", nv, output);
						result += child_result;
						--cnt;
					}
		      	}
		   	}
  		}
  	}

  	return result;
}

void fill_child_argv(int argc, char *argv[], int child_index, char *child_argv[]) {
	char number[32];
	sprintf(number, "%d", child_index);
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "--worker_path") == 0 && i < argc - 1) {
			strcpy(child_argv[0], argv[i + 1]);
		} else if (strcmp(argv[i], "-x") == 0 && i < argc - 1) {
			strcpy(child_argv[1], argv[i]);
			strcpy(child_argv[2], argv[i + 1]);
		} else if (strcmp(argv[i], "-n") == 0 && i < argc - 1) {
			strcpy(child_argv[3], argv[i]);
			strcpy(child_argv[4], number);
		}
	}
}

void print_argv(char *argv[]) {
	printf("In print_argv()\n");
	int i = 0;
	while (argv[i] != NULL) {
		printf("%s\n", argv[i++]);
	}
}