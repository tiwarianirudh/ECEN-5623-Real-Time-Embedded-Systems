
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sched.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <sstream>
#include <fstream>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>


#define PPM_MQ "/ppm_writer_mq"
#define JPG_MQ "/jpg_writer_mq"
#define SHP_MQ "/sharp_writer_mq"
#define BACK_MQ "/back_writer_mq"
#define ERROR -1
#define NUM_THREADS 6
#define NUM_FRAMES 10

using namespace cv;
using namespace std;

int dev=0;
VideoCapture cap(1);


//struct timespec frame_time;
static struct timespec sleep_time = {0, 0};
static struct timespec remaining_time = {0, 0};
double capture_start_time = 0.0;

static mqd_t frame_message_queue;
static mqd_t jpg_message_queue;
static mqd_t sharp_message_queue;
static mqd_t back_message_queue;
struct mq_attr message_queue_attr;
struct mq_attr jpg_queue_attr;
static mq_attr sharp_queue_attr;
static mq_attr back_queue_attr;

typedef struct
{
    int threadIdx;
} threadParams_t;

pthread_t threads[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param main_param;
pthread_attr_t main_attr;
pid_t mainpid;

sem_t sem_ppm, sem_jpg, sem_sharp, sem_write, sem_BE;


double getTimeMsec();
void precisionDelay(long int seconds, long int nanoseconds);
void intialize_messagequeue(void);
void destroy_messagequeue(void);


void precisionDelay(long int seconds, long int nanoseconds)
{
  sleep_time.tv_sec = seconds;
  sleep_time.tv_nsec = nanoseconds;

  do
  {
    nanosleep(&sleep_time, &remaining_time);

    sleep_time.tv_sec = remaining_time.tv_sec;
    sleep_time.tv_nsec = remaining_time.tv_nsec;
  }
  while ((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0));

}

double getTimeMsec(void)
{
  struct timespec event_ts = {0, 0};

  clock_gettime(CLOCK_REALTIME, &event_ts);
  return ((event_ts.tv_sec)*1000.0) + ((event_ts.tv_nsec)/1000000.0);
}

void intialize_messagequeue(void)
{
  message_queue_attr.mq_maxmsg = NUM_FRAMES;
  message_queue_attr.mq_msgsize = sizeof(char *);
  message_queue_attr.mq_flags = 0;

  frame_message_queue = mq_open(PPM_MQ, O_CREAT|O_RDWR, 0644, &message_queue_attr);
  if(frame_message_queue == (mqd_t)ERROR)
    perror("mq_open");

  jpg_queue_attr.mq_maxmsg = NUM_FRAMES;
  jpg_queue_attr.mq_msgsize = sizeof(char *);
  jpg_queue_attr.mq_flags = 0;

  jpg_message_queue = mq_open(JPG_MQ, O_CREAT|O_RDWR, 0644, &jpg_queue_attr);
  if(jpg_message_queue == (mqd_t)ERROR)
    perror("mq_open");

  sharp_queue_attr.mq_maxmsg = NUM_FRAMES;
  sharp_queue_attr.mq_msgsize = sizeof(char *);
  sharp_queue_attr.mq_flags = 0;

  sharp_message_queue = mq_open(SHP_MQ, O_CREAT|O_RDWR, 0644, &sharp_queue_attr);
  if(sharp_message_queue == (mqd_t)ERROR)
    perror("mq_open");

  back_queue_attr.mq_maxmsg = NUM_FRAMES;
  back_queue_attr.mq_msgsize = sizeof(char *);
  back_queue_attr.mq_flags = 0;

  back_message_queue = mq_open(BACK_MQ, O_CREAT|O_RDWR, 0644, &back_queue_attr);
  if(back_message_queue == (mqd_t)ERROR)
    perror("mq_open");

}

void destroy_messagequeue(void)
{
  mq_close(frame_message_queue);
  mq_close(jpg_message_queue);
  mq_close(sharp_message_queue);
  mq_close(back_message_queue);
  mq_unlink(PPM_MQ);
  mq_unlink(JPG_MQ);
  mq_unlink(SHP_MQ);
  mq_unlink(BACK_MQ);
}

void *captureThread(void *threadp)
{
    int i = 0;

    double capture_end_time = 0.0;
    double exec_time = 0.0;
    double prev_exec_time = 0.0;
    double cap_jitter = 0.0;
    double accumulated_jitter = 0.0;
    double average_jitter = 0.0;

    Mat frame_ppm;
    char *frame_ptr;
    char buffer[sizeof(char *)];
    frame_ptr =  (char *) malloc(sizeof(frame_ppm.data));//this is creating problem

    system("uname -a > spec.out");

    while(i < NUM_FRAMES)
    {
        sem_wait(&sem_ppm);
        capture_start_time= getTimeMsec();
        printf("Frame: %d\n", i);
        printf("Timestamp: %0.8lf seconds\n", capture_start_time/1000.0);
        cap.open(dev);
        cap >> frame_ppm; // get a new frame from cameras
        cap.release();
        frame_ptr = (char *) frame_ppm.data;
        memcpy(buffer, &frame_ptr, sizeof(char *));
        if(frame_ptr==NULL) {
          printf("Null pointer\n");
          break;
        }

        if(mq_send(frame_message_queue, buffer, message_queue_attr.mq_msgsize, 30) == ERROR)
        {
          perror("mq_send error");
        }

        if(mq_send(jpg_message_queue, buffer, jpg_queue_attr.mq_msgsize, 30) == ERROR)
        {
          perror("mq_send error for jpg");
        }

        if(mq_send(sharp_message_queue, buffer, sharp_queue_attr.mq_msgsize, 30) == ERROR)
        {
          perror("mq_send error for sharp");
        }

        if(mq_send(back_message_queue, buffer, back_queue_attr.mq_msgsize, 30) == ERROR)
        {
          perror("mq_send error for background");
        }


        i++;
        capture_end_time = getTimeMsec();
        exec_time = (capture_end_time - capture_start_time);
        printf("Capture Execution Time: %0.8lf\n", exec_time/1000.0);
        if(i>0){
        cap_jitter = exec_time - prev_exec_time;
        }
        prev_exec_time = exec_time;
        accumulated_jitter += cap_jitter;
        sem_post(&sem_write);
        sem_post(&sem_jpg);
        sem_post(&sem_sharp);
        sem_post(&sem_BE);

    }
    average_jitter = accumulated_jitter/NUM_FRAMES;
    printf("\nAverage capture jitter: %0.8lf miliseconds\n\n", average_jitter);
    pthread_exit(NULL);
}

void *ppmwriterThread(void *threadp)
{
    int i = 0;
    unsigned int priority;
    Mat frame_ppm;
    char *frame_ptr;
    char buffer[sizeof(char *)];
    std::ostringstream name;
    std::vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_PXM_BINARY);
    compression_params.push_back(1); // #0 for P3 and #1 for P6

    while(i < NUM_FRAMES)
    {
      sem_wait(&sem_write);
      name << "frame_" << i << ".ppm";
      if(mq_receive(frame_message_queue, buffer, message_queue_attr.mq_msgsize, &priority) == ERROR)
      {
        perror("mq_receive error");
      }
      else{
        memcpy(&frame_ptr, buffer, sizeof(char *));
        frame_ppm = Mat(480, 640, CV_8UC3, frame_ptr);
      }

      imwrite(name.str(), frame_ppm, compression_params);

      std::fstream outfile;
      std::fstream temp;
      std::fstream temp1;

      outfile.open (name.str(), ios::in|ios::out);
      outfile.seekp (ios::beg);
      outfile << "  ";

      temp.open("test.txt", ios::in|ios::out|ios::trunc);
      temp << outfile.rdbuf();
      outfile.close();
      temp.close();

      outfile.open (name.str(), ios::in|ios::out|ios::trunc);
      temp1.open("test.txt", ios::in|ios::out);
      temp.open("spec.out", ios::in);

      outfile << "P6" << endl << "#Time Stamp: " << setprecision(10) << fixed << capture_start_time/1000.0 << " seconds" << endl << "#System Specs: " << temp.rdbuf() << temp1.rdbuf();

      outfile.close();
      temp.close();
      i++;
      name.str("");//overwriting with blank string
    }
    pthread_exit(NULL);
}

void *jpgThread(void *threadp)
{
    Mat frame_jpg;
    unsigned int priority;
    char *frame_ptr;
    char buffer[sizeof(char *)];
    std::ostringstream name, name1;
    std::vector<int> compression_params;
    int i = 0;
    double frame_start_time = 0.0;
    double frame_end_time = 0.0;
    double exec_time = 0.0;
    double prev_exec_time = 0.0;
    double cap_jitter = 0.0;
    double accumulated_jitter = 0.0;
    double average_jitter = 0.0;

    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95); // #0 for P3 and #1 for P6

    while(i < NUM_FRAMES)
    {
        sem_wait(&sem_jpg);
        frame_start_time = getTimeMsec();
        name << "frame_comp" << i << ".jpg";
        name1 << "frame_" << i << ".ppm";
        if(mq_receive(jpg_message_queue, buffer, jpg_queue_attr.mq_msgsize, &priority) == ERROR)
        {
          perror("mq_receive error");
        }
        else{
          memcpy(&frame_ptr, buffer, sizeof(char *));
          frame_jpg = Mat(480, 640, CV_8UC3, frame_ptr);
        }
        imwrite(name.str(), frame_jpg, compression_params);
        name.str("");//overwriting with blank string
        name1.str("");
        i++;
        frame_end_time = getTimeMsec();
        exec_time = frame_end_time - frame_start_time;
        printf("Jpg Execution Time: %0.8lf\n", exec_time/1000.0);
        if(i>0){
        cap_jitter = exec_time - prev_exec_time;
        }
        prev_exec_time = exec_time;
        accumulated_jitter += cap_jitter;
    }
    average_jitter = accumulated_jitter/NUM_FRAMES;
    printf("\nAverage compress jitter: %0.8lf miliseconds\n\n", average_jitter);
    pthread_exit(NULL);
}


void *sharpenThread(void *threadp)
{
    int i = 0;
    Mat blur_image, temp_image, frame_sharp;
    unsigned int priority;
    char *frame_ptr;
    char buffer[sizeof(char *)];
    std::ostringstream name, name1;
    std::vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_PXM_BINARY);
    compression_params.push_back(1); // #0 for P3 and #1 for P6
    double frame_start_time = 0.0;
    double frame_end_time = 0.0;
    double exec_time = 0.0;
    double prev_exec_time = 0.0;
    double cap_jitter = 0.0;
    double accumulated_jitter = 0.0;
    double average_jitter = 0.0;

    while(i < NUM_FRAMES)
    {
        sem_wait(&sem_sharp);
        frame_start_time = getTimeMsec();
        name << "frame_sharp" << i << ".ppm";
        //name1 << "frame_" << i << ".ppm";
        frame_start_time = getTimeMsec();
        if(mq_receive(sharp_message_queue, buffer, sharp_queue_attr.mq_msgsize, &priority) == ERROR)
        {
          perror("mq_receive error for sharp");
        }
        else{
          memcpy(&frame_ptr, buffer, sizeof(char *));
          temp_image = Mat(480, 640, CV_8UC3, frame_ptr);
        }
        GaussianBlur(temp_image, blur_image, cv::Size(0, 0), 3);
        addWeighted(temp_image, 1.5, blur_image, -0.5, 0, frame_sharp);
        imwrite(name.str(), frame_sharp, compression_params);
        name.str("");//overwriting with blank string
        //name1.str("");
        i++;
        frame_end_time = getTimeMsec();
        exec_time = frame_end_time - frame_start_time;
        printf("Sharp Execution Time: %0.8lf\n", exec_time/1000.0);
        if(i>0){
        cap_jitter = exec_time - prev_exec_time;
        }
        prev_exec_time = exec_time;
        accumulated_jitter += cap_jitter;
    }
    average_jitter = accumulated_jitter/NUM_FRAMES;
    printf("\nAverage Sharpen jitter: %0.8lf miliseconds\n\n", average_jitter);
    pthread_exit(NULL);
}

void *backgroundElimThread(void *threadp)
{
  int i=0;

  Mat diff, previous, next, thres_frame;
  unsigned int priority;
  char *frame_ptr;
  //char *frame_ptr1;
  char buffer[sizeof(char *)];

  std::ostringstream name, name1, name2;
  std::vector<int> compression_params;
  compression_params.push_back(CV_IMWRITE_PXM_BINARY);
  compression_params.push_back(1); // #0 for P3 and #1 for P6

  double frame_start_time = 0.0;
  double frame_end_time = 0.0;
  double exec_time = 0.0;
  double prev_exec_time = 0.0;
  double cap_jitter = 0.0;
  double accumulated_jitter = 0.0;
  double average_jitter = 0.0;

  while(i<NUM_FRAMES)
  {
      sem_wait(&sem_BE);
      frame_start_time = getTimeMsec();
      name << "frame_BE" << i << ".ppm";
      if(mq_receive(back_message_queue, buffer, back_queue_attr.mq_msgsize, &priority) == ERROR)
      {
        perror("mq_receive error");
      }
      else{
        memcpy(&frame_ptr, buffer, sizeof(char *));
        next = Mat(480, 640, CV_8UC3, frame_ptr);
      }

      cv::cvtColor(next, next, CV_BGR2GRAY);
      if(i>0){
      absdiff(previous, next, diff);
      threshold(diff, thres_frame, 10, 255, CV_THRESH_BINARY);
      imwrite(name.str(), thres_frame, compression_params);
      }
      previous = next.clone();

      i++;
      name.str("");//overwriting with blank string

      frame_end_time = getTimeMsec();
      exec_time = frame_end_time - frame_start_time;
      printf("Background Elimination Execution Time: %0.8lf\n\n", exec_time/1000.0);
      if(i>0){
      cap_jitter = exec_time - prev_exec_time;
      }
      prev_exec_time = exec_time;
      accumulated_jitter += cap_jitter;
  }
  average_jitter = accumulated_jitter/NUM_FRAMES;
  printf("Average Background jitter: %0.8lf miliseconds\n\n", average_jitter);
  pthread_exit(NULL);

}

void *Sequencer(void *threadp)
{
  int i = 0;
  printf("Starting Sequencer\n\n");

  // Sequencing loop for LCM phasing of S1, S2
  do
  {
      sem_post(&sem_ppm);precisionDelay(1, 0);
      i++;
   }
   while (i<NUM_FRAMES);
   pthread_exit(NULL);
}


int main(int argc, char *argv[])
{

    if(argc > 1)
    {
        sscanf(argv[1], "%d", &dev);
        printf("Using %s\n", argv[1]);
    }

    else if(argc == 1){
        printf("Using default\n");
    }
    else
    {
        printf("usage: videothread [dev]\n");
        exit(-1);
    }

    int i = 0;
    int rc;
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 640);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 480);
    cap.set(CV_CAP_PROP_FPS, 10.0);
    printf("fps %lf\n", cap.get(CV_CAP_PROP_FPS));
    cpu_set_t cpuset;
    cpu_set_t cpuset1;

    intialize_messagequeue();
    sem_init(&sem_ppm, 0, 0);
    sem_init(&sem_write, 0, 0);
    sem_init(&sem_jpg, 0, 0);
    sem_init(&sem_sharp, 0, 0);

    mainpid=getpid();

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if(rc < 0) perror("main_param");

    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);

    for(i=0;i<NUM_THREADS;i++){
        pthread_attr_init(&rt_sched_attr[i]);
    }
    pthread_attr_setaffinity_np(&rt_sched_attr[0], sizeof(cpu_set_t), &cpuset);
    rt_param[0].sched_priority=rt_max_prio-1;
    pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);

    pthread_create(&threads[0], &rt_sched_attr[0], Sequencer, (void*) (NULL));

    CPU_ZERO(&cpuset1);
    CPU_SET(2, &cpuset1);

    for(i=1;i<NUM_THREADS-1;i++){
      rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
      rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
      pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &cpuset1);
      rt_param[i].sched_priority=rt_max_prio-i-1;
      pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);
    }

    pthread_create(&threads[1], &rt_sched_attr[1], captureThread, (void*)(NULL));
    pthread_create(&threads[2], &rt_sched_attr[2], jpgThread, (void*)(NULL));
    pthread_create(&threads[3], &rt_sched_attr[3], sharpenThread, (void*)(NULL));
    pthread_create(&threads[4], &rt_sched_attr[4], backgroundElimThread, (void*)(NULL));
    pthread_create(&threads[5], &rt_sched_attr[5], ppmwriterThread, (void*)(NULL));

    for(i=0;i<NUM_THREADS;i++)
    {
       pthread_join(threads[i], NULL);
    }

    destroy_messagequeue();
    return 0;
}
