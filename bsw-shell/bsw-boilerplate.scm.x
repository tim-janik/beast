; -*- scheme -*-

(define bsw-server (bsw-server-get))
(define (bsw-param-range-stringify value min max step) (string-append (number->string value) " "
                                                                      (number->string min)   " "
                                                                      (number->string max)   " "
                                                                      (number->string step)))
(define (bsw-param-string name dflt) (string-append "BseParamString:" name ":" dflt))
(define (bsw-param-bool   name dflt) (string-append "BseParamBool:" name ":" (if dflt "1" "0")))
(define (bsw-param-frange name range) (string-append "BseParamFRange:" name ":"
                                                     (number->string (car range)) " "
                                                     (number->string (cadr range)) " "
                                                     (number->string (caddr range)) " "
                                                     (number->string (cadddr range))))
(define (bsw-param-irange name range) (string-append "BseParamIRange:" name ":"
                                                     (number->string (car range)) " "
                                                     (number->string (cadr range)) " "
                                                     (number->string (caddr range)) " "
                                                     (number->string (cadddr range))))
(define (bsw-param-note   name dflt) (string-append "BseNote:" name ":" dflt))

