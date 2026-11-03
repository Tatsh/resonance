#include <stdio.h>

static void printWrapper(const char *value) {
    printf(value);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        return 0;
    }

    printf(argv[1]);       // Vulnerable.
    printf("%s", argv[1]); // Safe.
    printWrapper(argv[1]); // Vulnerable.

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        printf(buffer);       // Vulnerable.
        printf("%s", buffer); // Safe.
    }
    return 0;
}
