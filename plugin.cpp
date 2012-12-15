#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/AST.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Token.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/AST/ASTContext.h"
#include "clang/Lex/Preprocessor.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>

using namespace clang;

namespace {

typedef std::map<std::string, std::vector<std::pair<std::string, std::string>>> EAV;
typedef std::map<std::string, PresumedLoc> LM;

EAV StructDefs;
LM StructLocs;

EAV FuncDefs;
LM FuncLocs;

EAV GlobalDefs;
LM GlobalLocs;

class PrintFunctionsVisitor : public RecursiveASTVisitor<PrintFunctionsVisitor> {
public:
  explicit PrintFunctionsVisitor(ASTContext& Context)
    : Context(Context) { }

  typedef RecursiveASTVisitor<PrintFunctionsVisitor> Default;

  std::string currentRecord;
  std::string currentFunc;

  template <typename Type>
  PresumedLoc getLocation(Type* t) {
    return Context.getSourceManager().getPresumedLoc(t->getLocation());
  }

  bool VisitRecordDecl(RecordDecl *decl) {
    auto name = decl->getNameAsString();
    //std::cout << name << std::endl;
    currentRecord = name;
    StructLocs[name] = getLocation(decl);
    return true;
  }

  bool TraverseRecordDecl(RecordDecl *decl) {
    bool result = !decl->getIdentifier() ? true : Default::TraverseRecordDecl(decl);
    currentRecord = decl->getNameAsString();
    return result;
  }

  bool VisitFieldDecl(FieldDecl *decl) {
    auto name = decl->getNameAsString();
    auto type = decl->getTypeSourceInfo()->getType();
    auto typeName = type.getAsString();
    //std::cout << "  " << name << " : " << typeName << std::endl;
    StructDefs[currentRecord].push_back(std::make_pair(name, typeName));
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *decl) {
    auto name = decl->getNameInfo().getName().getAsString();
    auto type = decl->getResultType();
    auto typeName = type.getAsString();
    //std::cout << name << " : " << typeName << std::endl;
    currentFunc = name;
    FuncDefs[currentFunc].push_back(std::make_pair("Return", typeName));
    FuncLocs[name] = getLocation(decl);
    return true;
  }

  bool TraverseFunctionDecl(FunctionDecl *decl) {
    return decl->isThisDeclarationADefinition() ?
      Default::TraverseFunctionDecl(decl) : true;
  }

  bool VisitParmVarDecl(ParmVarDecl *decl) {
    auto name = decl->getNameAsString();
    auto type = decl->getOriginalType();
    auto typeName = type.getAsString();
    //std::cout << "  " << name << " : " << typeName << std::endl;
    FuncDefs[currentFunc].push_back(std::make_pair(name, typeName));
    return true;
  }

private:
  ASTContext& Context;
};

class PrintFunctionsConsumer : public ASTConsumer {
public:
  explicit PrintFunctionsConsumer(ASTContext& Context)
    : Visitor(Context) { }

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

  virtual void GetPPMutationListener() {

  }

private:
  PrintFunctionsVisitor Visitor;
};

class PreprocessorCallbacks : public PPCallbacks {
public:
  PreprocessorCallbacks(Preprocessor& processor) : _processor(processor) { }

  virtual void MacroDefined(const Token& identifier, const MacroInfo* info) {
    auto id = identifier.getIdentifierInfo();
    auto name = _processor.getSpelling(identifier);

    /*
    std::cout << name;
    std::for_each(info->tokens_begin(), info->tokens_end(), [this](const Token& name) { std::cout << " " << _processor.getSpelling(name); });
    std::cout << std::endl;
    */

    std::stringstream cat;
    std::for_each(info->tokens_begin(), info->tokens_end(), [&cat, this](const Token& name) { cat << _processor.getSpelling(name); });

    if (info->getNumTokens() > 0) {
      if (info->isObjectLike()) {
        auto kind = info->getReplacementToken(0).getKind();
        GlobalDefs[name].push_back(std::make_pair(cat.str(), getTokenName(kind)));
      }
      else if (info->isFunctionLike()) {
        GlobalDefs[name].push_back(std::make_pair(cat.str(), "mixin"));
      }
    }
  }

  virtual void FileChanged(SourceLocation loc, FileChangeReason reason, SrcMgr::CharacteristicKind type, FileID id) {
    //std::cout << _processor.getSourceManager().getBufferName(loc) << std::endl;
  }

  virtual void EndOfMainFile() {
    //std::cout << "Done" << std::endl;
  }

private:
  Preprocessor& _processor;
};

bool print_structs = false;
bool print_functions = false;

void printStructs() {
  for (auto& record : StructDefs) {
    //std::cout << StructLocs[record.first].getFilename() << std::endl;
    std::cout << "struct " << record.first << " { \n";
    for (auto& field : record.second) {
      std::cout << "  " << field.second << " " << field.first << ";" << std::endl;
    }
    std::cout << "};\n\n";
  }
}

void printFuncs() {
  for (auto& func : FuncDefs) {
    std::cout << func.second[0].second << " " << func.first << "(";
    bool first = true;
    std::string sep = "";
    for (auto& param : func.second) {
      if (first) { first = false; continue; }
      std::cout << sep << param.second << " " << param.first;
      sep = ", ";
    }
    std::cout << ");\n\n";
  }
}

class PrintFunctionNamesAction : public PluginASTAction {
protected:
  virtual ASTConsumer* CreateASTConsumer(CompilerInstance &Compiler, llvm::StringRef InFile) {
    return new PrintFunctionsConsumer(Compiler.getASTContext());
  }

  virtual void ExecuteAction() {
    getCompilerInstance().getPreprocessor().addPPCallbacks(new PreprocessorCallbacks(getCompilerInstance().getPreprocessor()));
    PluginASTAction::ExecuteAction();
    printStructs();
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {

    for (auto& arg : args) {
      if (arg == "structs")
        print_structs = true;
      else if (arg == "functions")
        print_functions = true;
    }
    
    /*
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      llvm::errs() << "PrintFunctionNames arg = " << args[i] << "\n";

      // Example error handling.
      if (args[i] == "-an-error") {
        DiagnosticsEngine &D = CI.getDiagnostics();
        unsigned DiagID = D.getCustomDiagID(
          DiagnosticsEngine::Error, "invalid argument '" + args[i] + "'");
        D.Report(DiagID);
        return false;
      }
    }
    if (args.size() && args[0] == "help")
      PrintHelp(llvm::errs());
    */

    return true;
  }
  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help for PrintFunctionNames plugin goes here\n";
  }

};

}

static FrontendPluginRegistry::Add<PrintFunctionNamesAction>
X("cleng", "print type info");
