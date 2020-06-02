#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
    }

    token = tokenize(argv[1]);
    Node *node = expr();

    codegen(node);

    return 0;
}
