#include <serialization.hpp>
#include <json.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <tuple>

#include <clang-c/Index.h>

using json = nlohmann::json;

json results;
json functions = json::array();
json macros = json::array();
json enums = json::array();
std::shared_ptr<json> current_enum;

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

  macros.push_back({
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

CXCursor declaration(CXType type) {
  return clang_getTypeDeclaration(type);
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

json type(CXType type) {
  //std::cout << spelling(type) << " " << canonical(type).kind << " " << type.kind << std::endl;
  bool isConst = clang_isConstQualifiedType(type);
  json result;
  if (isConst) {
    result["const"] = isConst;
  }
  if (type.kind == CXType_Unexposed) {
    type = canonical(type);
  }

  result["kind"] = type.kind;

  switch (type.kind) {
    case CXTypeKind::CXType_Typedef: {
      result["typedef"] = true;
      result["name"] = ::spelling(type).asString();
      auto cursor = clang_getTypeDeclaration(type);
      type = underlying(cursor);
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
    auto nonconst_type = clang_getCursorType(clang_getTypeDeclaration(type));
    if (nonconst_type.kind == CXTypeKind::CXType_Invalid || nonconst_type.kind == CXTypeKind::CXType_Unexposed) {
      auto res = ::spelling(type).asString();
      if (res.size() > 6 && res.substr(0, 5) == "const") {
        result["type"] = res.substr(6);
      }
      else {
        result["type"] = ::spelling(type).asString();
      }
    }
    else {
      result["type"] = ::type(nonconst_type);
    }
    return result;
  }

  auto type_str = ::spelling(type).asString();
  switch (type.kind) {
    case CXTypeKind::CXType_Record:
      if (type_str.size() > 7 && type_str.substr(0, 6) == "struct") {
        type_str = type_str.substr(7);
      }
      if (type_str.size() > 6 && type_str.substr(0, 5) == "class") {
        type_str = type_str.substr(6);
      }
      break;
    case CXTypeKind::CXType_Enum:
      if (type_str.size() > 5 && type_str.substr(0, 4) == "enum") {
        type_str = type_str.substr(5);
      }
      break;
    default: {

    }
  }
  result["type"] = type_str;
  return result;
}

json type(CXCursor cursor) {
  return ::type(clang_getCursorType(cursor));
}

StringHandle fullname(CXCursor cursor) {
  return clang_getCursorDisplayName(cursor);
}

StringHandle name(CXCursor cursor) {
  return clang_getCursorSpelling(cursor);
}

bool canonical(CXCursor cursor) {
  auto can = clang_getCanonicalCursor(cursor);
  return clang_equalCursors(cursor, can);
}

template <CXCursorKind>
struct Handler {
  static CXChildVisitResult process(CXCursor cursor) {
    return CXChildVisit_Recurse;
  };
};

template <>
struct Handler<CXCursorKind::CXCursor_FunctionDecl> {
  static CXChildVisitResult process(CXCursor cursor) {
    if (!canonical(cursor)) {
      return CXChildVisit_Continue;
    }

    json args = json::array();
    auto nargs = clang_Cursor_getNumArguments(cursor);
    for (auto idx = 0u; idx < nargs; ++idx) {
      auto arg = clang_Cursor_getArgument(cursor, idx);
      args.push_back({
        {"name", ::name(arg).asString()},
        {"type", ::type(arg)}
      });
    }

    functions.push_back({
      {"name", ::name(cursor).asString()},
      {"full", ::fullname(cursor).asString()},
      {"signature", ::type(cursor)},
      {"result", ::type(::result(cursor))},
      {"args", args}
    });

    return CXChildVisit_Recurse;
  };
};

template <>
struct Handler<CXCursorKind::CXCursor_TypedefDecl> {
  static CXChildVisitResult process(CXCursor cursor) {
    if (declaration(underlying(cursor)).kind == CXCursorKind::CXCursor_EnumDecl) {
      (*current_enum)["name"] = ::spelling(clang_getCursorType(cursor)).asString();
    }
    return CXChildVisit_Continue;
  }
};

template <>
struct Handler<CXCursorKind::CXCursor_EnumDecl> {
  static CXChildVisitResult process(CXCursor cursor) {
    if (current_enum) {
      enums.push_back(std::move(*current_enum));
    }

    current_enum = std::make_shared<json>();
    auto& ref = *current_enum;
    auto is_typedef = clang_getCursorType(cursor).kind == CXTypeKind::CXType_Typedef;
    ref["name"] = is_typedef ? ::name(declaration(underlying(cursor))).asString() : ::spelling(cursor).asString();
    ref["values"] = json::object();
    ref["type"] = ::spelling(clang_getEnumDeclIntegerType(cursor)).asString();
    return CXChildVisit_Recurse;
  }
};

template <>
struct Handler<CXCursorKind::CXCursor_EnumConstantDecl> {
  static CXChildVisitResult process(CXCursor cursor) {
    auto& values = (*current_enum)["values"];
    values[::spelling(cursor).asString()] = clang_getEnumConstantDeclValue(cursor);
    return CXChildVisit_Recurse;
  }
};

template <>
struct Handler<CXCursorKind::CXCursor_MacroDefinition> {
  static CXChildVisitResult process(CXCursor cursor) {
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
    return CXChildVisit_Recurse;
  };
};

template <>
struct Handler<CXCursorKind::CXCursor_Namespace> {
  static CXChildVisitResult process(CXCursor cursor) {
    auto ns = ::spelling(cursor);
    //std::cout << "entered namespace: " << ns << std::endl;
    return CXChildVisit_Recurse;
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
      return Handler<CXCursor_FunctionDecl>::process(cursor);
    case CXCursor_MacroDefinition:
      return Handler<CXCursor_MacroDefinition>::process(cursor);
    case CXCursor_Namespace:
      return Handler<CXCursor_Namespace>::process(cursor);
    case CXCursor_EnumDecl:
      return Handler<CXCursor_EnumDecl>::process(cursor);
    case CXCursor_EnumConstantDecl:
      return Handler<CXCursor_EnumConstantDecl>::process(cursor);
    case CXCursor_TypedefDecl:
      return Handler<CXCursor_TypedefDecl>::process(cursor);
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

  if (current_enum) {
    enums.push_back(std::move(*current_enum));
  }

  results["macros"] = macros;
  results["functions"] = functions;
  results["enums"] = enums;
  std::cout << results << std::endl;
}
