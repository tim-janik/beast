;; -*- scheme -*-

;; cache invocations to avoid extra round trips
(define bse-janitor-get-specific
  (let ((result (bse-janitor-get-specific)))
    (lambda () result)))
;; script convenience
(define (bse-script-exit msg-type message) (bse-janitor-exit (bse-janitor-get-specific) msg-type message))
(define (bse-script-progress progress) (bse-janitor-progress (bse-janitor-get-specific) progress))

(define bse-server (bse-server-get))
(define (bse-param-string name dflt) (string-append "BseParamString:" name ":" dflt))
(define (bse-param-bool   name dflt) (string-append "BseParamBool:" name ":" (if dflt "1" "0")))
(define (bse-param-frange name range) (string-append "BseParamFRange:" name ":"
                                                     (number->string (car range)) " "
                                                     (number->string (cadr range)) " "
                                                     (number->string (caddr range)) " "
                                                     (number->string (cadddr range))))
(define (bse-param-irange name range) (string-append "BseParamIRange:" name ":"
                                                     (number->string (car range)) " "
                                                     (number->string (cadr range)) " "
                                                     (number->string (caddr range)) " "
                                                     (number->string (cadddr range))))
(define (bse-param-note    name dflt) (string-append "BseNote:" name ":" dflt))
(define (bse-param-proxy   name type) (string-append "BseParamProxy" type ":" name ":" "0"))
(define (bse-param-project name) (bse-param-proxy name "BseProject"))
(define (bse-param-part    name) (bse-param-proxy name "BsePart"))
(define (bse-param-snet    name) (bse-param-proxy name "BseSNet"))
(define (bse-param-item    name) (bse-param-proxy name "BseItem"))
(define (bse-param-source  name) (bse-param-proxy name "BseSource"))

(define (bse-item-set proxy . list)
  (while (pair? list)
	 (bse-glue-set-prop proxy (car list) (cadr list))
	 (set! list (cddr list))))

(define (bse-item-get proxy pname)
  (bse-glue-get-prop proxy pname))

(define bse-module-set bse-item-set)
	
(define (bse-test-error err) (not (bse-enum-match? err 'error-none)))

(define (bse-item-tname item) (bse-item-get-name-or-type item))

(define (bse-module-connect-ochannel omod ochan imod ichan)
  (let ((err (bse-source-set-input imod ichan omod ochan)))
    (if (bse-test-error err)
	(error (string-append "failed to connect input \""
			      ichan
			      "\" of "
			      (bse-item-tname imod)
			      " to output \""
			      ochan
			      "\" of "
			      (bse-item-tname omod)
			      ": "
			      (bse-error-blurb err))))))
