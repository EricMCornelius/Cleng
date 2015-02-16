#include <serializer/json/json.h>
#include <serializer/json/impl.h>
#include <serializer/string_escaper.h>

#include <iostream>
#include <vector>
#include <string>
#include <tuple>

#include <clang-c/Index.h>

json::Value results;
json::Array functions;
json::Array macros;

struct StringHandle {
  StringHandle(CXString str) : data(str) {}
  ~StringHandle() {
    if (clean) {
      clang_disposeString(data);
    }
  }

  StringHandle(const StringHandle&) = delete;
  StringHandle(StringHandle&& other) {
    data = std::move(other.data);
    other.clean = false;
  }

  operator const char*() {
    return clang_getCString(data);
  }

  std::string asString() {
    return std::string(clang_getCString(data));
  }

  bool clean = true;
  CXString data;
  std::vector<std::string> attributes;
};

// adapted from http://clang-developers.42468.n3.nabble.com/Extracting-macro-information-using-libclang-the-C-Interface-to-Clang-td4042648.html
std::tuple<std::string, std::string> translateMacro(CXCursor cursor) {
  std::string name;
  std::string value;
  CXToken* tokens = nullptr;
  unsigned numTokens = 0;
  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

  CXSourceRange range = clang_getCursorExtent(cursor);
  clang_tokenize(tu, range, &tokens, &numTokens);
  for (auto n = 0u; n < numTokens; ++n) {
    auto str = clang_getTokenSpelling(tu, tokens[n]);
    auto ptr = clang_getCString(str);

    if (n == 0) {
      name = ptr;
    }
    else {
      CXTokenKind tokenKind = clang_getTokenKind(tokens[n]);
      if (tokenKind != CXToken_Comment) {
        std::string text = ptr;
        if (!text.empty() && (text[0] != '#'))
          value += text;
      }
    }
    clang_disposeString(str);
  }

  macros.emplace_back(json::Object{
    {"name", name},
    {"value", value}
  });

  auto result = std::make_tuple(name, value);
  clang_disposeTokens(tu, tokens, numTokens);
  return result;
}

void printLocation(CXCursor cursor) {
  CXFile file;
  unsigned int line;
  unsigned int column;
  unsigned int offset;
  auto range = clang_getCursorExtent(cursor);
  auto locations = {clang_getRangeStart(range), clang_getRangeEnd(range)};

  for (auto& loc : locations) {
    clang_getFileLocation(loc, &file, &line, &column, &offset);
    auto filename = clang_getFileName(file);
    std::cout << "file: " << clang_getCString(filename) << " " << line << "[" << column << "]" << std::endl;
    clang_disposeString(filename);
  }
}

CXType result(CXCursor cursor) {
  return clang_getCursorResultType(cursor);
}

CXType canonical(CXType type) {
  return clang_getCanonicalType(type);
}

CXType underlying(CXCursor decl) {
  return clang_getTypedefDeclUnderlyingType(decl);
}

StringHandle spelling(CXType type) {
  return clang_getTypeSpelling(type);
}

StringHandle spelling(CXTypeKind kind) {
  return clang_getTypeKindSpelling(kind);
}

StringHandle spelling(CXCursor cursor) {
  return clang_getCursorSpelling(cursor);
}

json::Value type(CXType type) {
  bool isConst = clang_isConstQualifiedType(type);
  json::Object result;
  if (isConst) {
    result["const"] = isConst;
  }
  result["kind"] = type.kind;

  switch(type.kind) {
    case CXTypeKind::CXType_Typedef: {
      result["typedef"] = true;
      result["name"] = ::spelling(type).asString();
      auto cursor = clang_getTypeDeclaration(type);
      type = clang_getTypedefDeclUnderlyingType(cursor);
      result["type"] = ::type(type);
      return result;
    }
    case CXTypeKind::CXType_Pointer:
      result["pointer"] = true;
      result["type"] = ::type(clang_getPointeeType(type));
      return result;
    case CXTypeKind::CXType_LValueReference:
      result["lvalue_reference"] = true;
      result["type"] = ::type(clang_getPointeeType(type));
      return result;
    case CXTypeKind::CXType_RValueReference:
      result["rvalue_reference"] = true;
      result["type"] = ::type(clang_getPointeeType(type));
      return result;
    case CXType_ConstantArray:
    case CXType_Vector:
    case CXType_IncompleteArray:
    case CXType_VariableArray:
    case CXType_DependentSizedArray:
      result["array"] = ::type(clang_getArrayElementType(type));
      return result;
    default: {

    }
  }

  if (isConst) {
    type = clang_getCursorType(clang_getTypeDeclaration(type));
    result["type"] = ::type(type);
    return result;
  }
  result["type"] = ::spelling(type).asString();
  return result;
}

json::Value type(CXCursor cursor) {
  return ::type(clang_getCursorType(cursor));
}

StringHandle fullname(CXCursor cursor) {
  return clang_getCursorDisplayName(cursor);
}

StringHandle name(CXCursor cursor) {
  return clang_getCursorSpelling(cursor);
}

template <CXCursorKind>
struct Handler {
  static void process(CXCursor cursor) {};
};

template <>
struct Handler<CXCursorKind::CXCursor_FunctionDecl> {
  static void process(CXCursor cursor) {
    auto canonical = clang_getCanonicalCursor(cursor);
    if (!clang_equalCursors(cursor, canonical)) {
      return;
    }

    json::Array args;
    auto nargs = clang_Cursor_getNumArguments(cursor);
    for (auto idx = 0u; idx < nargs; ++idx) {
      auto arg = clang_Cursor_getArgument(cursor, idx);
      args.emplace_back(json::Object{
        {"name", ::name(arg).asString()},
        {"type", ::type(arg)}
      });
    }

    functions.emplace_back(json::Object{
      {"name", ::name(cursor).asString()},
      {"full", ::fullname(cursor).asString()},
      {"signature", ::type(cursor)},
      {"result", ::type(::result(cursor))},
      {"args", args}
    });
  };
};

template <>
struct Handler<CXCursorKind::CXCursor_MacroDefinition> {
  static void process(CXCursor cursor) {
    auto res = translateMacro(cursor);
    auto val = std::get<1>(res);

    /*
    char* end;
    auto coerced = std::strtoll(val.data(), &end, 0);
    if (end == val.data() + val.size()) {
      std::cout << std::get<0>(res) << " " << std::get<1>(res) << " " << coerced << std::endl;
    }
    else {
      std::cout << "string: " << std::get<0>(res) << " " << std::get<1>(res) << std::endl;
    }
    */
  };
};

template <>
struct Handler<CXCursorKind::CXCursor_Namespace> {
  static void process(CXCursor cursor) {
    auto ns = ::spelling(cursor);
    //std::cout << "entered namespace: " << ns << std::endl;
  }
};

CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData data) {
  if (!clang_Location_isFromMainFile(clang_getCursorLocation(cursor))) {
    return CXChildVisit_Continue;
  }
  // printLocation(cursor);

  // return CXChildVisit_[Break|Continue|Recurse]
  switch (cursor.kind) {
    case CXCursor_FunctionDecl:
      Handler<CXCursor_FunctionDecl>::process(cursor);
      break;
    case CXCursor_MacroDefinition:
      Handler<CXCursor_MacroDefinition>::process(cursor);
      break;
    case CXCursor_Namespace:
      Handler<CXCursor_Namespace>::process(cursor);
      break;
    default: {
      // printCursor(cursor);
    }
  }
  return CXChildVisit_Recurse;
}

int main(int argc, char* argv[]) {
  const std::vector<const char*> args(argv + 1, argv + argc);
  if (args.size() < 1) {
    std::cout << "Usage: scanner [filename]" << std::endl;
    return 0;
  }

  const auto filename = args[0];
  auto ctx = clang_createIndex(false, true);
  auto tu = clang_parseTranslationUnit(
    ctx,
    filename,
    &args[1],
    args.size() - 1,
    0,
    0,
    CXTranslationUnit_DetailedPreprocessingRecord);

  clang_visitChildren(clang_getTranslationUnitCursor(tu), visitor, nullptr);

  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(ctx);

  results["macros"] = macros;
  results["functions"] = functions;
  std::cout << results << std::endl;
}
