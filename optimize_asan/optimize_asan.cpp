#include <unordered_set>
#include <unordered_map>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Instrumentation/AddressSanitizer.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

namespace
{
    struct OptimizeASan : public ModulePass
    {
        static char ID;
        OptimizeASan() : ModulePass(ID) {}

        void getAnalysisUsage(AnalysisUsage &AU) const override
        {
            // Add analysis passes
            AU.addRequired<BasicAAWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>(); // TODO: try to cleverly hoist instrumentation from loops
        }

        bool runOnModule(Module &M) override
        {
            for (Function &F : M)
            {
                errs() << "Running OptimizeASan pass on ";
                errs().write_escaped(F.getName()) << '\n';

                BasicAAResult &basicAAResult = getAnalysis<BasicAAWrapperPass>().getResult();
                AliasSetTracker aliasSetTracker();

                // https://github.com/google/sanitizers/wiki/AddressSanitizerCompileTimeOptimizations

                // loop through aliasSetTracker sets
                /*
                for (const auto &AS : aliasSetTracker)
                {
                    // find the pre-dominator for all aliased instructions
                    // insert a dummy load into predom to trigger instrumentation
                    // disable instrumentation on subsequent instructions
                }
                */

                /*
                LoopInfoWrapperPass &LIWP = getAnalysis<LoopInfoWrapperPass>(F);
                LoopInfo &LI = LIWP.getLoopInfo();

                for (Loop *L : LI)
                {
                }
                */

                // The below code prevents all ASan instrumentation entirely
                /*
                for (BasicBlock &BB : F)
                {
                    for (Instruction &Inst : BB)
                    {
                        LLVMContext &context = Inst.getContext();
                        MDNode *nosanitize = MDNode::get(context, MDString::get(context, "nosanitize"));
                        Inst.setMetadata(LLVMContext::MD_nosanitize, nosanitize);
                    }
                }
                */
            }

            return true;
        }
    };
}

char OptimizeASan::ID = 0;
static RegisterPass<OptimizeASan> X("optimize_asan", "Optimize ASan", false, false);
