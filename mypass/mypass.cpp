#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation/AddressSanitizer.h"

using namespace llvm;

namespace
{
    struct Hello : public FunctionPass
    {
        static char ID;
        Hello() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override
        {
            errs() << "Running ASan pass on ";
            errs().write_escaped(F.getName()) << '\n';

            // block sanitization on everything
            for (BasicBlock &BB : F)
            {
                for (Instruction &Inst : BB)
                {
                    LLVMContext &context = Inst.getContext();
                    Inst.hasMetadata();
                    MDNode *nosanitize = MDNode::get(context, MDString::get(context, "nosanitize"));
                    Inst.setMetadata(LLVMContext::MD_nosanitize, nosanitize);
                }
            }

            /* FunctionAnalysisManager FAM;
            PassManager<Function> PM;

            PM.run(F, FAM); */

            return true;
        }
    };
}

char Hello::ID = 0;
static RegisterPass<Hello> X("mypass", "Custom Pass", false, false);
