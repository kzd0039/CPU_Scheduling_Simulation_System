CC = gcc
REMOVE = rm

aubatch: aubatch.c 
	${CC} -o batch_job batch_job.c
	${CC} -o aubatch aubatch.c

clean:
	${REMOVE} aubatch batch_job