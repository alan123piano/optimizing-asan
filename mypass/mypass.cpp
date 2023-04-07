#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

namespace
{
    struct Hello : public llvm::FunctionPass
    {
        static char ID;
        Hello() : FunctionPass(ID) {}

        bool runOnFunction(llvm::Function &F) override
        {
            llvm::errs() << "Hello: ";
            llvm::errs().write_escaped(F.getName()) << '\n';
            return false;
        }
    };
}

char Hello::ID = 0;
static llvm::RegisterPass<Hello> X("hello", "Hello World Pass",
                                   false /* Only looks at CFG */,
                                   false /* Analysis Pass */);
