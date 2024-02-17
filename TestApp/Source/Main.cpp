#include "JScr.h"
using namespace JScr;

namespace TestApp
{
	int main(int argc, char* argv[])
	{
		printf("Loading script file...");

		auto script = Script::FromFile("C:\\Test\\test.jscr", {});

		//printf("Script file loaded, beginning execution.");
	}
}