#include <iostream>
#include <iomanip>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
using namespace std;


const int ARRAY_SIZE = 400000;
const int NUM_OF_THREADS = 4;


class Numbers {

private:

          int size;

          double *myarray;

          double sum;



public:

          Numbers () {
            myarray = new double [ARRAY_SIZE];

            cout << "Populating array:" << endl;
            for(int i = 0; i < ARRAY_SIZE; i++){
              myarray[i] = double((rand() % 400 + 100))/100;

              cout << "\r" << setprecision(2) << fixed << double(i + 1 )/ (ARRAY_SIZE/100) << "%  " << flush;
            }
            cout << endl;
          }

          ~Numbers(){
            delete []myarray;
          }

          double get_sum(){
            return sum;
          }

          void add_sum(double addedNum){
            sum += addedNum;
          }

          double* get_array_ptr(int index){
            return &myarray[index];
          }
          void print_expected(){
            double sum = 0;
            for(int i = 0; i < ARRAY_SIZE; i++){
              sum += myarray[i];
            }
            cout << "---------------\n Expected Sum: " << sum << "\n---------------" << endl;
          }

};

struct thread_data{
  int thread_id;
  Numbers * numbers;
  pthread_mutex_t * lock;
};



void *do_work(void *arg){


  int fp[2];
  int rp[2];

  pid_t cpid;
  double tempSum = 0;
  if(pipe(fp) == -1){
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  if(pipe(rp) == -1){
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  cpid = fork();

  if(cpid == -1){
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if(cpid == 0){
    close(fp[1]);
    close(rp[0]);


    double val;

    for(int i = 0; i < (ARRAY_SIZE/NUM_OF_THREADS); i++){
      read(fp[0], &val, sizeof(val));
      tempSum += val;
      }



    write(rp[1], &tempSum, sizeof(tempSum));


    exit(0);
    }
  else{
    close(fp[0]);
    close(rp[1]);
    struct thread_data * data = (struct thread_data *)arg;
    int thread_id = data -> thread_id;
    Numbers * numbers = data -> numbers;
    double * arrayptr = numbers -> get_array_ptr(thread_id * ARRAY_SIZE/NUM_OF_THREADS);
    pthread_mutex_t * lock = data -> lock;

    cout << "Piping data from thread[" << thread_id << "]..." << endl;
    for(int i = 0; i < (ARRAY_SIZE/NUM_OF_THREADS); i++){
      write(fp[1], arrayptr, sizeof(arrayptr));
      arrayptr++;
      }



    wait(NULL);

    double addedVal;
    read(rp[0], &addedVal, sizeof(addedVal));
    cout << "Thread[" << thread_id <<"] received from pipe: " << addedVal << endl;

    pthread_mutex_lock(lock);
    cout << "Locked for thread[" << thread_id << "] and adding " << addedVal << " to existing sum:" << numbers-> get_sum() << endl;
    numbers-> add_sum(addedVal);
    cout << "New sum is: " << numbers -> get_sum() << endl;
    cout << "Releasing lock from thread[" << thread_id <<"]..." << endl;
    pthread_mutex_unlock(lock);

  //  close(fp[1]);
  //  close(rp[0]);
  }

}


int main(){

  Numbers numbers = Numbers();

  pthread_t threadList[NUM_OF_THREADS];
  struct thread_data data[NUM_OF_THREADS];
  pthread_mutex_t lock;

  pthread_mutex_init(&lock, NULL);


  numbers.print_expected();

  for(int i = 0; i <NUM_OF_THREADS; i++){
    data[i].thread_id = i;
    data[i].numbers = &numbers;
    data[i].lock = &lock;
    pthread_create(&threadList[i], NULL, do_work, &data[i]);
  }

  for(int i = 0; i < NUM_OF_THREADS; i++){
    pthread_join(threadList[i], NULL);
  }

  cout << "\n---\nFINAL RESULT: " << numbers.get_sum()<<endl;

  return 0;
}
