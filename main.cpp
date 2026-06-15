#include <bits/stdc++.h>
using namespace std;

// ====== LEXER ======

enum class TokenKind {
    EOF_T,
    FN, LET, IF, ELSE, WHILE, RETURN,
    TRUE_T, FALSE_T,
    INT_TYPE, STR_TYPE, BOOL_TYPE, FLOAT_TYPE,
    IDENT, INT_LIT, STR_LIT,
    EQ, NEQ, LTE, GTE, AND, OR,
    ASSIGN, LT, GT,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    NOT,
    LPAREN, RPAREN, LBRACE, RBRACE,
    COLON, SEMI, COMMA, DOT
};

struct Token {
    TokenKind kind;
    string text;
    int pos;
};

struct Lexer {
    string src;
    int pos = 0;
    vector<Token> tokens;

    bool eof() const { return pos >= (int)src.size(); }
    char peek() const { return eof() ? '\0' : src[pos]; }
    char get() { return eof() ? '\0' : src[pos++]; }

    void add(TokenKind k, const string &t, int p) {
        tokens.push_back({k, t, p});
    }

    void lex() {
        while (!eof()) {
            int start = pos;
            char c = peek();
            if (isspace(c)) { get(); continue; }
            if (c == '/' && pos + 1 < (int)src.size() && src[pos+1] == '/') {
                while (!eof() && peek() != '\n') get();
                continue;
            }
            if (isalpha(c) || c == '_') {
                string id;
                while (!eof() && (isalnum(peek()) || peek() == '_')) id += get();
                if (id == "fn") add(TokenKind::FN, id, start);
                else if (id == "let") add(TokenKind::LET, id, start);
                else if (id == "if") add(TokenKind::IF, id, start);
                else if (id == "else") add(TokenKind::ELSE, id, start);
                else if (id == "while") add(TokenKind::WHILE, id, start);
                else if (id == "return") add(TokenKind::RETURN, id, start);
                else if (id == "true") add(TokenKind::TRUE_T, id, start);
                else if (id == "false") add(TokenKind::FALSE_T, id, start);
                else if (id == "int") add(TokenKind::INT_TYPE, id, start);
                else if (id == "str") add(TokenKind::STR_TYPE, id, start);
                else if (id == "bool") add(TokenKind::BOOL_TYPE, id, start);
                else if (id == "float") add(TokenKind::FLOAT_TYPE, id, start);
                else add(TokenKind::IDENT, id, start);
                continue;
            }
            if (isdigit(c)) {
                string num;
                while (!eof() && isdigit(peek())) num += get();
                add(TokenKind::INT_LIT, num, start);
                continue;
            }
            if (c == '"') {
                get();
                string s;
                while (!eof() && peek() != '"') {
                    if (peek() == '\\') {
                        get();
                        if (eof()) break;
                        char esc = get();
                        if (esc == 'n') s += '\n';
                        else if (esc == 't') s += '\t';
                        else s += esc;
                    } else s += get();
                }
                if (!eof() && peek() == '"') get();
                add(TokenKind::STR_LIT, s, start);
                continue;
            }
            auto two = [&](char a, char b, TokenKind k) {
                if (!eof() && peek() == a && pos+1 < (int)src.size() && src[pos+1] == b) {
                    get(); get();
                    add(k, string()+a+b, start);
                    return true;
                }
                return false;
            };
            if (two('=', '=', TokenKind::EQ)) continue;
            if (two('!', '=', TokenKind::NEQ)) continue;
            if (two('<', '=', TokenKind::LTE)) continue;
            if (two('>', '=', TokenKind::GTE)) continue;
            if (two('&', '&', TokenKind::AND)) continue;
            if (two('|', '|', TokenKind::OR)) continue;

            c = get();
            switch (c) {
                case '=': add(TokenKind::ASSIGN, "=", start); break;
                case '<': add(TokenKind::LT, "<", start); break;
                case '>': add(TokenKind::GT, ">", start); break;
                case '+': add(TokenKind::PLUS, "+", start); break;
                case '-': add(TokenKind::MINUS, "-", start); break;
                case '*': add(TokenKind::STAR, "*", start); break;
                case '/': add(TokenKind::SLASH, "/", start); break;
                case '%': add(TokenKind::PERCENT, "%", start); break;
                case '!': add(TokenKind::NOT, "!", start); break;
                case '(': add(TokenKind::LPAREN, "(", start); break;
                case ')': add(TokenKind::RPAREN, ")", start); break;
                case '{': add(TokenKind::LBRACE, "{", start); break;
                case '}': add(TokenKind::RBRACE, "}", start); break;
                case ':': add(TokenKind::COLON, ":", start); break;
                case ';': add(TokenKind::SEMI, ";", start); break;
                case ',': add(TokenKind::COMMA, ",", start); break;
                case '.': add(TokenKind::DOT, ".", start); break;
                default:
                    throw runtime_error("Unexpected char at " + to_string(start));
            }
        }
        tokens.push_back({TokenKind::EOF_T, "", pos});
    }
};

// ====== AST ======

struct Expr;
struct Stmt;
struct Block;
struct FunctionDef;

using ExprPtr = shared_ptr<Expr>;
using StmtPtr = shared_ptr<Stmt>;
using BlockPtr = shared_ptr<Block>;
using FuncPtr = shared_ptr<FunctionDef>;

struct Expr {
    virtual ~Expr() = default;
};

struct LiteralExpr : Expr {
    string type;
    variant<int, double, bool, string> value;
    LiteralExpr(const string &t, variant<int,double,bool,string> v) : type(t), value(v) {}
};

struct VarExpr : Expr {
    string name;
    VarExpr(const string &n) : name(n) {}
};

struct UnaryExpr : Expr {
    string op;
    ExprPtr expr;
    UnaryExpr(const string &o, ExprPtr e) : op(o), expr(e) {}
};

struct BinExpr : Expr {
    string op;
    ExprPtr left, right;
    BinExpr(const string &o, ExprPtr l, ExprPtr r) : op(o), left(l), right(r) {}
};

struct CallExpr : Expr {
    string name;
    vector<ExprPtr> args;
    CallExpr(const string &n, vector<ExprPtr> a) : name(n), args(move(a)) {}
};

struct FloatConstructExpr : Expr {
    ExprPtr intPart;
    ExprPtr fracPart;
    FloatConstructExpr(ExprPtr i, ExprPtr f) : intPart(i), fracPart(f) {}
};

struct Stmt {
    virtual ~Stmt() = default;
};

struct VarDeclStmt : Stmt {
    string name;
    string type;
    ExprPtr expr;
    VarDeclStmt(const string &n, const string &t, ExprPtr e) : name(n), type(t), expr(e) {}
};

struct AssignStmt : Stmt {
    string name;
    ExprPtr expr;
    AssignStmt(const string &n, ExprPtr e) : name(n), expr(e) {}
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    ExprStmt(ExprPtr e) : expr(e) {}
};

struct IfStmt : Stmt {
    ExprPtr cond;
    BlockPtr thenBlk;
    BlockPtr elseBlk;
    IfStmt(ExprPtr c, BlockPtr t, BlockPtr e) : cond(c), thenBlk(t), elseBlk(e) {}
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    BlockPtr body;
    WhileStmt(ExprPtr c, BlockPtr b) : cond(c), body(b) {}
};

struct ReturnStmt : Stmt {
    ExprPtr expr;
    ReturnStmt(ExprPtr e) : expr(e) {}
};

struct Block {
    vector<StmtPtr> stmts;
};

struct FunctionDef {
    string name;
    vector<pair<string,string>> params;
    string retType;
    BlockPtr body;
};

struct Program {
    vector<FuncPtr> funcs;
};

// ====== PARSER ======

struct Parser {
    vector<Token> toks;
    int i = 0;

    Parser(const vector<Token> &t) : toks(t) {}

    Token &peek() { return toks[i]; }
    bool match(TokenKind k) {
        if (peek().kind == k) { i++; return true; }
        return false;
    }
    Token &expect(TokenKind k) {
        if (!match(k)) throw runtime_error("Expected token");
        return toks[i-1];
    }

    string parseType() {
        Token &t = peek();
        if (t.kind == TokenKind::INT_TYPE) { i++; return "int"; }
        if (t.kind == TokenKind::STR_TYPE) { i++; return "str"; }
        if (t.kind == TokenKind::BOOL_TYPE) { i++; return "bool"; }
        if (t.kind == TokenKind::FLOAT_TYPE) { i++; return "float"; }
        throw runtime_error("Expected type");
    }

    BlockPtr parseBlock() {
        expect(TokenKind::LBRACE);
        auto blk = make_shared<Block>();
        while (!match(TokenKind::RBRACE)) {
            blk->stmts.push_back(parseStmt());
        }
        return blk;
    }

    FuncPtr parseFunc() {
        expect(TokenKind::FN);
        string name = expect(TokenKind::IDENT).text;
        expect(TokenKind::LPAREN);
        vector<pair<string,string>> params;
        if (!match(TokenKind::RPAREN)) {
            while (true) {
                string pname = expect(TokenKind::IDENT).text;
                expect(TokenKind::COLON);
                string ptype = parseType();
                params.push_back({pname, ptype});
                if (match(TokenKind::COMMA)) continue;
                expect(TokenKind::RPAREN);
                break;
            }
        }
        expect(TokenKind::COLON);
        string rtype = parseType();
        BlockPtr body = parseBlock();
        auto fn = make_shared<FunctionDef>();
        fn->name = name;
        fn->params = move(params);
        fn->retType = rtype;
        fn->body = body;
        return fn;
    }

    Program parseProgram() {
        Program p;
        while (peek().kind != TokenKind::EOF_T) {
            p.funcs.push_back(parseFunc());
        }
        return p;
    }

    StmtPtr parseStmt() {
        Token &t = peek();
        if (t.kind == TokenKind::LET) return parseVarDecl();
        if (t.kind == TokenKind::IF) return parseIf();
        if (t.kind == TokenKind::WHILE) return parseWhile();
        if (t.kind == TokenKind::RETURN) return parseReturn();
        if (t.kind == TokenKind::IDENT) {
            Token &next = toks[i+1];
            if (next.kind == TokenKind::ASSIGN) {
                string name = expect(TokenKind::IDENT).text;
                expect(TokenKind::ASSIGN);
                ExprPtr e = parseExpr();
                expect(TokenKind::SEMI);
                return make_shared<AssignStmt>(name, e);
            }
        }
        ExprPtr e = parseExpr();
        expect(TokenKind::SEMI);
        return make_shared<ExprStmt>(e);
    }

    StmtPtr parseVarDecl() {
        expect(TokenKind::LET);
        string name = expect(TokenKind::IDENT).text;
        expect(TokenKind::COLON);
        string tname = parseType();
        expect(TokenKind::ASSIGN);
        ExprPtr e = parseExpr();
        expect(TokenKind::SEMI);
        return make_shared<VarDeclStmt>(name, tname, e);
    }

    StmtPtr parseIf() {
        expect(TokenKind::IF);
        expect(TokenKind::LPAREN);
        ExprPtr cond = parseExpr();
        expect(TokenKind::RPAREN);
        BlockPtr thenBlk = parseBlock();
        BlockPtr elseBlk = nullptr;
        if (match(TokenKind::ELSE)) {
            elseBlk = parseBlock();
        }
        return make_shared<IfStmt>(cond, thenBlk, elseBlk);
    }

    StmtPtr parseWhile() {
        expect(TokenKind::WHILE);
        expect(TokenKind::LPAREN);
        ExprPtr cond = parseExpr();
        expect(TokenKind::RPAREN);
        BlockPtr body = parseBlock();
        return make_shared<WhileStmt>(cond, body);
    }

    StmtPtr parseReturn() {
        expect(TokenKind::RETURN);
        ExprPtr e = parseExpr();
        expect(TokenKind::SEMI);
        return make_shared<ReturnStmt>(e);
    }

    ExprPtr parseExpr() { return parseOr(); }

    ExprPtr parseOr() {
        ExprPtr e = parseAnd();
        while (match(TokenKind::OR)) {
            ExprPtr r = parseAnd();
            e = make_shared<BinExpr>("||", e, r);
        }
        return e;
    }

    ExprPtr parseAnd() {
        ExprPtr e = parseEq();
        while (match(TokenKind::AND)) {
            ExprPtr r = parseEq();
            e = make_shared<BinExpr>("&&", e, r);
        }
        return e;
    }

    ExprPtr parseEq() {
        ExprPtr e = parseCmp();
        while (true) {
            if (match(TokenKind::EQ)) {
                ExprPtr r = parseCmp();
                e = make_shared<BinExpr>("==", e, r);
            } else if (match(TokenKind::NEQ)) {
                ExprPtr r = parseCmp();
                e = make_shared<BinExpr>("!=", e, r);
            } else break;
        }
        return e;
    }

    ExprPtr parseCmp() {
        ExprPtr e = parseAdd();
        while (true) {
            if (match(TokenKind::LT)) {
                ExprPtr r = parseAdd();
                e = make_shared<BinExpr>("<", e, r);
            } else if (match(TokenKind::LTE)) {
                ExprPtr r = parseAdd();
                e = make_shared<BinExpr>("<=", e, r);
            } else if (match(TokenKind::GT)) {
                ExprPtr r = parseAdd();
                e = make_shared<BinExpr>(">", e, r);
            } else if (match(TokenKind::GTE)) {
                ExprPtr r = parseAdd();
                e = make_shared<BinExpr>(">=", e, r);
            } else break;
        }
        return e;
    }

    ExprPtr parseAdd() {
        ExprPtr e = parseMul();
        while (true) {
            if (match(TokenKind::PLUS)) {
                ExprPtr r = parseMul();
                e = make_shared<BinExpr>("+", e, r);
            } else if (match(TokenKind::MINUS)) {
                ExprPtr r = parseMul();
                e = make_shared<BinExpr>("-", e, r);
            } else break;
        }
        return e;
    }

    ExprPtr parseMul() {
        ExprPtr e = parseUnary();
        while (true) {
            if (match(TokenKind::STAR)) {
                ExprPtr r = parseUnary();
                e = make_shared<BinExpr>("*", e, r);
            } else if (match(TokenKind::SLASH)) {
                ExprPtr r = parseUnary();
                e = make_shared<BinExpr>("/", e, r);
            } else if (match(TokenKind::PERCENT)) {
                ExprPtr r = parseUnary();
                e = make_shared<BinExpr>("%", e, r);
            } else break;
        }
        return e;
    }

    ExprPtr parseUnary() {
        if (match(TokenKind::NOT)) {
            ExprPtr e = parseUnary();
            return make_shared<UnaryExpr>("!", e);
        }
        if (match(TokenKind::MINUS)) {
            ExprPtr e = parseUnary();
            return make_shared<UnaryExpr>("-", e);
        }
        return parsePrimary();
    }

    ExprPtr parsePrimary() {
        Token &t = peek();

        if (t.kind == TokenKind::FLOAT_TYPE) {
            i++;
            expect(TokenKind::LPAREN);
            ExprPtr ip = parseExpr();
            expect(TokenKind::DOT);
            ExprPtr fp = parseExpr();
            expect(TokenKind::RPAREN);
            return make_shared<FloatConstructExpr>(ip, fp);
        }

        if (t.kind == TokenKind::INT_LIT) {
            i++;
            int v = stoi(t.text);
            return make_shared<LiteralExpr>("int", v);
        }
        if (t.kind == TokenKind::STR_LIT) {
            i++;
            return make_shared<LiteralExpr>("str", t.text);
        }
        if (t.kind == TokenKind::TRUE_T) {
            i++;
            return make_shared<LiteralExpr>("bool", true);
        }
        if (t.kind == TokenKind::FALSE_T) {
            i++;
            return make_shared<LiteralExpr>("bool", false);
        }
        if (t.kind == TokenKind::IDENT) {
            string name = t.text;
            i++;
            if (match(TokenKind::LPAREN)) {
                vector<ExprPtr> args;
                if (!match(TokenKind::RPAREN)) {
                    while (true) {
                        args.push_back(parseExpr());
                        if (match(TokenKind::COMMA)) continue;
                        expect(TokenKind::RPAREN);
                        break;
                    }
                }
                return make_shared<CallExpr>(name, args);
            }
            return make_shared<VarExpr>(name);
        }
        if (t.kind == TokenKind::LPAREN) {
            i++;
            ExprPtr e = parseExpr();
            expect(TokenKind::RPAREN);
            return e;
        }
        throw runtime_error("Unexpected token in primary");
    }
};

// ====== INTERPRETER ======

struct Value {
    string type; // "int","float","bool","str","void"
    variant<int,double,bool,string> v;

    static Value makeInt(int x){ return {"int", x}; }
    static Value makeFloat(double x){ return {"float", x}; }
    static Value makeBool(bool x){ return {"bool", x}; }
    static Value makeStr(const string &x){ return {"str", x}; }
    static Value makeVoid(){ return {"void", 0}; }
};

struct Env {
    shared_ptr<Env> parent;
    unordered_map<string, Value> vars;
    Env(shared_ptr<Env> p=nullptr):parent(p){}
    void define(const string &n, const Value &v){ vars[n]=v; }
    bool set(const string &n, const Value &v){
        if (vars.count(n)){ vars[n]=v; return true; }
        if (parent) return parent->set(n,v);
        return false;
    }
    Value get(const string &n){
        if (vars.count(n)) return vars[n];
        if (parent) return parent->get(n);
        throw runtime_error("Undefined variable: " + n);
    }
};

struct ReturnSignal {
    Value val;
};

struct Interpreter {
    Program prog;
    unordered_map<string, FuncPtr> funcs;

    Interpreter(const Program &p):prog(p){
        for (auto &f: prog.funcs) funcs[f->name]=f;
    }

    Value evalExpr(ExprPtr e, shared_ptr<Env> env) {
        if (auto lit = dynamic_pointer_cast<LiteralExpr>(e)) {
            return {lit->type, lit->value};
        }
        if (auto v = dynamic_pointer_cast<VarExpr>(e)) {
            return env->get(v->name);
        }
        if (auto u = dynamic_pointer_cast<UnaryExpr>(e)) {
            Value val = evalExpr(u->expr, env);
            if (u->op == "!") {
                if (val.type != "bool") throw runtime_error("! requires bool");
                return Value::makeBool(!get<bool>(val.v));
            }
            if (u->op == "-") {
                if (val.type == "int") return Value::makeInt(-get<int>(val.v));
                if (val.type == "float") return Value::makeFloat(-get<double>(val.v));
                throw runtime_error("- requires int or float");
            }
        }
        if (auto b = dynamic_pointer_cast<BinExpr>(e)) {
            Value L = evalExpr(b->left, env);
            Value R = evalExpr(b->right, env);
            string op = b->op;

            auto asDouble = [](const Value &x)->double{
                if (x.type=="int") return get<int>(x.v);
                if (x.type=="float") return get<double>(x.v);
                throw runtime_error("Expected numeric");
            };

            if (op=="+" || op=="-" || op=="*" || op=="/" || op=="%") {
                if (op=="+" && L.type=="str" && R.type=="str") {
                    return Value::makeStr(get<string>(L.v) + get<string>(R.v));
                }
                if (L.type=="int" && R.type=="int") {
                    int a = get<int>(L.v), b = get<int>(R.v);
                    if (op=="+") return Value::makeInt(a+b);
                    if (op=="-") return Value::makeInt(a-b);
                    if (op=="*") return Value::makeInt(a*b);
                    if (op=="/") return Value::makeInt(a/b);
                    if (op=="%") return Value::makeInt(a%b);
                }
                if ((L.type=="int"||L.type=="float") && (R.type=="int"||R.type=="float")) {
                    double a = asDouble(L), c = asDouble(R);
                    if (op=="+") return Value::makeFloat(a+c);
                    if (op=="-") return Value::makeFloat(a-c);
                    if (op=="*") return Value::makeFloat(a*c);
                    if (op=="/") return Value::makeFloat(a/c);
                    if (op=="%") return Value::makeFloat(fmod(a,c));
                }
                throw runtime_error("Invalid operands for " + op);
            }

            if (op=="<" || op=="<=" || op==">" || op==">=") {
                double a = asDouble(L), c = asDouble(R);
                bool res=false;
                if (op=="<") res = a<c;
                else if (op=="<=") res = a<=c;
                else if (op==">") res = a>c;
                else if (op==">=") res = a>=c;
                return Value::makeBool(res);
            }

            if (op=="==") return Value::makeBool(L.v == R.v);
            if (op=="!=") return Value::makeBool(L.v != R.v);

            if (op=="&&") {
                if (L.type!="bool" || R.type!="bool") throw runtime_error("&& requires bool");
                return Value::makeBool(get<bool>(L.v) && get<bool>(R.v));
            }
            if (op=="||") {
                if (L.type!="bool" || R.type!="bool") throw runtime_error("|| requires bool");
                return Value::makeBool(get<bool>(L.v) || get<bool>(R.v));
            }

            throw runtime_error("Unknown binary op");
        }
        if (auto c = dynamic_pointer_cast<CallExpr>(e)) {
            if (c->name=="print") {
                if (c->args.size()!=1) throw runtime_error("print() takes 1 arg");
                Value v = evalExpr(c->args[0], env);
                if (v.type=="int") cout << get<int>(v.v) << "\n";
                else if (v.type=="float") cout << get<double>(v.v) << "\n";
                else if (v.type=="bool") cout << (get<bool>(v.v)?"true":"false") << "\n";
                else if (v.type=="str") cout << get<string>(v.v) << "\n";
                return Value::makeVoid();
            }
            if (c->name=="len") {
                if (c->args.size()!=1) throw runtime_error("len() takes 1 arg");
                Value v = evalExpr(c->args[0], env);
                if (v.type!="str") throw runtime_error("len() requires str");
                return Value::makeInt((int)get<string>(v.v).size());
            }
            if (c->name=="to_str") {
                if (c->args.size()!=1) throw runtime_error("to_str() takes 1 arg");
                Value v = evalExpr(c->args[0], env);
                if (v.type=="int") return Value::makeStr(to_string(get<int>(v.v)));
                if (v.type=="float") return Value::makeStr(to_string(get<double>(v.v)));
                if (v.type=="bool") return Value::makeStr(get<bool>(v.v)?"true":"false");
                if (v.type=="str") return v;
                return Value::makeStr("");
            }
            if (!funcs.count(c->name)) throw runtime_error("Unknown function: " + c->name);
            FuncPtr fn = funcs[c->name];
            if (c->args.size()!=fn->params.size()) throw runtime_error("Arg count mismatch");
            vector<Value> argVals;
            for (auto &a: c->args) argVals.push_back(evalExpr(a, env));
            return callFunction(fn, argVals);
        }
        if (auto f = dynamic_pointer_cast<FloatConstructExpr>(e)) {
            Value iv = evalExpr(f->intPart, env);
            Value fv = evalExpr(f->fracPart, env);
            if (iv.type!="int" || fv.type!="int") throw runtime_error("float(int.int) requires ints");
            int i = get<int>(iv.v);
            int frac = get<int>(fv.v);
            int fracAbs = abs(frac);
            int digits = (fracAbs==0)?1:0;
            while (fracAbs>0){ digits++; fracAbs/=10; }
            double val = (double)i + (double)frac / pow(10.0, digits);
            return Value::makeFloat(val);
        }
        throw runtime_error("Unknown expr");
    }

    void execBlock(BlockPtr blk, shared_ptr<Env> env) {
        auto local = make_shared<Env>(env);
        for (auto &s: blk->stmts) execStmt(s, local);
    }

    void execStmt(StmtPtr s, shared_ptr<Env> env) {
        if (auto v = dynamic_pointer_cast<VarDeclStmt>(s)) {
            Value val = evalExpr(v->expr, env);
            env->define(v->name, val);
        } else if (auto a = dynamic_pointer_cast<AssignStmt>(s)) {
            Value val = evalExpr(a->expr, env);
            if (!env->set(a->name, val)) throw runtime_error("Undefined var in assign: " + a->name);
        } else if (auto e = dynamic_pointer_cast<ExprStmt>(s)) {
            evalExpr(e->expr, env);
        } else if (auto i = dynamic_pointer_cast<IfStmt>(s)) {
            Value c = evalExpr(i->cond, env);
            if (c.type!="bool") throw runtime_error("if cond must be bool");
            if (get<bool>(c.v)) execBlock(i->thenBlk, env);
            else if (i->elseBlk) execBlock(i->elseBlk, env);
        } else if (auto w = dynamic_pointer_cast<WhileStmt>(s)) {
            while (true) {
                Value c = evalExpr(w->cond, env);
                if (c.type!="bool") throw runtime_error("while cond must be bool");
                if (!get<bool>(c.v)) break;
                execBlock(w->body, env);
            }
        } else if (auto r = dynamic_pointer_cast<ReturnStmt>(s)) {
            Value v = evalExpr(r->expr, env);
            throw ReturnSignal{v};
        } else {
            throw runtime_error("Unknown stmt");
        }
    }

    Value callFunction(FuncPtr fn, const vector<Value> &args) {
        auto env = make_shared<Env>();
        for (size_t i=0;i<args.size();++i) {
            env->define(fn->params[i].first, args[i]);
        }
        try {
            execBlock(fn->body, env);
        } catch (ReturnSignal &rs) {
            return rs.val;
        }
        return Value::makeVoid();
    }

    void run() {
        if (!funcs.count("main")) throw runtime_error("No main()");
        FuncPtr mainFn = funcs["main"];
        Value ret = callFunction(mainFn, {});
        (void)ret;
    }
};

// ====== MAIN ======

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: sprout <file.sprout>\n";
        return 1;
    }
    ifstream in(argv[1]);
    if (!in) {
        cerr << "Cannot open file\n";
        return 1;
    }
    string src((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());

    Lexer lx{src};
    lx.lex();
    Parser p(lx.tokens);
    Program prog = p.parseProgram();
    Interpreter interp(prog);
    try {
        interp.run();
    } catch (const exception &e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
