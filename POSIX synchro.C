#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
 
static volatile bool buf_full;
static volatile int buf[5];
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cnd_empty = PTHREAD_COND_INITIALIZER,
                cnd_full  = PTHREAD_COND_INITIALIZER;
 
 
void put(int x){
   pthread_mutex_lock( &mtx );
   while ( buf_full ) {
      pthread_cond_wait( &cnd_empty, &mtx );
   }
   buf = x;
   buf_full = true;
   pthread_cond_signal( &cnd_full );
   pthread_mutex_unlock( &mtx );
}
 
int get(){
   int x;
   pthread_mutex_lock( &mtx );
   while ( ! buf_full ) {
      pthread_cond_wait( &cnd_full, &mtx );
   }
   x = buf;
   buf_full = false;
   pthread_cond_signal( &cnd_empty );
   pthread_mutex_unlock( &mtx );
   return x;
}
 
void* send1(void* p){
   put(1);
}
  
void* send2(void* p){
   put(2);
}
 
void* receive(void* p){
   int x;
   x = get();
   printf("%d\n", x); fflush(stdout);
   x = get();
   printf("%d\n", x); fflush(stdout);
   x = get();
   printf("%d\n", x); fflush(stdout);
}

void* f(void* p){
    static int i;
    static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&mtx);
    i++;
    pthread_mutex_unlock(&mtx);

    return NULL;
}
 
int main(){
   pthread_t tsnd_1, tsnd_2, trcv;
   buf_full = true;
   pthread_create(&tsnd_1, NULL, send1, NULL);
   pthread_create(&tsnd_2, NULL, send2, NULL);
   pthread_create(&trcv, NULL, receive, NULL);
   pthread_join(trcv, NULL);
   return 0;
}
