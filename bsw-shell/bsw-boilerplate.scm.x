; -*- scheme -*-

(define bsw-server (bsw-server-get))
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
(define (bsw-param-note    name dflt) (string-append "BseNote:" name ":" dflt))
(define (bsw-param-proxy   name type) (string-append "BseParamProxy" type ":" name ":" "0"))
(define (bsw-param-project name) (bsw-param-proxy name "BseProject"))
(define (bsw-param-part    name) (bsw-param-proxy name "BsePart"))
(define (bsw-param-snet    name) (bsw-param-proxy name "BseSNet"))
(define (bsw-param-item    name) (bsw-param-proxy name "BseItem"))
(define (bsw-param-source  name) (bsw-param-proxy name "BseSource"))

(define (bsw-item-set proxy . list)
  (while (pair? list)
	 (bsw-glue-set-prop proxy (car list) (cadr list))
	 (set! list (cddr list))))

(define bsw-module-set bsw-item-set)
	
(define (bsw-test-error err) (not (bsw-enum-match? err 'error-none)))

(define (bsw-item-tname item) (bsw-item-get-name-or-type item))

(define (bsw-module-connect-ochannel omod ochan imod ichan)
  (let ((err (bsw-source-set-input imod ichan omod ochan)))
    (if (bsw-test-error err)
	(error (string-append "failed to connect input \""
			      ichan
			      "\" of "
			      (bsw-item-tname imod)
			      " to output \""
			      ochan
			      "\" of "
			      (bsw-item-tname omod)
			      ": "
			      (bsw-error-blurb err))))))
