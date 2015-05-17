#include <serialization.hpp>
#include <json.hpp>

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include <stdexcept>
#include <functional>

using namespace clang;
using json = nlohmann::json;

namespace {

class JsonASTPrinter : public ASTConsumer {
public:
  explicit JsonASTPrinter(ASTContext& context, json& output) : _context(context), _output(output) { }

  virtual void HandleTranslationUnit(clang::ASTContext& context) {
    //Context.getTranslationUnitDecl();
  }

  virtual bool HandleTopLevelDecl(DeclGroupRef g) {
    for (auto& decl : g) {
      json obj;
      dispatch_decl(obj, *decl, _context);
      _output.push_back(obj);
    }

    return true;
  }

private:
  ASTContext& _context;
  json& _output;
};

class PreprocessorCallbacks : public PPCallbacks {
public:
  PreprocessorCallbacks(Preprocessor& processor, json& output) : _processor(processor), _output(output) { }

  virtual void MacroDefined(const Token& identifier, const MacroDirective* info) {
    json obj;
    visit(obj, identifier, _processor);
    visit(obj, *info, _processor);
    _output.push_back(obj);
  }

  virtual void FileChanged(SourceLocation loc, FileChangeReason reason, SrcMgr::CharacteristicKind type, FileID id) {
    //std::cout << _processor.getSourceManager().getBufferName(loc) << std::endl;
  }

  virtual void EndOfMainFile() {
    //std::cout << "Done" << std::endl;
  }

private:
  Preprocessor& _processor;
  json& _output;
};

typedef std::map<std::string, std::function<void()>> CallbackMap;

CallbackMap initArgumentActions() {
  CallbackMap ArgumentActions;
  ArgumentActions["help"] = [](){ std::cout << "[help]" << std::endl; };
  return ArgumentActions;
}

CallbackMap ArgumentActions = initArgumentActions();

class PrintASTAction : public PluginASTAction {
protected:
  virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::make_unique<JsonASTPrinter>(Compiler.getASTContext(), _output);
  }

  virtual void ExecuteAction() {
    auto& pp = getCompilerInstance().getPreprocessor();
    auto cbs = std::make_unique<PreprocessorCallbacks>(pp, _output);
    pp.addPPCallbacks(std::move(cbs));

    PluginASTAction::ExecuteAction();

    std::ofstream output("output.json");
    output << _output.dump();
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {

    for (auto& arg : args) {
      auto action = ArgumentActions.find(arg);
      if (action == ArgumentActions.end()) {
        DiagnosticsEngine &D = CI.getDiagnostics();
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error, "Invalid argument: %s");
        D.Report(DiagID);

        DiagID = D.getCustomDiagID(DiagnosticsEngine::Error, "%s");
        D.Report(DiagID);
        return false;
      }
      else {
        action->second();
      }
    }

    return true;
  }

  json _output;
};

}

static FrontendPluginRegistry::Add<PrintASTAction>
X("cleng", "print type info");
