#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Instrumentation/AddressSanitizer.h"
#include "llvm/Transforms/Instrumentation/SanitizerCoverage.h"

using namespace llvm;

namespace
{
    struct ASan : public ModulePass
    {
        static char ID;
        ASan() : ModulePass(ID) {}

        bool runOnModule(Module &M) override
        {
            for (Function &F : M)
            {
                F.addFnAttr(Attribute::SanitizeAddress);
            }

            ModuleAnalysisManager MAM;
            LoopAnalysisManager LAM;
            FunctionAnalysisManager FAM;
            CGSCCAnalysisManager CGAM;

            PassBuilder PB;
            PB.registerModuleAnalyses(MAM);
            PB.registerCGSCCAnalyses(CGAM);
            PB.registerFunctionAnalyses(FAM);
            PB.registerLoopAnalyses(LAM);
            PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

            ModulePassManager MPM;
            MPM.addPass(ModuleSanitizerCoveragePass());
            MPM.addPass(ModuleAddressSanitizerPass(AddressSanitizerOptions(), false));
            MPM.run(M, MAM);

            return true;
        }
    };
}

char ASan::ID = 0;
static RegisterPass<ASan> X("asan", "Address Sanitizer module pass", false, false);
