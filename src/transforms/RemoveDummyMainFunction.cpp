/**
 * LLVM transformation pass to resolve indirect calls
 *
 * The transformation performs "devirtualization" which consists of
 * looking for indirect function calls and transforming them into a
 * switch statement that selects one of several direct function calls
 * to execute. Devirtualization happens if a pointer analysis can
 * resolve the indirect calls and compute all possible callees.
 **/


#include "llvm/Pass.h"
//#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "seadsa/CompleteCallGraph.hh"
#include "seadsa/InitializePasses.hh"

namespace previrt {
    namespace transforms {

        using namespace llvm;

        /** 
         ** Resolve indirect calls by one direct call for possible callee
         ** function 
         **/
        class RemoveDummyMainFunction : public ModulePass {
            public:
                static char ID;

                RemoveDummyMainFunction() : ModulePass(ID) {
                    // Initialize sea-dsa pass
                    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
                    llvm::initializeCompleteCallGraphPass(Registry);
                }

                virtual bool runOnModule(Module &M) override {

                    if (!M.getFunction ("main")) { 
                        errs () << "RemoveDummyMainFunction: Main doesn't exist already \n";
                        return false;
                    }


                    Function* Main = M.getFunction("main");

                    if(!Main->hasMetadata() || !(Main->getMetadata("dummy.metadata"))) {
                        errs()<<"Module has no Dummy Main Function, exiting...\n";
                        return false;
                    }

                    Main->eraseFromParent();

                    return true;
                }

                virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
                    // AU.addRequired<CallGraphWrapperPass>();
                    AU.addRequired<seadsa::CompleteCallGraph>();
                    // FIXME: DevirtualizeFunctions does not fully update the call
                    // graph so we don't claim it's preserved.
                    // AU.setPreservesAll();
                    // AU.addPreserved<CallGraphWrapperPass>();
                }

                virtual StringRef getPassName() const override {
                    return "Remove Main Function from Bitcode";
                }
        };

        char RemoveDummyMainFunction::ID = 0;

    } // end namespace
} // end namespace

static llvm::RegisterPass<previrt::transforms::RemoveDummyMainFunction>
X("MainToLib",
        "Converty a program bitcode into a library module by removing the main function",
        false,
        false);

