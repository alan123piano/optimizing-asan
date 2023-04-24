#include <vector>
#include <list>
#include <utility>
#include <unordered_set>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/IVDescriptors.h"

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
			AU.addRequired<ScalarEvolutionWrapperPass>();
			AU.addRequired<LoopInfoWrapperPass>(); // TODO: try to cleverly hoist instrumentation from loops
		}

		void no_sanitize_gep(Value *gep, MDNode *nosanitize)
		{
			for (auto u : gep->users())
			{
				if (auto inst = dyn_cast<Instruction>(u))
				{
					if (inst->getOpcode() == Instruction::Load || inst->getOpcode() == Instruction::Store)
					{
						inst->setMetadata(LLVMContext::MD_nosanitize, nosanitize);
					}
				}
			}
		}

		void eliminate_mem(std::unordered_map<Value *, int> &ptr_group, int m)
		{
			static char ID;
			OptimizeASan() : FunctionPass(ID) {}

			void getAnalysisUsage(AnalysisUsage & AU) const override
			{
				// Add analysis passes
				AU.addRequired<AAResultsWrapperPass>();
				AU.addRequired<DominatorTreeWrapperPass>();
				AU.addRequired<PostDominatorTreeWrapperPass>();
				AU.addRequired<ScalarEvolutionWrapperPass>();
				AU.addRequired<LoopInfoWrapperPass>(); // TODO: try to cleverly hoist instrumentation from loops
			}

			void eliminate_mem(std::unordered_map<Value *, int> & ptr_group, int m)
			{
				for (auto &[v, x] : ptr_group)
				{
					x = -1;
				}
			}
		}

		unsigned get_width(Instruction *inst)
		{
			Type *t;
			if (inst->getOpcode() == Instruction::Load)
			{
				t = inst->getType();
			}
			else
			{
				t = inst->getOperand(0)->getType();
			}

			if (t->isIntegerTy())
			{
				return t->getIntegerBitWidth();
			}
			else if (t->isFloatTy())
			{
				return 32;
			}
			else if (t->isDoubleTy())
			{
				return 64;
			}
			return 0;
		}

		// Source: LLVM version 16
		Instruction *findNearestCommonDominator(DominatorTree &DT, Instruction *I1, Instruction *I2)
		{
			BasicBlock *BB1 = I1->getParent();
			BasicBlock *BB2 = I2->getParent();
			if (BB1 == BB2)
			{
				return I1->comesBefore(I2) ? I1 : I2;
			}
			if (!DT.isReachableFromEntry(BB2))
			{
				return I1;
			}
			if (!DT.isReachableFromEntry(BB1))
			{
				return I2;
			}
			BasicBlock *DomBB = DT.findNearestCommonDominator(BB1, BB2);
			if (BB1 == DomBB)
			{
				return I1;
			}
			if (BB2 == DomBB)
			{
				return I2;
			}
			return DomBB->getTerminator();
		}

		bool runOnFunction(Function &F) override
		{
			errs() << "Running OptimizeASan pass on ";
			errs().write_escaped(F.getName()) << '\n';

			// AAResults &AAResult = getAnalysis<AAResultsWrapperPass>().getAAResults();
			DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
			ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
			LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

			LLVMContext &context = F.getContext();
			MDNode *nosanitize = MDNode::get(context, MDString::get(context, "nosanitize"));

			for (auto &L : LI)
			{
				if (!L->isCanonical(SE))
				{
					continue;
				}

				BasicBlock *preheader = L->getLoopPreheader();
				long long finalIVVal;
				Optional<Loop::LoopBounds> option = L->getBounds(SE);
				if (option != None)
				{
					Loop::LoopBounds bounds = *option;
					errs() << bounds.getInitialIVValue() << "\n";
					errs() << bounds.getFinalIVValue() << "\n";
					if (auto ci = dyn_cast<ConstantInt>(&bounds.getFinalIVValue()))
					{
						finalIVVal = ci->getSExtValue() - 1;
						errs() << finalIVVal << "\n";
						if (finalIVVal == -1)
						{
							continue;
						}
					}
					else
					{
						continue;
					}
				}
				else
				{
					continue;
				}
				// Just look at the first basic block because it would
				// get complicated with branches
				BasicBlock *BB = *(L->block_begin());
				for (auto &inst : *BB)
				{
					// Give up if there is a branch
					if (auto gep = dyn_cast<GetElementPtrInst>(&inst))
					{
						if (gep->getNumIndices() != 1)
						{
							continue;
						}
						const SCEV *scevGep = SE.getSCEV(gep);
						if (auto scevGepADD = dyn_cast<SCEVAddRecExpr>(scevGep))
						{
							errs() << "woop\n";
							scevGepADD->print(errs());
							errs() << "\n";
							if (scevGepADD->getStart()->getExpressionSize() != 1)
							{
								continue;
							}
							// check invariant
							Value *ptr = gep->getOperand(0);
							ptr->print(errs());
							errs() << "\n";
							if (!L->isLoopInvariant(ptr))
							{
								continue;
							}

							// gives us size
							const SCEV *step = scevGepADD->getStepRecurrence(SE);
							step->print(errs());
							errs() << "\n";

							long long size;
							if (auto ci = dyn_cast<SCEVConstant>(step))
							{
								size = ci->getValue()->getSExtValue();
							}
							else
							{
								continue;
							}

							// sanitize instructions that use gep
							no_sanitize_gep(gep, nosanitize);
							// insert %a = gep i8 p size*finalIVValue
							Type *i32 = Type::getInt32Ty(context);
							Value *offset = ConstantInt::get(i32, size * finalIVVal);
							Type *ty = Type::getInt8Ty(context);
							Value *idx = GetElementPtrInst::CreateInBounds(ty, ptr, ArrayRef(offset), Twine(""), preheader->getTerminator());
							// insert load i8 %a
							new LoadInst(ty, idx, Twine(""), preheader->getTerminator());
						}
						else
						{
							continue;
						}
					}
				}
			}

			unsigned get_width(Instruction * inst)
			{
				Type *t;
				if (inst->getOpcode() == Instruction::Load)
				{
					t = inst->getType();
				}
				else
				{
					t = inst->getOperand(0)->getType();
				}

				if (t->isIntegerTy())
				{
					return t->getIntegerBitWidth();
				}
				else if (t->isFloatTy())
				{
					return 32;
				}
				else if (t->isDoubleTy())
				{
					return 64;
				}
				return 0;
			}

			// Source: LLVM version 16
			Instruction *findNearestCommonDominator(DominatorTree & DT, Instruction * I1, Instruction * I2)
			{
				BasicBlock *BB1 = I1->getParent();
				BasicBlock *BB2 = I2->getParent();
				if (BB1 == BB2)
				{
					return I1->comesBefore(I2) ? I1 : I2;
				}
				if (!DT.isReachableFromEntry(BB2))
				{
					return I1;
				}
				if (!DT.isReachableFromEntry(BB1))
				{
					return I2;
				}
				BasicBlock *DomBB = DT.findNearestCommonDominator(BB1, BB2);
				if (BB1 == DomBB)
				{
					return I1;
				}
				if (BB2 == DomBB)
				{
					return I2;
				}
				return DomBB->getTerminator();
			}

			bool runOnFunction(Function & F) override
			{
				errs() << "Running OptimizeASan pass on ";
				errs().write_escaped(F.getName()) << '\n';

				// AAResults &AAResult = getAnalysis<AAResultsWrapperPass>().getAAResults();
				DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
				ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
				LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

				for (auto &L : LI)
				{
					if (!L->isCanonical(SE))
					{
						continue;
					}
					errs() << "yabo\n";
					for (auto &bb : L->blocks())
					{
						for (auto &inst : *bb)
						{
							if (auto gep = dyn_cast<GetElementPtrInst>(&inst))
							{
								errs() << "yabo3\n";
								if (gep->getNumIndices() != 1)
								{
									continue;
								}
								errs() << "yabo4\n";
								const SCEV *scevGep = SE.getSCEV(gep);
								scevGep->print(errs());
								errs() << "\n";
							}
						}
					}
				}

				// Possible better algorithm
				// It would Use a lot of memory and maybe it is not worth it
				// Forward BFS for BB, where ptr_group[BB][v] = union of ptr_group[P][v] for predecessors P of BB
				// In BB, if new v has never been seen ptr_group[BB][v] = {id}
				// Once at exit BB reverse map Int -> Value
				// Add checks at each Int by finding common dominator of each Value
				// std::unordered_map<BasicBlock*, std::unordered_map<Value*,std::unordered_set<int>>> ptr_group;

				std::vector<std::vector<Instruction *>> mem_group;
				std::unordered_map<Value *, int> ptr_group;

				for (BasicBlock &BB : F)
				{
					for (Instruction &I : BB)
					{
						bool b1 = I.getOpcode() == Instruction::Load;
						bool b2 = I.getOpcode() == Instruction::Store;
						if (b1 || b2)
						{
							Value *addr;
							if (b1)
							{
								addr = I.getOperand(0);
							}
							else
							{
								addr = I.getOperand(1);
							}
							if (ptr_group.find(addr) == ptr_group.end())
							{
								ptr_group[addr] = mem_group.size();
								std::vector<Instruction *> v;
								mem_group.push_back(v);
							}
							if (ptr_group[addr] != -1)
							{
								mem_group[ptr_group[addr]].push_back(&I);
							}
						}
						else if (auto callInst = dyn_cast<CallInst>(&I))
						{
							// callInst->print(errs());
							// errs() << "\n";
							for (auto &v : callInst->args())
							{
								// v->print(errs());
								if (ptr_group.find(v) != ptr_group.end() && ptr_group[v] != -1)
								{
									eliminate_mem(ptr_group, ptr_group[v]);
								}
								// errs() << ", ";
							}
							// errs() << "\n";
						}
					}
				}

				errs() << "Size of mem_group=" << mem_group.size() << "\n";
				for (unsigned i = 0; i < mem_group.size(); ++i)
				{
					errs() << "Size of val=" << mem_group[i].size() << "\n";
					auto it = mem_group[i].begin();
					while (it != mem_group[i].end())
					{
						(*it)->print(errs());
						errs() << "\n";
						++it;
					}
				}

				for (auto &list : mem_group)
				{
					if (list.size() == 1)
					{
						continue;
					}

					Instruction *cdom = list.front();
					for (auto &inst : list)
					{
						cdom = findNearestCommonDominator(DT, inst, cdom);
					}

					unsigned max_width = 0;
					for (auto &inst : list)
					{
						unsigned width = get_width(inst);
						if (max_width < width)
						{
							max_width = width;
						}
						LLVMContext &context = inst->getContext();
						errs() << "ADDING NOSANITIZE\n";
						MDNode *nosanitize = MDNode::get(context, MDString::get(context, "nosanitize"));
						inst->setMetadata(LLVMContext::MD_nosanitize, nosanitize);
					}
					if (max_width != 0)
					{
						LLVMContext &context = cdom->getContext();
						Type *ty = Type::getIntNTy(context, max_width);
						Value *ptr;
						if (list.front()->getOpcode() == Instruction::Load)
						{
							ptr = list.front()->getOperand(0);
						}
						else
						{
							ptr = list.front()->getOperand(1);
						}
						new LoadInst(ty, ptr, Twine(""), cdom);
					}
					else if (auto callInst = dyn_cast<CallInst>(&I))
					{
						// callInst->print(errs());
						// errs() << "\n";
						for (auto &v : callInst->args())
						{
							// v->print(errs());
							if (ptr_group.find(v) != ptr_group.end() && ptr_group[v] != -1)
							{
								ptr_group[v] = -1;
								// eliminate_mem(ptr_group, ptr_group[v]);
							}
							// errs() << ", ";
						}
						// errs() << "\n";
					}
				}
			}

			errs() << "Size of mem_group=" << mem_group.size() << "\n";
			for (unsigned i = 0; i < mem_group.size(); ++i)
			{
				errs() << "Size of val=" << mem_group[i].size() << "\n";
				auto it = mem_group[i].begin();
				while (it != mem_group[i].end())
				{
					(*it)->print(errs());
					errs() << "\n";
					++it;
				}
			}

			for (auto &list : mem_group)
			{
				if (list.size() == 1)
				{
					continue;
				}

				Instruction *cdom = list.front();
				for (auto &inst : list)
				{
					cdom = findNearestCommonDominator(DT, inst, cdom);
				}

				unsigned max_width = 0;
				for (auto &inst : list)
				{
					unsigned width = get_width(inst);
					if (max_width < width)
					{
						max_width = width;
					}
					inst->setMetadata(LLVMContext::MD_nosanitize, nosanitize);
				}
				if (max_width != 0)
				{
					LLVMContext &context = cdom->getContext();
					Type *ty = Type::getIntNTy(context, max_width);
					Value *ptr;
					if (list.front()->getOpcode() == Instruction::Load)
					{
						ptr = list.front()->getOperand(0);
					}
					else
					{
						ptr = list.front()->getOperand(1);
					}
					new LoadInst(ty, ptr, Twine(""), cdom);
				}
			}

			/**
			 * Loop optimization: This optimization will move ASan
			 * instrumentation code off of the frequent path and onto the
			 * infrequent path for basic loops.
			 *
			 * We also need to unroll the loop once for correctness (so we can
			 * always instrument the first access).
			 */

			return true;
		}
	};
}

char OptimizeASan::ID = 0;
static RegisterPass<OptimizeASan> X("optimize_asan", "Optimize ASan", false, false);
