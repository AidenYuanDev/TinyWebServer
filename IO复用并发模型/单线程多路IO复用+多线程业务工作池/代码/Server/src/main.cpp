#include "server.h"
#include <iostream>
using namespace std;
#define PORT 8080

int main() {
  try {
    Server server(PORT);
    server.run();
  } catch (const exception &e) {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }
  return 0;
}
