#include <cstdlib>

extern "C" int NetworkProcessMain(int argc, char** argv);

int main(int argc, char** argv)
{
    if (argc < 3) {
        return -1;
    }
    return NetworkProcessMain(argc, argv);
}
