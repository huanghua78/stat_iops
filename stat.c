#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

static int num_threads;
static char * filename;
const long N = 10000000;

void run(void * arg)
{
	struct stat st;
	long i;
	int rc;

	for (i = 0; i < N; i++) {
		rc = stat(filename, &st);
		if (rc != 0) {
			printf("Stat error:%d %d\n", rc, errno);
			return;
		}
	}
	printf("Completed stats:%d\n", N);
}

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct thread_info {    /* Used as argument to thread_start() */
	pthread_t thread_id;        /* ID returned by pthread_create() */
	int       thread_num;       /* Application-defined thread # */
	char     *argv_string;      /* From command-line argument */
};

/* Thread start function: display address near top of our stack,
   and return upper-cased copy of argv_string. */

static void *
thread_start(void *arg)
{
	struct thread_info *tinfo = arg;

	printf("Thread %2d: top of stack near %p; argv_string=%s\n",
        	tinfo->thread_num, (void *) &tinfo, tinfo->argv_string);
	run(NULL);

	return NULL;
}

int
main(int argc, char *argv[])
{
	int s, num_threads;
	pthread_attr_t attr;
	if (argc != 3) {
		fprintf(stderr, "usage: %s number_of_threads filename %d\n", argv[0], argc);
		return -1;
	}

	num_threads = atoi(argv[1]);
	filename    = argv[2];
	

	/* Initialize thread creation attributes. */

	s = pthread_attr_init(&attr);
	if (s != 0)
		handle_error_en(s, "pthread_attr_init");

	/* Allocate memory for pthread_create() arguments. */

	struct thread_info *tinfo = calloc(num_threads, sizeof(*tinfo));
	if (tinfo == NULL)
		handle_error("calloc");

	/* Create one thread for each command-line argument. */
	for (int tnum = 0; tnum < num_threads; tnum++) {
		tinfo[tnum].thread_num = tnum + 1;
		tinfo[tnum].argv_string = NULL;

		/* The pthread_create() call stores the thread ID into
		   corresponding element of tinfo[]. */

		s = pthread_create(&tinfo[tnum].thread_id, &attr,
			           &thread_start, &tinfo[tnum]);
		if (s != 0)
			handle_error_en(s, "pthread_create");
	}

	/* Destroy the thread attributes object, since it is no
	   longer needed. */

	s = pthread_attr_destroy(&attr);
	if (s != 0)
		handle_error_en(s, "pthread_attr_destroy");
	/* Now join with each thread, and display its returned value. */

	for (int tnum = 0; tnum < num_threads; tnum++) {
		s = pthread_join(tinfo[tnum].thread_id, NULL);
		if (s != 0)
			handle_error_en(s, "pthread_join");

		printf("Joined with thread %d\n",
			    tinfo[tnum].thread_num );
	}

	free(tinfo);
	exit(EXIT_SUCCESS);
}
