/**
 * LLVM transformation pass to resolve indirect calls
 *
 * The transformation performs "devirtualization" which consists of
 * looking for indirect function calls and transforming them into a
 * switch statement that selects one of several direct function calls
 * to execute. Devirtualization happens if a pointer analysis can
 * resolve the indirect calls and compute all possible callees.
 **/

#include <iostream>
#include <fstream>

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
        class GetExternalFunctions : public ModulePass {
            public:
                static char ID;

                std::string getModuleName(std::string path){
                    return path.substr(path.find_last_of("/")+1);
                }

                GetExternalFunctions() : ModulePass(ID) {
                    // Initialize sea-dsa pass
                    //llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
                    //llvm::initializeCompleteCallGraphPass(Registry);
                }

                virtual bool runOnModule(Module &M) override {

                    std::string ModuleName = getModuleName(M.getName());
                    errs()<<"Module Name:\t"<<ModuleName<<"\n";
                    Function* Main = M.getFunction("main");

                    // Only run for bitcode with a main function
                    if(!Main){
                        return false;
                    }

                    if(Main && Main->hasMetadata() && (Main->getMetadata("dummy.metadata"))) {
                        errs()<<"GetExternalFunctions:\tModule is not a true program, exiting...\n";
                        return false;
                    }


                    std::ofstream write_file;
                    write_file.open("external.functions."+ModuleName);
                    write_file << "{ \"functions\": [";
                    std::vector<std::string> functions;

                    for(Function &F: M){
                        if(F.isDeclaration()){
                            errs()<<F.getName()<<":\tDeclaration"<<"\n";
                            functions.push_back(F.getName());
                        }
                    }


                    for(int i; i < functions.size(); ++i){
                        write_file << "\""<<(functions[i])<<"\"";

                        if(i != (functions.size()-1)){
                            write_file <<", ";
                        }
                    }

                    write_file << "] }";

                    write_file.close();


                    return false;
                }

                virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
                    // AU.addRequired<CallGraphWrapperPass>();
                    //AU.addRequired<seadsa::CompleteCallGraph>();
                    // FIXME: DevirtualizeFunctions does not fully update the call
                    // graph so we don't claim it's preserved.
                    // AU.setPreservesAll();
                    // AU.addPreserved<CallGraphWrapperPass>();
                }

                virtual StringRef getPassName() const override {
                    return "Remove Main Function from Bitcode";
                }
        };

        char GetExternalFunctions::ID = 0;

    } // end namespace
} // end namespace

static llvm::RegisterPass<previrt::transforms::GetExternalFunctions>
X("GetExternal",
        "Get the name of all external functions within a module",
        false,
        false);

