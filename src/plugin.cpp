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

#include <serializer/json/json.h>
#include <serializer/string_escaper.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

using namespace clang;

typedef std::pair<std::string, std::string> AV;
typedef std::vector<AV> AVV;
typedef std::map<std::string, AVV> EAV;
typedef std::map<std::string, std::string> EV;
typedef std::map<std::string, PresumedLoc> LM;

template <>
struct format_override<AV, JsonOutStream> {
  template <typename Stream>
  static void format(Stream& out, const AV& obj) {
    out << "{\"" << obj.first << "\":" << "\"" << escape_string(obj.second) << "\"}";
  }
};

namespace {

EAV StructDefs;
LM StructLocs;

EAV FuncDefs;
LM FuncLocs;

EAV FunctionTypedefs;
EV SimpleTypedefs;

EAV GlobalDefs;
LM GlobalLocs;

class PrintFunctionsVisitor : public RecursiveASTVisitor<PrintFunctionsVisitor> {
public:
  explicit PrintFunctionsVisitor(ASTContext& Context)
    : Context(Context) { }

  typedef RecursiveASTVisitor<PrintFunctionsVisitor> Default;

  std::string currentRecord;
  std::string currentFunc;
  std::string currentTypedef;

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

    if (FuncDefs.find(name) != FuncDefs.end())
      return false;

    inFunction = true;
    currentFunc = name;
    FuncDefs[currentFunc].push_back(std::make_pair("return", typeName));
    FuncLocs[name] = getLocation(decl);
    return true;
  }

  bool TraverseFunctionDecl(FunctionDecl *decl) {
    return Default::TraverseFunctionDecl(decl);
    //return decl->isThisDeclarationADefinition() ?
    //  Default::TraverseFunctionDecl(decl) : true;
  }

  bool VisitParmVarDecl(ParmVarDecl *decl) {
    auto name = decl->getNameAsString();
    auto type = decl->getOriginalType();
    auto typeName = type.getAsString();
    //std::cout << "  " << name << " : " << typeName << std::endl;
    if (inFunction)
      FuncDefs[currentFunc].push_back(std::make_pair(name, typeName));
    else
      FunctionTypedefs[currentTypedef].push_back(std::make_pair(name, typeName));
    return true;
  }

  bool VisitTypedefDecl(TypedefDecl *decl) {
    inFunction = false;
    auto name = decl->getNameAsString();
    currentTypedef = name;
    auto type = decl->getUnderlyingType();
    if (type->isFunctionPointerType()) {
      auto ftype = type->getAs<PointerType>()->getPointeeType()->getAs<FunctionType>();
      FunctionTypedefs[currentTypedef].push_back(std::make_pair("return", ftype->getResultType().getAsString()));
    }
    else {
      SimpleTypedefs[name] = type.getAsString();
    }
    return true;
  }

private:
  bool inFunction = false;
  ASTContext& Context;
};

class PrintFunctionsConsumer : public ASTConsumer {
public:
  explicit PrintFunctionsConsumer(ASTContext& Context)
    : Visitor(Context) { }

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
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
        GlobalDefs[name].push_back(std::make_pair(getTokenName(kind), cat.str()));
      }
      else if (info->isFunctionLike()) {
        GlobalDefs[name].push_back(std::make_pair("mixin", cat.str()));
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
bool print_macros = false;
bool print_typedefs = false;
bool print_all = false;
bool print_json = false;
bool print_text = false;
typedef std::map<std::string, std::function<void()>> CallbackMap;

CallbackMap initArgumentActions() {
  CallbackMap ArgumentActions;
  ArgumentActions["structs"] = [](){ print_structs = true; };
  ArgumentActions["functions"] = [](){ print_functions = true; };
  ArgumentActions["macros"] = [](){ print_macros = true; };
  ArgumentActions["typedefs"] = [](){ print_typedefs = true; };
  ArgumentActions["all"] = [](){ print_all = true; };
  ArgumentActions["json"] = [](){ print_json = true; };
  ArgumentActions["text"] = [](){ print_text = true; };
  ArgumentActions["default"] = [](){};
  ArgumentActions["help"] = [](){ std::cout << "[structs|functions|help]" << std::endl; };
  return ArgumentActions;
}

CallbackMap ArgumentActions = initArgumentActions();

void printStructs() {
  if (print_text) {
    for (auto& record : StructDefs) {
      //std::cout << StructLocs[record.first].getFilename() << std::endl;
      std::cout << "struct " << record.first << " { \n";
      for (auto& field : record.second) {
        std::cout << "  " << field.second << " " << field.first << ";" << std::endl;
      }
      std::cout << "};\n\n";
    }
  }

  if (print_json) {
    std::ofstream structs_file("structs.json");
    JsonOutStream js(structs_file);
    format(js, StructDefs);
  }
}

void printFuncs() {
  if (print_text) {
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

  if (print_json) {
    std::ofstream functions_file("functions.json");
    JsonOutStream js(functions_file);
    format(js, FuncDefs);
  }
}

void printTypedefs() {
  if (print_text) {
    for (auto& td : FunctionTypedefs) {
      std::cout << td.second[0].second << " " << td.first << "(";
      bool first = true;
      std::string sep = "";
      for (auto& param : td.second) {
        if (first) { first = false; continue; }
        std::cout << sep << param.second << " " << param.first;
        sep = ", ";
      }
      std::cout << ");\n\n";
    }

    for (auto& td : SimpleTypedefs) {
      std::cout << td.first << " -> " << td.second << std::endl;
    }
  }

  if (print_json) {
    std::ofstream typedefs_file("typedefs.json");
    JsonOutStream js(typedefs_file);
    typedefs_file << "{\"simple\":";
    format(js, SimpleTypedefs);
    typedefs_file << ",\"complex\":";
    format(js, FunctionTypedefs);
    typedefs_file << "}";
  }
}

void printMacros() {
  if (print_text) {
    for (auto& macro : GlobalDefs) {
      auto type = macro.second[0].first;
      if (type == "numeric_constant" || type == "string_literal")
        std::cout << "define " << macro.first << ": " << macro.second[0].first << std::endl;
    }
  }

  if (print_json) {
    std::ofstream macros_file("macros.json");
    JsonOutStream js(macros_file);
    format(js, GlobalDefs);
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
    if (print_structs || print_all)
      printStructs();
    if (print_functions || print_all)
      printFuncs();
    if (print_macros || print_all)
      printMacros();
    if (print_typedefs || print_all)
      printTypedefs();
    std::cout << "\n" << std::endl;
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {

    for (auto& arg : args) {
      auto action = ArgumentActions.find(arg);
      if (action == ArgumentActions.end()) {
        DiagnosticsEngine &D = CI.getDiagnostics();
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error, "invalid argument '" + arg + "'");
        D.Report(DiagID);

        std::string opts;
        for (auto& action : ArgumentActions)
          opts += action.first + " ";
        DiagID = D.getCustomDiagID(DiagnosticsEngine::Error, "Allowed options: " + opts);
        D.Report(DiagID);
        return false;
      }
      else {
        action->second();
      }
    }

    return true;
  }
};

}

static FrontendPluginRegistry::Add<PrintFunctionNamesAction>
X("cleng", "print type info");
