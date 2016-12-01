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
/*
 * This example shows how to use adapters to use STL algorithms
 * with ARIA functors.
 * An adapter is an STL function object that calls a method pointer
 * (like &ArFunctor::invoke) on whatever object it's applied to when
 * invoked with the STL convention (which uses operator().)
 *
 * This lets you use ARIA functors in STL algorithms (which normally
 * only work on STL function objects):
 *
 *  STL Algorithm -> STL Function Pointer Adapter -> ARIA Functor -> 
 *    ARIA Functor Target Method
 *
 *
 * Things end up looking kind of redundant
 *   
*/


#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <iostream>

#include "Aria.h"


/*
  This is the class that contains some methods to use as callbacks targets.
*/
class CallbackClass
{
public:

  void callback1();
  void callback2(int i);
  bool callback3(const char *str);
};

void CallbackClass::callback1()
{
  printf("Invoked callback1\n");
}

void CallbackClass::callback2(int i)
{
  printf("Invoked callback2 with argument of '%d'\n", i);
}

bool CallbackClass::callback3(const char *str)
{
  printf("Invoked callback3 with argument of '%s'\n", str);
  return(true);
}


// But not all functor targets need to be in a class:
void globalFunction() 
{
  std::cout << "Invoked globalFunction." << std::endl;
}

int main()
{
  CallbackClass cb;


  // For functors with no arguments:
  std::list<ArFunctor*> functors;
  ArFunctorC<CallbackClass> functor1(cb, &CallbackClass::callback1);
  functors.push_back(&functor1);
  functors.push_back(&functor1);
  functors.push_back(&functor1);
  std::for_each(functors.begin(), functors.end(), 
      std::mem_fun(&ArFunctor::invoke));

  // For functors with arguments, give mem_fun template parameters.
  std::list<ArFunctor1<int>*> functorsWithArg;
  ArFunctor1C<CallbackClass, int> functor2(cb, &CallbackClass::callback2);
  std::mem_fun1_t<void, ArFunctor1<int>, int> f(&ArFunctor1<int>::invoke);
  functorsWithArg.push_back(&functor2);
  functorsWithArg.push_back(&functor2);
  functorsWithArg.push_back(&functor2);
  std::for_each(functorsWithArg.begin(), functorsWithArg.end(), std::bind2nd(f, 42));

  // You can use other STL algorithms if your functor returns something.
  // count_if will invoke each functor, and return the number of functor
  // invocations that returned true (in this case, 3, since they will 
  // always return true)
  std::list<ArRetFunctor1<bool, const char*>*> functorsWithRet;
  ArRetFunctor1C<bool, CallbackClass, const char *>
    functor3(cb, &CallbackClass::callback3);
  std::mem_fun1_t<bool, ArRetFunctor1<bool, const char*>, const char*> rf(&ArRetFunctor1<bool, const char*>::invokeR);
  functorsWithRet.push_back(&functor3);
  functorsWithRet.push_back(&functor3);
  functorsWithRet.push_back(&functor3);
  int c = std::count_if(functorsWithRet.begin(), functorsWithRet.end(), std::bind2nd(rf, "testing"));
  std::cout << "Count=" << c << std::endl;

  return(0);
}
