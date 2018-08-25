
# AWK file to extract translatable strings from single line assignments

/^#/ { print "// " substr ($0, 2) ; next }

/^[[:space:]]*_([^=]*)=/ {
    name = $0; value = $0
    sub (/=.*/, "", name)
    sub (/[[:space:]]*_/, "", name)
    sub (/^[^=]*=/, "", value)
    gsub ("[\\\"]", "\\" "\\&", value)
    print "__INTLFIX__: /*" name "*/_(\"" value "\");"
    next
}

{ print "\t; " $0 " ;" }

