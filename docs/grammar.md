# Sprout Grammar
```mermaid
railroad-ebnf-beta
    title Expression Grammar

    program         = { function } ;

    function        = "fn" IDENT "(" [ parameters ] ")" ":" type block ;
    parameters      = parameter , { "," parameter } ;
    parameter       = IDENT ":" type ;

    type            = "int" | "str" | "bool" | "float" ;

    block           = "{" { statement } "}" ;

    statement       =
        vardecl
        | if_stmt
        | while_stmt
        | return_stmt
        | assign_stmt
        | expr_stmt ;

    vardecl         = "let" IDENT ":" type "=" expression ";" ;
    assign_stmt     = IDENT "=" expression ";" ;
    expr_stmt       = expression ";" ;

    if_stmt         = "if" "(" expression ")" block [ "else" block ] ;
    while_stmt      = "while" "(" expression ")" block ;
    return_stmt     = "return" expression ";" ;

    expression      = or_expr ;

    or_expr         = and_expr , { "||" and_expr } ;
    and_expr        = equality_expr , { "&&" equality_expr } ;

    equality_expr   =
        comparison_expr ,
        { ( "==" | "!=" ) comparison_expr } ;

    comparison_expr =
        additive_expr ,
        { ( "<" | "<=" | ">" | ">=" ) additive_expr } ;

    additive_expr   =
        multiplicative_expr ,
        { ( "+" | "-" ) multiplicative_expr } ;

    multiplicative_expr =
        unary_expr ,
        { ( "*" | "/" | "%" ) unary_expr } ;

    unary_expr      =
        ( "!" | "-" ) unary_expr
        | primary ;

    primary         =
        literal
        | variable
        | call
        | "(" expression ")"
        | float_construct ;

    literal         =
        INT
        | STRING
        | "true"
        | "false" ;

    variable        = IDENT ;

    call            = IDENT "(" [ arguments ] ")" ;
    arguments       = expression , { "," expression } ;

    float_construct =
        "float" "(" expression "." expression ")" ;

    IDENT           = letter , { letter | digit | "_" } ;
    INT             = digit , { digit } ;
    STRING          = '"' , { character | escape } , '"' ;

    letter          = "A"-"Z" | "a"-"z" | "_" ;
    digit           = "0"-"9" ;
    escape          = "\"" , any-character ;
    character       = any-character-except-quote-or-backslash ;
```
