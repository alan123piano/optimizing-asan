#include <vector>
#include <list>
#include <utility>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/MemoryLocation.h"

using namespace llvm;

namespace
{
    struct OptimizeASan : public FunctionPass
    {
        static char ID;
        OptimizeASan() : FunctionPass(ID) {}

        void getAnalysisUsage(AnalysisUsage &AU) const override
        {
            // Add analysis passes
            AU.addRequired<AAResultsWrapperPass>();
	    AU.addRequired<DominatorTreeWrapperPass>();
	    AU.addRequired<PostDominatorTreeWrapperPass>();
            //AU.addRequired<LoopInfoWrapperPass>(); // TODO: try to cleverly hoist instrumentation from loops
        }

	//Source: LLVM version 16
	Instruction *findNearestCommonDominator(DominatorTree &DT, Instruction *I1, Instruction *I2)
	{
		BasicBlock *BB1 = I1->getParent();
		BasicBlock *BB2 = I2->getParent();
		if(BB1 == BB2)
		{
			return I1->comesBefore(I2) ? I1 : I2;
		}
		if(!DT.isReachableFromEntry(BB2))
		{
			return I1;
		}
		if(!DT.isReachableFromEntry(BB1))
		{
			return I2;
		}
		BasicBlock *DomBB = DT.findNearestCommonDominator(BB1, BB2);
		if(BB1 == DomBB)
		{
			return I1;
		}
		if(BB2 == DomBB)
		{
			return I2;
		}
		return DomBB->getTerminator();
	}

	bool ptr_inst(Instruction* inst)
	{
		if(!inst->getType()->isVoidTy() && !inst->getType()->isPointerTy())
		{
			return false;
		}
		for(unsigned i = 0; i < inst->getNumOperands(); ++i)
		{
			if(!inst->getOperand(i)->getType()->isPointerTy())
			{
				return false;
			}
		}
		return true;
	}

	/*int find_group(AliasAnalysis &AAResult, std::vector<std::list<Instruction*>> &mem_group, MemoryLocation &memloc)
	{
		for(unsigned i = 0; i<mem_group.size(); ++i)
		{
			for(auto &inst: mem_group[i])
			{
				MemoryLocation loc = MemoryLocation::get(inst);
				if(inst == memloc.Ptr || AAResult.alias(loc, memloc) == AliasResult::MustAlias)
				{
					return i;
				}				
			}
		}
		return -1;
	}*/

        bool runOnFunction(Function &F) override
        {
		errs() << "Running OptimizeASan pass on ";
		errs().write_escaped(F.getName()) << '\n';
		
		AAResults &AAResult = getAnalysis<AAResultsWrapperPass>().getAAResults();
		DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
		PostDominatorTree &PDT = getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
		std::vector<std::list<Instruction*>> mem_group;
		std::unordered_map<Value*, int> ptr_group;
		

		for(BasicBlock &BB : F)
		{
			for(Instruction &I : BB)
			{
				//THIS IS TERRIBLE
				if(I.getOpcode() == Instruction::Load)
				{
					Value *addr = I.getOperand(0);
					if(ptr_group.find(addr) == ptr_group.end() || ptr_group[addr] == -1)
					{
						std::list<Instruction*> y;
						y.push_back(&I);
						mem_group.push_back(y);
					}
					else
					{
						mem_group[ptr_group[addr]].push_back(&I);
						if(I.getType()->isPointerTy())
						{
							ptr_group[&I] = ptr_group[addr];
						}
					}
				}
				else if(I.getOpcode() == Instruction::Store)
				{
					Value *p1 = I.getOperand(0);
					Value *p2 = I.getOperand(1);
				
					bool foundP1 = ptr_group.find(p1) != ptr_group.end();
					bool foundP2 = ptr_group.find(p2) != ptr_group.end();

					if(!foundP1 && !foundP2)
					{
						std::list<Instruction*> y;
						y.push_back(&I);
						ptr_group[p2] = mem_group.size();
						mem_group.push_back(y);
					}
					else if(!foundP1 && !p1->getType()->isPointerTy() && foundP2 && ptr_group[p2] != -1)
					{
						mem_group[ptr_group[p2]].push_back(&I);
					}
					else if(foundP1 && ptr_group[p1] != -1 && !foundP2)
					{
						mem_group[ptr_group[p1]].push_back(&I);
						ptr_group[p2] = ptr_group[p1];
					}
					else if(foundP1 && ptr_group[p1] != -1 && foundP2 && ptr_group[p2] == ptr_group[p1])
					{
						mem_group[ptr_group[p1]].push_back(&I);
					}
					else
					{
						ptr_group[p2] = -1;
					}
				}
			}
		}
		
		/*for(BasicBlock &BB : F)
		{
			for(Instruction &inst : BB)
			{
				Optional<MemoryLocation> memlocopt = MemoryLocation::getOrNone(&inst);
				if(memlocopt != None)

				{
					MemoryLocation memloc = *memlocopt;
					int group = find_group(AAResult, mem_group, memloc);
					if(group != -1)
					{
						mem_group[group].push_back(&inst);
					}
					else
					{
						std::list<Instruction*> y;
						y.push_back(&inst);
						mem_group.push_back(y);
					}
				}
			}
		}*/

		errs() << "Size of mem_group=" << mem_group.size() << "\n";
		for(auto &list : mem_group)
		{
			errs() << "Size of val=" << list.size() << "\n";
			auto it = list.begin();
			/*while(it != list.end())
			{
				if(ptr_inst(*it))
				{
					it = list.erase(it);
				}
				else
				{
					++it;
				}
			}*/
			it = list.begin();
			while(it != list.end())
			{
				(*it)->print(errs());
				errs() << "\n";
				++it;
			}

		}

		for(auto &list : mem_group)
		{
			Instruction *common_dominator = list.front();
			unsigned max_width = 32;
			if(list.size() <= 1)
			{
				continue;
			}
			for(auto &inst : list)
			{
				common_dominator = findNearestCommonDominator(DT, common_dominator, inst);

				Type *t;
				if(inst->getOpcode() == Instruction::Load)
				{
					t = inst->getType();
				}
				else
				{
					t = inst->getOperand(0)->getType();
				}
				if(t->isIntegerTy())
				{
					unsigned width = t->getIntegerBitWidth();
					if(max_width < width)
					{
						max_width = width;
					}
				}
				else if(t->isFloatTy() && max_width < 32)
				{
					max_width = 32;
				}
				else if(t->isDoubleTy() && max_width < 64)
				{
					max_width = 64;
				}
				
				LLVMContext &context = inst->getContext();
				MDNode *nosanitize = MDNode::get(context, MDString::get(context, "nosanitize"));
				inst->setMetadata(LLVMContext::MD_nosanitize, nosanitize);
			}
			if(max_width != 0)
			{
				LLVMContext &context = common_dominator->getContext();
				Type *ty = Type::getIntNTy(context, max_width);
				Instruction *front = list.front();
				if(front->getOpcode() == Instruction::Load)
				{
					Value *ptr = front->getOperand(0);
					new LoadInst(ty, ptr, Twine(""), common_dominator);
				}
				else
				{
					if(front->getOperand(0)->getType()->isPointerTy())
					{
						Value *ptr = front->getOperand(0);
						new LoadInst(ty, ptr, Twine(""), common_dominator);	
					}
					else if(!front->getOperand(0)->hasName())
					{
						Value *ptr = front->getOperand(1);
						new LoadInst(ty, ptr, Twine(""), common_dominator);
					}
				}
				errs() << "CD=\n";
				common_dominator->print(errs());
				errs() << "\n";
			}
		}

		
		/*
		LoopInfoWrapperPass &LIWP = getAnalysis<LoopInfoWrapperPass>(F);
		LoopInfo &LI = LIWP.getLoopInfo();

		for (Loop *L : LI)
		{
		}
		*/


		return true;
        }
    };
}

char OptimizeASan::ID = 0;
static RegisterPass<OptimizeASan> X("optimize_asan", "Optimize ASan", false, false);
