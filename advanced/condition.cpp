/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2014 Adept Technology

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

If you wish to redistribute ARIA under different terms, contact 
Adept MobileRobots for information about a commercial version of ARIA at 
robots@mobilerobots.com or 
Adept MobileRobots, 10 Columbia Drive, Amherst, NH 03031; +1-603-881-7960
*/
#include <stdlib.h>
#include <time.h>
#include "Aria.h"


/*
  This example is to show how to use a condition variable. Conditions
  are usefull for delaying the execution of a thread until some time later
  when some other part of the program wants to wake it up.

  This program has 5 threads. The main thread and 4 worker threads. The
  worker threads all wait on the condition variable to be woken up. When
  they are woken up they print that fact and then go right back to sleep
  waiting on the condition variable.

  The main thread randomly sleeps between 100ms and 500ms and then wakes
  up a single thread. It does this 10 times then tells all the threads
  to exit and waits for them to do so. The main thread then exits, which
  ends the program.

  This is simply an example of how to make threads wait for something to
  happen without doing a busy loop, which takes up CPU time.
*/


// The thread class
class Task : public ArASyncTask
{
public:

  Task(int num, ArCondition *cond);
  virtual ~Task() {}

  // The run loop of the thread.
  void * runThread(void *arg);

protected:

  int myNum;
  ArCondition *myCond;
};


// Constructor
Task::Task(int num, ArCondition *cond) :
  myNum(num),
  myCond(cond)
{
  setThreadName("Task");
}

// The main loop for the threads. It checks to see if its still running.
// If not, it exits. The running variable is managed by ArThread. While its
// running, it waits on the condition variable.
void * Task::runThread(void *arg)
{
  threadStarted();
  while (getRunning())
  {
    printf("Task %d waiting\n", myNum);
    myCond->wait();
    printf("Task %d woke up\n", myNum);
  }

  return(NULL);
}


int main()
{
  int i, sleepTime;
  // The condition the threads will be using
  ArCondition cond;
  // The threads
  Task task1(1, &cond);
  Task task2(2, &cond);
  Task task3(3, &cond);
  Task task4(4, &cond);

  // Initialize Aria, which in turn initializes the thread layer
  Aria::init();

  // Initialize the rand() function
  srand((unsigned)time(NULL));

  // Lets start all correct threads.
  task1.create();
  task2.create();
  task3.create();
  task4.create();

  // Lets wake up the threads at different random times
  for (i=0; i<10; ++i)
  {
    sleepTime=rand()%400+100;
    printf("Main: Sleeping %dms\n", sleepTime);
    ArUtil::sleep(sleepTime);
    printf("Main: Waking up a thread\n");
    cond.signal();
  }

  printf("Exiting\n");

  // Stop all the threads, which sets their running variable to false
  ArThread::stopAll();

  // Now that all the threads are marked as not running, wake them up
  // so that we can exit the program gracefully.
  cond.broadcast();

  // Wait for all the threads to exit
  ArThread::joinAll();

  // Uninit Aria
  Aria::uninit();

  return(0);
}
