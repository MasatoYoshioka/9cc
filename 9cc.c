#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
    }

    token = tokenize(argv[1]);
    Program *prog = program();

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        int offset = 0;
        for(VarList *vl = fn->locals; vl; vl = vl->next) {
            Var *var = vl->var;
            offset += var->ty->size;
            var->offset = offset;
        }
        fn->stack_size = offset;
    }

    codegen(prog);

    return 0;
}
