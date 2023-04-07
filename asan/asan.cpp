#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Instrumentation/AddressSanitizer.h"

using namespace llvm;

namespace
{
    struct ASan : public ModulePass
    {
        static char ID;
        ASan() : ModulePass(ID) {}

        bool runOnModule(Module &M) override
        {
            errs() << "hello world" << '\n';
            errs() << M.getName() << '\n';

            ModuleAnalysisManager MAM;
            PassManager<Module> PM;

            PM.addPass(ModuleAddressSanitizerPass(AddressSanitizerOptions()));
            PM.run(M, MAM);

            return true;
        }
    };
}

char ASan::ID = 0;
static RegisterPass<ASan> X("asan", "Address Sanitizer module pass", false, false);
