import sys
import re

# ========== LEXER ==========

TOKEN_SPEC = [
    ("WHITESPACE", r"[ \t\r\n]+"),
    ("COMMENT",    r"//[^\n]*"),
    ("FN",         r"\bfn\b"),
    ("LET",        r"\blet\b"),
    ("IF",         r"\bif\b"),
    ("ELSE",       r"\belse\b"),
    ("WHILE",      r"\bwhile\b"),
    ("RETURN",     r"\breturn\b"),
    ("TRUE",       r"\btrue\b"),
    ("FALSE",      r"\bfalse\b"),
    ("INT_TYPE",   r"\bint\b"),
    ("STR_TYPE",   r"\bstr\b"),
    ("BOOL_TYPE",  r"\bbool\b"),
    ("FLOAT_TYPE", r"\bfloat\b"),
    ("PRINT",      r"\bprint\b"),
    ("LEN",        r"\blen\b"),
    ("TO_STR",     r"\bto_str\b"),
    ("IDENT",      r"[A-Za-z_][A-Za-z0-9_]*"),
    ("INT",        r"[0-9]+"),
    ("STRING",     r"\"([^\"\\]|\\.)*\""),
    ("EQ",         r"=="),
    ("NEQ",        r"!="),
    ("LTE",        r"<="),
    ("GTE",        r">="),
    ("AND",        r"&&"),
    ("OR",         r"\|\|"),
    ("ASSIGN",     r"="),
    ("LT",         r"<"),
    ("GT",         r">"),
    ("PLUS",       r"\+"),
    ("MINUS",      r"-"),
    ("STAR",       r"\*"),
    ("SLASH",      r"/"),
    ("PERCENT",    r"%"),
    ("NOT",        r"!"),
    ("LPAREN",     r"\("),
    ("RPAREN",     r"\)"),
    ("LBRACE",     r"\{"),
    ("RBRACE",     r"\}"),
    ("COLON",      r":"),
    ("SEMI",       r";"),
    ("COMMA",      r","),
    ("DOT",        r"\."),
]

MASTER_RE = re.compile("|".join(f"(?P<{name}>{pattern})" for name, pattern in TOKEN_SPEC))

class Token:
    def __init__(self, kind, value, pos):
        self.kind = kind
        self.value = value
        self.pos = pos
    def __repr__(self):
        return f"Token({self.kind}, {self.value})"

def lex(src):
    tokens = []
    pos = 0
    while pos < len(src):
        m = MASTER_RE.match(src, pos)
        if not m:
            raise SyntaxError(f"Unexpected character at {pos}: {src[pos]!r}")
        kind = m.lastgroup
        text = m.group()
        if kind in ("WHITESPACE", "COMMENT"):
            pos = m.end()
            continue
        if kind == "STRING":
            # strip quotes, handle escapes minimally
            text = bytes(text[1:-1], "utf-8").decode("unicode_escape")
        tokens.append(Token(kind, text, pos))
        pos = m.end()
    tokens.append(Token("EOF", "", pos))
    return tokens

# ========== AST NODES ==========

class Program: 
    def __init__(self, functions): self.functions = functions

class FunctionDef:
    def __init__(self, name, params, return_type, body):
        self.name = name
        self.params = params  # list of (name, type)
        self.return_type = return_type
        self.body = body

class Block:
    def __init__(self, statements): self.statements = statements

class VarDecl:
    def __init__(self, name, type_name, expr):
        self.name = name
        self.type_name = type_name
        self.expr = expr

class Assign:
    def __init__(self, name, expr):
        self.name = name
        self.expr = expr

class If:
    def __init__(self, cond, then_block, else_block):
        self.cond = cond
        self.then_block = then_block
        self.else_block = else_block

class While:
    def __init__(self, cond, body):
        self.cond = cond
        self.body = body

class Return:
    def __init__(self, expr):
        self.expr = expr

class ExprStmt:
    def __init__(self, expr):
        self.expr = expr

class BinOp:
    def __init__(self, op, left, right):
        self.op = op
        self.left = left
        self.right = right

class UnaryOp:
    def __init__(self, op, expr):
        self.op = op
        self.expr = expr

class Call:
    def __init__(self, name, args):
        self.name = name
        self.args = args

class Literal:
    def __init__(self, value, type_name):
        self.value = value
        self.type_name = type_name

class Var:
    def __init__(self, name):
        self.name = name

class FloatConstruct:
    def __init__(self, int_part, frac_part):
        self.int_part = int_part
        self.frac_part = frac_part

# ========== PARSER ==========

class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.i = 0

    def peek(self):
        return self.tokens[self.i]

    def match(self, kind):
        if self.peek().kind == kind:
            t = self.peek()
            self.i += 1
            return t
        return None

    def expect(self, kind):
        t = self.match(kind)
        if not t:
            raise SyntaxError(f"Expected {kind}, got {self.peek().kind}")
        return t

    def parse_program(self):
        funcs = []
        while self.peek().kind != "EOF":
            funcs.append(self.parse_function())
        return Program(funcs)

    def parse_function(self):
        self.expect("FN")
        name = self.expect("IDENT").value
        self.expect("LPAREN")
        params = []
        if not self.match("RPAREN"):
            while True:
                pname = self.expect("IDENT").value
                self.expect("COLON")
                ptype = self.parse_type()
                params.append((pname, ptype))
                if self.match("COMMA"):
                    continue
                else:
                    self.expect("RPAREN")
                    break
        self.expect("COLON")
        rtype = self.parse_type()
        body = self.parse_block()
        return FunctionDef(name, params, rtype, body)

    def parse_type(self):
        tok = self.peek()
        if tok.kind in ("INT_TYPE", "STR_TYPE", "BOOL_TYPE", "FLOAT_TYPE"):
            self.i += 1
            if tok.kind == "INT_TYPE": return "int"
            if tok.kind == "STR_TYPE": return "str"
            if tok.kind == "BOOL_TYPE": return "bool"
            if tok.kind == "FLOAT_TYPE": return "float"
        raise SyntaxError(f"Expected type, got {tok.kind}")

    def parse_block(self):
        self.expect("LBRACE")
        stmts = []
        while not self.match("RBRACE"):
            stmts.append(self.parse_statement())
        return Block(stmts)

    def parse_statement(self):
        tok = self.peek()
        if tok.kind == "LET":
            return self.parse_vardecl()
        if tok.kind == "IF":
            return self.parse_if()
        if tok.kind == "WHILE":
            return self.parse_while()
        if tok.kind == "RETURN":
            return self.parse_return()
        # assignment or expr
        if tok.kind == "IDENT":
            # lookahead for ASSIGN
            if self.tokens[self.i+1].kind == "ASSIGN":
                name = self.expect("IDENT").value
                self.expect("ASSIGN")
                expr = self.parse_expression()
                self.expect("SEMI")
                return Assign(name, expr)
        # expression statement
        expr = self.parse_expression()
        self.expect("SEMI")
        return ExprStmt(expr)

    def parse_vardecl(self):
        self.expect("LET")
        name = self.expect("IDENT").value
        self.expect("COLON")
        tname = self.parse_type()
        self.expect("ASSIGN")
        expr = self.parse_expression()
        self.expect("SEMI")
        return VarDecl(name, tname, expr)

    def parse_if(self):
        self.expect("IF")
        self.expect("LPAREN")
        cond = self.parse_expression()
        self.expect("RPAREN")
        then_block = self.parse_block()
        else_block = None
        if self.match("ELSE"):
            else_block = self.parse_block()
        return If(cond, then_block, else_block)

    def parse_while(self):
        self.expect("WHILE")
        self.expect("LPAREN")
        cond = self.parse_expression()
        self.expect("RPAREN")
        body = self.parse_block()
        return While(cond, body)

    def parse_return(self):
        self.expect("RETURN")
        expr = self.parse_expression()
        self.expect("SEMI")
        return Return(expr)

    # Expression precedence:
    # OR, AND, equality, comparison, additive, multiplicative, unary, primary

    def parse_expression(self):
        return self.parse_or()

    def parse_or(self):
        expr = self.parse_and()
        while self.match("OR"):
            right = self.parse_and()
            expr = BinOp("||", expr, right)
        return expr

    def parse_and(self):
        expr = self.parse_equality()
        while self.match("AND"):
            right = self.parse_equality()
            expr = BinOp("&&", expr, right)
        return expr

    def parse_equality(self):
        expr = self.parse_comparison()
        while True:
            if self.match("EQ"):
                right = self.parse_comparison()
                expr = BinOp("==", expr, right)
            elif self.match("NEQ"):
                right = self.parse_comparison()
                expr = BinOp("!=", expr, right)
            else:
                break
        return expr

    def parse_comparison(self):
        expr = self.parse_additive()
        while True:
            if self.match("LT"):
                right = self.parse_additive()
                expr = BinOp("<", expr, right)
            elif self.match("LTE"):
                right = self.parse_additive()
                expr = BinOp("<=", expr, right)
            elif self.match("GT"):
                right = self.parse_additive()
                expr = BinOp(">", expr, right)
            elif self.match("GTE"):
                right = self.parse_additive()
                expr = BinOp(">=", expr, right)
            else:
                break
        return expr

    def parse_additive(self):
        expr = self.parse_multiplicative()
        while True:
            if self.match("PLUS"):
                right = self.parse_multiplicative()
                expr = BinOp("+", expr, right)
            elif self.match("MINUS"):
                right = self.parse_multiplicative()
                expr = BinOp("-", expr, right)
            else:
                break
        return expr

    def parse_multiplicative(self):
        expr = self.parse_unary()
        while True:
            if self.match("STAR"):
                right = self.parse_unary()
                expr = BinOp("*", expr, right)
            elif self.match("SLASH"):
                right = self.parse_unary()
                expr = BinOp("/", expr, right)
            elif self.match("PERCENT"):
                right = self.parse_unary()
                expr = BinOp("%", expr, right)
            else:
                break
        return expr

    def parse_unary(self):
        if self.match("NOT"):
            expr = self.parse_unary()
            return UnaryOp("!", expr)
        if self.match("MINUS"):
            expr = self.parse_unary()
            return UnaryOp("-", expr)
        return self.parse_primary()

    def parse_primary(self):
        tok = self.peek()

        # float(int.int) construct
        if tok.kind == "FLOAT_TYPE":
            # treat as 'float(' expr '.' expr ')'
            self.i += 1
            self.expect("LPAREN")
            int_part = self.parse_expression()
            self.expect("DOT")
            frac_part = self.parse_expression()
            self.expect("RPAREN")
            return FloatConstruct(int_part, frac_part)

        if tok.kind == "INT":
            self.i += 1
            return Literal(int(tok.value), "int")
        if tok.kind == "STRING":
            self.i += 1
            return Literal(tok.value, "str")
        if tok.kind == "TRUE":
            self.i += 1
            return Literal(True, "bool")
        if tok.kind == "FALSE":
            self.i += 1
            return Literal(False, "bool")
        if tok.kind == "IDENT":
            # function call or variable
            name = tok.value
            self.i += 1
            if self.match("LPAREN"):
                args = []
                if not self.match("RPAREN"):
                    while True:
                        args.append(self.parse_expression())
                        if self.match("COMMA"):
                            continue
                        else:
                            self.expect("RPAREN")
                            break
                return Call(name, args)
            return Var(name)
        if tok.kind == "LPAREN":
            self.i += 1
            expr = self.parse_expression()
            self.expect("RPAREN")
            return expr

        raise SyntaxError(f"Unexpected token {tok.kind} at {tok.pos}")

# ========== INTERPRETER ==========

class ReturnSignal(Exception):
    def __init__(self, value):
        self.value = value

class Env:
    def __init__(self, parent=None):
        self.parent = parent
        self.vars = {}
    def define(self, name, value, type_name):
        self.vars[name] = (value, type_name)
    def set(self, name, value, type_name=None):
        if name in self.vars:
            _, old_type = self.vars[name]
            self.vars[name] = (value, type_name or old_type)
            return
        if self.parent:
            self.parent.set(name, value, type_name)
            return
        raise NameError(f"Undefined variable {name}")
    def get(self, name):
        if name in self.vars:
            return self.vars[name]
        if self.parent:
            return self.parent.get(name)
        raise NameError(f"Undefined variable {name}")

class Interpreter:
    def __init__(self, program):
        self.program = program
        self.functions = {fn.name: fn for fn in program.functions}

    def run(self):
        if "main" not in self.functions:
            raise RuntimeError("No main() function")
        main_fn = self.functions["main"]
        return self.call_function(main_fn, [])

    def call_function(self, fn, args):
        if len(args) != len(fn.params):
            raise RuntimeError(f"Function {fn.name} expects {len(fn.params)} args, got {len(args)}")
        env = Env()
        for (pname, ptype), (val, vtype) in zip(fn.params, args):
            env.define(pname, val, ptype)
        try:
            self.exec_block(fn.body, env)
        except ReturnSignal as rs:
            return rs.value
        # default return 0 for main if no return
        return (0, fn.return_type)

    def exec_block(self, block, env):
        local = Env(env)
        for stmt in block.statements:
            self.exec_stmt(stmt, local)

    def exec_stmt(self, stmt, env):
        if isinstance(stmt, VarDecl):
            val, t = self.eval_expr(stmt.expr, env)
            env.define(stmt.name, val, stmt.type_name)
        elif isinstance(stmt, Assign):
            val, t = self.eval_expr(stmt.expr, env)
            env.set(stmt.name, val)
        elif isinstance(stmt, If):
            cond_val, cond_type = self.eval_expr(stmt.cond, env)
            if cond_type != "bool":
                raise RuntimeError("if condition must be bool")
            if cond_val:
                self.exec_block(stmt.then_block, env)
            elif stmt.else_block:
                self.exec_block(stmt.else_block, env)
        elif isinstance(stmt, While):
            while True:
                cond_val, cond_type = self.eval_expr(stmt.cond, env)
                if cond_type != "bool":
                    raise RuntimeError("while condition must be bool")
                if not cond_val:
                    break
                self.exec_block(stmt.body, env)
        elif isinstance(stmt, Return):
            val, t = self.eval_expr(stmt.expr, env)
            raise ReturnSignal((val, t))
        elif isinstance(stmt, ExprStmt):
            self.eval_expr(stmt.expr, env)
        else:
            raise RuntimeError(f"Unknown statement {stmt}")

    def eval_expr(self, expr, env):
        if isinstance(expr, Literal):
            return expr.value, expr.type_name
        if isinstance(expr, Var):
            return env.get(expr.name)
        if isinstance(expr, UnaryOp):
            val, t = self.eval_expr(expr.expr, env)
            if expr.op == "!":
                if t != "bool":
                    raise RuntimeError("! requires bool")
                return (not val, "bool")
            if expr.op == "-":
                if t != "int" and t != "float":
                    raise RuntimeError("- requires int or float")
                return (-val, t)
        if isinstance(expr, BinOp):
            left, lt = self.eval_expr(expr.left, env)
            right, rt = self.eval_expr(expr.right, env)
            op = expr.op

            # arithmetic
            if op in ("+", "-", "*", "/", "%"):
                # string concat
                if op == "+" and lt == "str" and rt == "str":
                    return (left + right, "str")
                # int/float arithmetic
                if lt == "int" and rt == "int":
                    if op == "+": return (left + right, "int")
                    if op == "-": return (left - right, "int")
                    if op == "*": return (left * right, "int")
                    if op == "/": return (left // right, "int")
                    if op == "%": return (left % right, "int")
                # promote to float if any float
                if (lt in ("int", "float")) and (rt in ("int", "float")):
                    lf = float(left)
                    rf = float(right)
                    if op == "+": return (lf + rf, "float")
                    if op == "-": return (lf - rf, "float")
                    if op == "*": return (lf * rf, "float")
                    if op == "/": return (lf / rf, "float")
                    if op == "%": return (lf % rf, "float")
                raise RuntimeError(f"Invalid operands for {op}: {lt}, {rt}")

            # comparisons
            if op in ("<", "<=", ">", ">="):
                return (eval(f"{left} {op} {right}"), "bool")

            if op == "==":
                return (left == right, "bool")
            if op == "!=":
                return (left != right, "bool")

            # logical
            if op == "&&":
                if lt != "bool" or rt != "bool":
                    raise RuntimeError("&& requires bools")
                return (left and right, "bool")
            if op == "||":
                if lt != "bool" or rt != "bool":
                    raise RuntimeError("|| requires bools")
                return (left or right, "bool")

            raise RuntimeError(f"Unknown binary op {op}")

        if isinstance(expr, Call):
            # built-ins
            if expr.name == "print":
                if len(expr.args) != 1:
                    raise RuntimeError("print() takes 1 arg")
                val, t = self.eval_expr(expr.args[0], env)
                print(val)
                return (None, "void")
            if expr.name == "len":
                if len(expr.args) != 1:
                    raise RuntimeError("len() takes 1 arg")
                val, t = self.eval_expr(expr.args[0], env)
                if t != "str":
                    raise RuntimeError("len() requires str")
                return (len(val), "int")
            if expr.name == "to_str":
                if len(expr.args) != 1:
                    raise RuntimeError("to_str() takes 1 arg")
                val, t = self.eval_expr(expr.args[0], env)
                return (str(val), "str")

            # user-defined
            if expr.name not in self.functions:
                raise RuntimeError(f"Unknown function {expr.name}")
            fn = self.functions[expr.name]
            arg_vals = [self.eval_expr(a, env) for a in expr.args]
            return self.call_function(fn, arg_vals)

        if isinstance(expr, FloatConstruct):
            int_val, it = self.eval_expr(expr.int_part, env)
            frac_val, ft = self.eval_expr(expr.frac_part, env)
            if it != "int" or ft != "int":
                raise RuntimeError("float(int.int) requires two ints")
            frac_abs = abs(frac_val)
            digits = len(str(frac_abs)) if frac_abs != 0 else 1
            value = float(int_val) + (float(frac_val) / (10 ** digits))
            return (value, "float")

        raise RuntimeError(f"Unknown expression {expr}")

# ========== DRIVER ==========

def run_source(src):
    tokens = lex(src)
    parser = Parser(tokens)
    prog = parser.parse_program()
    interp = Interpreter(prog)
    interp.run()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: sprout.py <file.sprout>")
        sys.exit(1)
    with open(sys.argv[1], "r", encoding="utf-8") as f:
        src = f.read()
    run_source(src)
