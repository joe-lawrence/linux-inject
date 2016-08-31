#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

int interval = 1;

void my_print(void)
{
	pid_t x = syscall(__NR_gettid);

	printf("%d: sleep(%d)... \n", x, interval);
	sleep(1);
}

/* this function is run by the second thread */
void *my_thread(void *x_void_ptr)
{
	while (1)
		my_print();

	return NULL;
}

int main()
{
	pthread_t my_thread_thread;

	if(pthread_create(&my_thread_thread, NULL, my_thread, NULL)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	while (1)
		my_print();

	/* wait for the second thread to finish */
	if(pthread_join(my_thread_thread, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return 2;
	}

	return 0;

}
