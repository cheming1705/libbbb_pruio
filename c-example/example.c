/* Lib BBB Pruio 
 * 
 * Copyright (C) 2014 Rafael Vega <rvega@elsoftwarehamuerto.org> 
 * Copyright (C) 2014 Miguel Vargas <miguelito.vargasf@gmail.com> 
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <bbb_pruio.h>
#include <bbb_pruio_pins.h>

/////////////////////////////////////////////////////////////////////
unsigned int finished = 0;
void signal_handler(int signal){
   finished = 1;
}

/////////////////////////////////////////////////////////////////////
static pthread_t monitor_thread;

static void* monitor_inputs(void* param){
   bbb_pruio_message message;
   while(!finished){
      while(bbb_pruio_messages_are_available() && !finished){
         bbb_pruio_read_message(&message);

         // Message from gpio
         if(message.is_gpio && message.gpio_number==P9_11){
            printf("P9_11: %i\n", message.value);
         }
         else if(message.is_gpio && message.gpio_number==P9_13){
            printf("P9_13: %i\n", message.value);
         }

         // Messages from adc
         else if(!message.is_gpio && message.adc_channel==1){
            printf("1: %i\n", message.value);
         }
         else if(!message.is_gpio && message.adc_channel==7){
            printf("\t7: %i\n", message.value);
         }
      }
      usleep(10000);
   }

   return NULL;
}

static int start_monitor_thread(){
   // TODO: set real time priority to this thread
   pthread_attr_t attr;
   if(pthread_attr_init(&attr)){
      return 1;
   }
   if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)){
      return 1;
   }
   if(pthread_create(&monitor_thread, &attr, &monitor_inputs, NULL)){
      return 1;
   }

   return 0;
}

static void stop_monitor_thread(){
   pthread_cancel(monitor_thread);
}

int main(int argc, const char *argv[]){
   // Listen to SIGINT signals (program termination)
   signal(SIGINT, signal_handler);

   bbb_pruio_start();

   start_monitor_thread();

   // Initialize 2 pins as outputs
   if(bbb_pruio_init_gpio_pin(P9_16, BBB_PRUIO_OUTPUT_MODE)){
      fprintf(stderr, "%s\n", "Could not initialize pin P9_12");
   }
   if(bbb_pruio_init_gpio_pin(P9_18, BBB_PRUIO_OUTPUT_MODE)){
      fprintf(stderr, "%s\n", "Could not initialize pin P9_14");
   }
   
   // Init 2 pins as inputs
   if(bbb_pruio_init_gpio_pin(P9_13, BBB_PRUIO_INPUT_MODE)){
      fprintf(stderr, "%s\n", "Could not initialize pin P9_13");
   }
   if(bbb_pruio_init_gpio_pin(P9_11, BBB_PRUIO_INPUT_MODE)){
      fprintf(stderr, "%s\n", "Could not initialize pin P9_11");
   }

   // Init 2 analog inputs
   if(bbb_pruio_init_adc_pin(1)){
      fprintf(stderr, "%s\n", "Could not initialize adc pin 1");
   }
   if(bbb_pruio_init_adc_pin(7)){
      fprintf(stderr, "%s\n", "Could not initialize adc pin 7");
   }


   // Check if library is returning adequately when trying to 
   // re-initialize a pin.
   if(!bbb_pruio_init_gpio_pin(P9_16, BBB_PRUIO_INPUT_MODE)){
      fprintf(stderr, "%s\n", "P9_16 was already initialized, should have returned error");
      exit(1);
   }
   if(bbb_pruio_init_gpio_pin(P9_18, BBB_PRUIO_OUTPUT_MODE)){
      fprintf(stderr, "%s\n", "P9_18 was already initialized as output, should have not returned error");
      exit(1);
   }

   // Blink 2 outputs
   while(!finished){
      bbb_pruio_set_pin_value(P9_16, 0);
      bbb_pruio_set_pin_value(P9_18, 1);
      sleep(3);
      bbb_pruio_set_pin_value(P9_16, 1);
      bbb_pruio_set_pin_value(P9_18, 0);
      sleep(3);
   }

   bbb_pruio_stop();
   stop_monitor_thread();

   return 0;
}
