#include <vector>
#include <list>
#include <utility>
#include <unordered_set>
#include <unordered_map>
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
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/IVDescriptors.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Transforms/Utils/Cloning.h"

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
			AU.addRequired<BranchProbabilityInfoWrapperPass>();
			AU.addRequired<LoopInfoWrapperPass>();
			AU.addRequired<DependenceAnalysisWrapperPass>();
		}

		void eliminate_mem(std::unordered_map<Value *, int> &ptr_group, int m)
		{
			for (auto &[v, x] : ptr_group)
			{
				if (x == m)
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

		BasicBlock *getLikelySuccessor(BasicBlock *bb)
		{
			BranchProbabilityInfo &bpi = getAnalysis<BranchProbabilityInfoWrapperPass>().getBPI();

			BranchProbability maxProbability;
			BasicBlock *likelySucc = nullptr;

			for (BasicBlock *succ : successors(bb))
			{
				BranchProbability probability = bpi.getEdgeProbability(bb, succ);
				if (!likelySucc || probability >= maxProbability)
				{
					maxProbability = probability;
					likelySucc = succ;
				}
			}

			if (maxProbability >= BranchProbability(4, 5))
			{
				return likelySucc;
			}
			else
			{
				return nullptr;
			}
		}

		// return A - B
		template <typename T>
		std::vector<T> setDifference(std::vector<T> &A, std::vector<T> &B)
		{
			std::unordered_set<T> BSet(B.begin(), B.end());
			std::vector<T> diff;
			for (const T &val : A)
			{
				if (BSet.find(val) == BSet.end())
				{
					diff.push_back(val);
				}
			}
			return diff;
		}

		// return A + B
		template <typename Base, typename DerivedA, typename DerivedB>
		std::vector<Base> setUnion(const std::vector<DerivedA> &A, const std::vector<DerivedB> &B)
		{
			std::unordered_set<Base> union_set;
			for (const DerivedA &V : A)
			{
				union_set.insert(static_cast<Base>(V));
			}
			for (const DerivedB &V : B)
			{
				union_set.insert(static_cast<Base>(V));
			}
			std::vector<Base> vec(union_set.begin(), union_set.end());
			return vec;
		}

		std::vector<LoadInst *> getLoads(std::vector<BasicBlock *> &blocks)
		{
			std::vector<LoadInst *> loads;
			for (BasicBlock *bb : blocks)
			{
				for (Instruction &inst : *bb)
				{
					LoadInst *loadInst = dyn_cast<LoadInst>(&inst);
					if (loadInst)
					{
						loads.push_back(loadInst);
					}
				}
			}
			return loads;
		}

		std::vector<StoreInst *> getStores(std::vector<BasicBlock *> &blocks)
		{
			std::vector<StoreInst *> stores;
			for (BasicBlock *bb : blocks)
			{
				for (Instruction &inst : *bb)
				{
					StoreInst *storeInst = dyn_cast<StoreInst>(&inst);
					if (storeInst)
					{
						stores.push_back(storeInst);
					}
				}
			}
			return stores;
		}

		Instruction *getDef(Value *V, Function &F)
		{
			for (BasicBlock &BB : F)
			{
				for (Instruction &I : BB)
				{
					if (dyn_cast<Value>(&I) == V)
					{
						return &I;
					}
				}
			}
			return nullptr;
		}

		std::vector<Instruction *> getDeps(Instruction *inst, Function &F)
		{
			std::unordered_set<Instruction *> deps;
			for (BasicBlock &BB : F)
			{
				for (Instruction &I : BB)
				{
					// follow use-chains and check if there's a path to inst
					std::unordered_set<Instruction *> visited;
					std::vector<Instruction *> stack;
					stack.push_back(&I);

					while (!stack.empty())
					{
						Instruction *top = stack.back();
						stack.pop_back();
						if (top == inst)
						{
							// we found a path to inst
							deps.insert(&I);
							break;
						}
						if (!visited.insert(top).second)
							continue;
						for (User *U : top->users())
						{
							Instruction *userInst = dyn_cast<Instruction>(U);
							if (!userInst)
								continue;
							stack.push_back(userInst);
						}
					}
				}
			}
			return std::vector<Instruction *>(deps.begin(), deps.end());
		}

		void removeBlocksFromPhi(PHINode *phi, BasicBlock *block)
		{
			for (unsigned int i = 0; i < phi->getNumIncomingValues(); ++i)
			{
				if (phi->getIncomingBlock(i) == block)
				{
					phi->removeIncomingValue(i);
					--i;
				}
			}
		}

		void frequentPathOptimization(Function &F)
		{
			/**
			 * Loop optimization: This optimization will move ASan
			 * instrumentation code off of the frequent path and onto the
			 * infrequent path for basic loops.
			 *
			 * We also need to unroll the loop once for correctness (so we can
			 * always instrument the first access).
			 */

			LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

			LLVMContext &context = F.getContext();
			MDNode *nosanitize = MDNode::get(context, MDString::get(context, "nosanitize"));

			for (Loop *L : LI)
			{
				BasicBlock *header = L->getHeader();

				// capture all BBs in loop
				std::vector<BasicBlock *> loopBlocks;
				{
					std::unordered_set<BasicBlock *> visited;
					std::vector<BasicBlock *> stack;
					stack.push_back(header);
					while (!stack.empty())
					{
						BasicBlock *top = stack.back();
						stack.pop_back();
						if (!visited.insert(top).second)
						{
							continue;
						}
						for (BasicBlock *succ : successors(top))
						{
							if (L->contains(succ))
							{
								stack.push_back(succ);
							}
						}
					}
					loopBlocks = std::vector<BasicBlock *>(visited.begin(), visited.end());
				}

				// populate trace by following frequent path
				std::vector<BasicBlock *> traceBlocks;
				{
					BasicBlock *curr = header;
					do
					{
						assert(L->contains(curr));
						traceBlocks.push_back(curr);
						curr = getLikelySuccessor(curr);
					} while (curr != header);
				}

				std::vector<BasicBlock *> infrequentBlocks = setDifference(loopBlocks, traceBlocks);

				for (BasicBlock *block : traceBlocks)
				{
					for (Instruction &I : *block)
					{
						I.setMetadata("TRACE", MDNode::get(context, MDString::get(context, "TRACE")));
					}
				}

				for (BasicBlock *block : infrequentBlocks)
				{
					for (Instruction &I : *block)
					{
						I.setMetadata("INFREQ", MDNode::get(context, MDString::get(context, "INFREQ")));
					}
				}

				// find memory instructions in trace whose addresses depend on infrequently-changed addresses
				std::vector<Instruction *> memInsts = setUnion<Instruction *>(getStores(traceBlocks), getLoads(traceBlocks));
				std::unordered_set<BasicBlock *> infreqDepBlocks;
				std::unordered_set<Instruction *> noSanitizeMemInsts;

				for (Instruction *memInst : memInsts)
				{
					Value *ptr = nullptr;
					if (StoreInst *store = dyn_cast<StoreInst>(memInst))
					{
						ptr = store->getPointerOperand();
					}
					else if (LoadInst *load = dyn_cast<LoadInst>(memInst))
					{
						ptr = load->getPointerOperand();
					}

					Instruction *ptrInst = getDef(ptr, F);
					if (!ptrInst)
						continue;

					errs() << "Found memory instruction\n";
					errs() << *memInst << "\n";

					// check if the ptrInst dependends on infrequent path
					Instruction *infreqDep = nullptr;
					std::vector<Instruction *> deps = getDeps(ptrInst, F);
					for (Instruction *dep : deps)
					{
						errs() << "Dep: " << *dep << "\n";
						bool isInfreq = false;
						for (BasicBlock *infreqBlock : infrequentBlocks)
						{
							if (dep->getParent() == infreqBlock)
							{
								isInfreq = true;
								break;
							}
						}

						if (isInfreq)
						{
							infreqDep = dep;
							break;
						}
					}
					if (!infreqDep)
						continue;

					// only perform the optimization if the dependency is in a block immediately preceding the memory instruction
					// the dependency block also needs to only have one successor (so we can duplicate the successor with the instrumentation)
					BasicBlock *depBlock = infreqDep->getParent();
					BasicBlock *memBlock = memInst->getParent();
					if (depBlock->getSingleSuccessor() != memBlock)
					{
						errs() << "Optimization cancelled because of successor requirement\n";
						continue;
					}

					errs() << "Performing frequent-path loop optimization\n";

					infreqDepBlocks.insert(depBlock);
					noSanitizeMemInsts.insert(memInst);
				}

				// re-add instrumentation for infreq deps by duplicating their successors
				for (BasicBlock *depBlock : infreqDepBlocks)
				{
					ValueToValueMapTy vmap;

					BasicBlock *memBlock = depBlock->getSingleSuccessor();
					if (memBlock)
						continue;
					assert(memBlock);

					BasicBlock *dupMemBlock = CloneBasicBlock(memBlock, vmap, "", &F);

					// make depBlock branch into dupMemBlock
					BranchInst *br = dyn_cast<BranchInst>(depBlock->getTerminator());
					for (unsigned int i = 0; i < br->getNumSuccessors(); ++i)
					{
						if (br->getSuccessor(i) == memBlock)
						{
							br->setSuccessor(i, dupMemBlock);
						}
					}

					// fix phi stuff in dupMemBlock
					for (Instruction &I : *dupMemBlock)
					{
						if (PHINode *phi = dyn_cast<PHINode>(&I))
						{
							errs() << "PHI BEFORE: " << *phi << "\n";
							for (BasicBlock *pred : predecessors(memBlock))
							{
								if (pred != depBlock)
								{
									removeBlocksFromPhi(phi, pred);
								}
							}
							errs() << "PHI AFTER: " << *phi << "\n";
						}
					}

					// fix phi stuff in successors
					for (BasicBlock *succ : successors(dupMemBlock))
					{
						for (Instruction &I : *succ)
						{
							if (PHINode *phi = dyn_cast<PHINode>(&I))
							{
								Value *originalValue = phi->getIncomingValueForBlock(memBlock);
								Value *mappedValue = vmap[originalValue];
								phi->addIncoming(mappedValue, dupMemBlock);
							}
						}
					}

					// iterate through successors
					/* auto it = std::next(block->getIterator());
					BasicBlock *lastBlock = *(L->getBlocks().rbegin());
					while (&*it != &*lastBlock)
					{
						BasicBlock *curr = &*it;
						BasicBlock *clone = CloneBasicBlock(curr, vmap);
						// curr->getSinglePredecessor()->replaceSuccessorsPhiUsesWith();

						// replace all branches into curr into branches into clone
						for (BasicBlock *pred : predecessors(curr))
						{
							BranchInst *br = dyn_cast<BranchInst>(pred->getTerminator());
							assert(br);
							for (unsigned int i = 0; i < br->getNumSuccessors(); ++i)
							{
							}
						}
						++it;
					} */
				}

				// disable instrumentation on selected mem insts
				for (Instruction *inst : noSanitizeMemInsts)
				{
					// inst->setMetadata(LLVMContext::MD_nosanitize, nosanitize);
				}
			}
		}

		void invariantAddressOptimization(Function &F)
		{
			/**
			 * Invariant address optimization: If a memory instruction is
			 * issued to an invariant address w.r.t. the loop, we hoist the
			 * check into the preheader.
			 *
			 * This is useful because classical optimizations can't hoist
			 * things like stores, and we also don't want to check shadow mem
			 * each time.
			 */

			LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

			LLVMContext &context = F.getContext();
			MDNode *nosanitize = MDNode::get(context, MDString::get(context, "nosanitize"));

			for (Loop *L : LI)
			{
				// TODO
			}
		}

		bool runOnFunction(Function &F) override
		{
			errs() << "Running OptimizeASan pass on ";
			errs().write_escaped(F.getName()) << '\n';

			// AAResults &AAResult = getAnalysis<AAResultsWrapperPass>().getAAResults();
			DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
			ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
			LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
			DependenceInfo &DI = getAnalysis<llvm::DependenceAnalysisWrapperPass>().getDI();

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
							for (auto u : gep->users())
							{
								if (auto inst = dyn_cast<Instruction>(u))
								{
									if (inst->getOpcode() == Instruction::Load || inst->getOpcode() == Instruction::Store)
									{
										errs() << "Setting NOSANITIZE\n";
										inst->setMetadata(LLVMContext::MD_nosanitize, nosanitize);
									}
								}
							}
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
					errs() << "Setting NOSANITIZE\n";
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

			frequentPathOptimization(F);
			invariantAddressOptimization(F);

			return true;
		}
	};
}

char OptimizeASan::ID = 0;
static RegisterPass<OptimizeASan> X("optimize_asan", "Optimize ASan", false, false);
