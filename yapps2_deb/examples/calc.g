globalvars = {}       # We will store the calculator's variables here

def lookup(map, name):
    for x,v in map:  
        if x == name: return v
    if not globalvars.has_key(name): print 'Undefined (defaulting to 0):', name
    return globalvars.get(name, 0)

def stack_input(scanner,ign):
    """Grab more input"""
    scanner.stack_input(raw_input(">?> "))

%%
parser Calculator:
    ignore:    "[ \r\t\n]+"
    ignore:    "[?]"         {{ stack_input }}

    token END: "$"
    token NUM: "[0-9]+"
    token VAR: "[a-zA-Z_]+"

    # Each line can either be an expression or an assignment statement
    rule goal:   expr<<[]>> END            {{ print '=', expr }}
                                           {{ return expr }}
               | "set" VAR expr<<[]>> END  {{ globalvars[VAR] = expr }}
                                           {{ print VAR, '=', expr }}
                                           {{ return expr }}

    # An expression is the sum and difference of factors
    rule expr<<V>>:   factor<<V>>         {{ n = factor }}
                     ( "[+]" factor<<V>>  {{ n = n+factor }}
                     |  "-"  factor<<V>>  {{ n = n-factor }}
                     )*                   {{ return n }}

    # A factor is the product and division of terms
    rule factor<<V>>: term<<V>>           {{ v = term }}
                     ( "[*]" term<<V>>    {{ v = v*term }}
                     |  "/"  term<<V>>    {{ v = v/term }}
                     )*                   {{ return v }}

    # A term is a number, variable, or an expression surrounded by parentheses
    rule term<<V>>:   
                 NUM                      {{ return int(NUM) }}
               | VAR                      {{ return lookup(V, VAR) }}
               | "\\(" expr "\\)"         {{ return expr }}
               | "let" VAR "=" expr<<V>>  {{ V = [(VAR, expr)] + V }}
                 "in" expr<<V>>           {{ return expr }}
%%
if __name__=='__main__':
    print 'Welcome to the calculator sample for Yapps 2.'
    print '  Enter either "<expression>" or "set <var> <expression>",'
    print '  or just press return to exit.  An expression can have'
    print '  local variables:  let x = expr in expr'
    # We could have put this loop into the parser, by making the
    # `goal' rule use (expr | set var expr)*, but by putting the
    # loop into Python code, we can make it interactive (i.e., enter
    # one expression, get the result, enter another expression, etc.)
    while 1:
        try: s = raw_input('>>> ')
        except EOFError: break
        if not s.strip(): break
        parse('goal', s)
    print 'Bye.'

