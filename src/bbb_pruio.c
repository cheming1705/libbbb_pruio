#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#include <time.h>

#include "bbb_pruio.h"

#ifndef BBB_PRUIO_START_ADDR_0
   #error "BBB_PRUIO_START_ADDR_0 must be defined."
#endif

#ifndef BBB_PRUIO_PREFIX
   #error "BBB_PRUIO_PREFIX must be defined."
#endif

#define DEBUG

/////////////////////////////////////////////////////////////////////
// Forward declarations
//
static inline int buffer_is_empty();
static inline void buffer_read(unsigned int* message);

/////////////////////////////////////////////////////////////////////
// PRU Initialization
//

static volatile unsigned int* shared_ram = NULL;

static int load_device_tree_overlay(){
   // Check if device tree overlay is loaded, load if needed.
   int device_tree_overlay_loaded = 0; 
   FILE* f;
   f = fopen("/sys/devices/bone_capemgr.9/slots","rt");
   if(f==NULL){
      return(1);
   }
   char line[256];
   while(fgets(line, 256, f) != NULL){
      if(strstr(line, "PRUIO-DTO") != NULL){
         device_tree_overlay_loaded = 1; 
      }
   }
   fclose(f);

   if(!device_tree_overlay_loaded){
      f = fopen("/sys/devices/bone_capemgr.9/slots","w");
      if(f==NULL){
         return(1);
      }
      fprintf(f, "PRUIO-DTO");
      fclose(f);
      usleep(100000);
   }

   return 0;
}

static int init_pru_system(){
   tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
   prussdrv_init();
   if(prussdrv_open(PRU_EVTOUT_0)) return 1;
   if(prussdrv_pruintc_init(&pruss_intc_initdata)) return 2;

   // Get pointer to shared ram
   void* p;
   prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &p);
   shared_ram = (volatile unsigned int*)p;

   return 0;
}

static int start_pru0_program(){
   char path[512] = "";
   strcat(path, BBB_PRUIO_PREFIX); 
   strcat(path, "/lib/libbbb_pruio_data0.bin");
   if(prussdrv_load_datafile(0, path)) return 3;

   strcpy(path, BBB_PRUIO_PREFIX); 
   strcat(path, "/lib/libbbb_pruio_text0.bin");
   if(prussdrv_exec_program_at(0, path, BBB_PRUIO_START_ADDR_0)) return 4;

   return 0;
}


/////////////////////////////////////////////////////////////////////
// Array to store adc callbacks
//

// An array of 14 function pointers called callbacks
static void (*adc_callbacks[14])(unsigned int, unsigned int);

static void reset_adc_callbacks(){
   int i;
   for(i=0; i<14; i++){
      adc_callbacks[i] = 0; 
   }
}

/////////////////////////////////////////////////////////////////////
// A thread to wait for ADC interrupts from PRU0
//

static pthread_t adc_thread;

static void* adc_thread_function(void* param){
   struct timespec t;
   unsigned long long time, previous_time;
   clock_gettime(CLOCK_MONOTONIC,&t);
   time = 1000000000*t.tv_sec + t.tv_nsec;
   previous_time = time;


   unsigned int message = 0;
   unsigned int data = 0;
   unsigned int overrun_counter = 99999;
   while(1){
      prussdrv_pru_wait_event(PRU_EVTOUT_0);
      prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

      while(!buffer_is_empty()){
         buffer_read(&message);
      /*    data = message; */
      }


      /* clock_gettime(CLOCK_MONOTONIC,&t); */
      /* previous_time = time; */
      /* time = 1000000000*t.tv_sec + t.tv_nsec; */
      /* overrun_counter ++; */
      /* if(overrun_counter > 100){ */
      /*    printf("overruns: %u\n", shared_ram[1026]); */
      /*    printf("time: %llu\n", (time-previous_time)/1000); */
      /*    overrun_counter=0; */
      /* } */



      /* usleep(1000); */
   }
   printf("%u\n", data);

   /* printf("%u messages received %u\n", i, data[0]); */
   /* printf("%u overruns\n", overruns); */
   /* printf("A\n"); */
   /* printf("%u overruns\n", 65541-i); */


   /* unsigned int i; */

   /* unsigned int data[1000][15]; */
   /* unsigned int data_counter = 0; */
   /* unsigned int datum = 0; */
   /* while(data_counter < 1000){ */
   /*    // Wait for interrupt from PRU */
   /*    prussdrv_pru_wait_event(PRU_EVTOUT_0); */
   /*    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT); */

   /*    clock_gettime(CLOCK_MONOTONIC,&t); */
   /*    previous_time = time; */
   /*    time = 1000000000*t.tv_sec + t.tv_nsec; */
   /*    #<{(| printf("time: %lu", (time-previous_time)/1000); |)}># */
   /*  */
   /*    data[data_counter][0] = (time-previous_time)/1000; */

   /*    i = 1; */
   /*    while(!buffer_is_empty()){ */
   /*       buffer_read(&datum); */
   /*       #<{(| printf("%8X\n",datum); |)}># */
   /*       data[data_counter][i] = datum; */
   /*       i++; */
   /*    } */

   /*    data_counter++; */

   /*    #<{(| usleep(1000); |)}># */
   /*    #<{(| (*adc_callbacks[1])(64, 64); |)}># */
   /*    #<{(| (*adc_callbacks[2])(3, 87); |)}># */
   /* } */

   /* printf("\n\n*****\n"); */
   /* int j; */
   /* for(i=0; i<1000; i++){ */
   /*    printf("%u ",data[i][0]); */
   /*    for(j=1; j<15; j++){ */
   /*       printf("%8X ",data[i][j]); */
   /*    }  */
   /*    printf("\n"); */
   /* } */

   return 0;
}

static int start_adc_thread(){
   // TODO: set real time priority to this thread
   pthread_attr_t attr;
   if(pthread_attr_init(&attr)){
      return 1;
   }
   if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)){
      return 1;
   }
   if(pthread_create(&adc_thread, &attr, &adc_thread_function, NULL)){
      return 1;
   }

   return 0;
}

static void stop_adc_thread(){
   while(pthread_cancel(adc_thread)){}
}

/////////////////////////////////////////////////////////////////////
// Ring buffer.
//

// Communication with PRU  is througn a ring buffer in the
// PRU shared memory area.
// shared_ram[0] to shared_ram[1023] is the buffer data.
// shared_ram[1024] is the start (read) pointer.
// shared_ram[1025] is the end (write) pointer.
//
// Messages are 32 bit unsigned ints. The 16 MSbits are the channel 
// number and the 16 LSbits are the value.
// 
// Read these:
// * http://en.wikipedia.org/wiki/Circular_buffer#Use_a_Fill_Count
// * https://groups.google.com/forum/#!category-topic/beagleboard/F9JI8_vQ-mE

unsigned int buffer_size;
volatile unsigned int *buffer_start;
volatile unsigned int *buffer_end;

void buffer_init(){
   buffer_size = 1024;
   buffer_start = &(shared_ram[1024]); // value inited to 0 in pru
   buffer_end = &(shared_ram[1025]); // value inited to 0 in pru
}

static inline int buffer_is_empty(){
   return (*buffer_start == *buffer_end);
}

static inline void buffer_read(unsigned int* message){
   *message = shared_ram[*buffer_start & (buffer_size-1)];

   // Don't write buffer start before reading message (mem barrier)
   // http://stackoverflow.com/questions/982129/what-does-sync-synchronize-do
   // https://en.wikipedia.org/wiki/Memory_ordering#Compiler_memory_barrier
   __sync_synchronize();

   // Increment buffer start, wrap around size
   *buffer_start = (*buffer_start+1) & (2*buffer_size - 1);
}


/////////////////////////////////////////////////////////////////////
// "Public" functions. ADC API.
//

int bbb_pruio_start_adc(){
   int err;
   load_device_tree_overlay();

   if((err=init_pru_system())){
      printf("Could not init PRU system: %i \n", err);
   }

   buffer_init();

   if((err=start_pru0_program())){
      printf("Could not load PRU0 program: %i \n", err);
   }

   reset_adc_callbacks();
   start_adc_thread();

   return 0;
}

int bbb_pruio_init_adc_pin(unsigned int pin_number, unsigned int steps, bbb_pruio_adc_callback){

   adc_callbacks[pin_number] = callback;

   return 0;
}

int bbb_pruio_stop_adc(){
   // TODO: send terminate message to PRU

   prussdrv_pru_disable(0);
   prussdrv_exit ();

   stop_adc_thread();
   reset_adc_callbacks();

   return 0;
}
