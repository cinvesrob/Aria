#include "Aria.h"
#include "ArNetworking.h"

void function1(void)
{
  printf("Function1 called\n");
}

void function2(void)
{
  printf("Function2 called\n");
}

void function3(void)
{
  printf("Function3 called\n");
}

void function4(ArArgumentBuilder *arg)
{
  printf("Function4 called with string '%s'\n", arg->getFullString());
}

void function5(ArArgumentBuilder *arg)
{
  printf("Function5 called with string '%s'\n", arg->getFullString());
}

void function6(ArArgumentBuilder *arg)
{
  printf("Function6 called with string '%s'\n", arg->getFullString());
}


int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;

  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }

  ArServerHandlerCommands commands(&server);
  commands.addCommand("Function1", "Call the function that is number 1!", 
			    new ArGlobalFunctor(&function1));
  commands.addCommand("Function2", "Second function to call", 
			    new ArGlobalFunctor(&function2));
  commands.addCommand("Function3", "Tree climb", 
			    new ArGlobalFunctor(&function3));
  commands.addStringCommand("StringFunction4", 
			"Print a string in function 4", 
			    new ArGlobalFunctor1<
			    ArArgumentBuilder *>(&function4));
  commands.addStringCommand("StringFunction5", 
			"Prints a string given (5)", 
			    new ArGlobalFunctor1<
			    ArArgumentBuilder *>(&function5));
  commands.addStringCommand("StringFunction6", 
				  "Print out a value for function 6", 
			    new ArGlobalFunctor1<
			    ArArgumentBuilder *>(&function6));

  server.run();
}
