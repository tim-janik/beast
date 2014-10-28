;; CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
;; -*- scheme -*-

;; allow i18n markup of strings
(define _  (lambda arglist (bse-gettext (apply string-append arglist))))   ;; translate concatenated string
(define U_ (lambda arglist (bse-gettext (apply string-append arglist))))   ;; for UTF-8 strings
(define Q_ (lambda arglist (bse-gettext-q (apply string-append arglist)))) ;; for context sensitive translations
(define N_ (lambda arglist (apply string-append arglist)))                 ;; not translated (or later)

;; cache invocations to avoid extra round trips
(define bse-script-janitor
  (let ((result (bse-script-janitor)))
    (lambda () result)))

;; leave a message and exit
(define bse-exit-message (lambda arglist (apply bse-script-message arglist) (bse-script-quit)))
(define bse-exit-error   (lambda arglist (apply bse-script-message 'error arglist) (bse-script-quit)))

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
(define (bse-param-song    name) (bse-param-proxy name "BseSong"))
(define (bse-param-source  name) (bse-param-proxy name "BseSource"))

(define (bse-item-set proxy . list)
  (while (pair? list)
	 (bse-glue-set-prop proxy (car list) (cadr list))
	 (set! list (cddr list))))

(define (bse-item-get proxy pname)
  (bse-glue-get-prop proxy pname))

(define bse-module-set bse-item-set)
	
(define (bse-error-test err) (not (bse-choice-match? err 'bse-error-none)))

(define (bse-item-tname item) (bse-item-get-name-or-type item))

(define (bse-module-connect-ochannel omod ochan imod ichan)
  (let ((err (bse-source-set-input imod ichan omod ochan)))
    (if (bse-error-test err)
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

;; boilerplate code for plugin/script registration
(define (bse-server-register-blocking register-request verbose)
  (let ((registration-done #f))
    (let ((sigid (bse-signal-connect bse-server "registration"
				     (lambda (server registration-type what error-msg)
				       (if (and verbose (string? what))
					   (display (string-append
						     "registering "
						     what
						     "...\n")))
				       (if (string? error-msg)
					   (display (string-append
						     "WARNING: registration failed for "
						     what
						     ": " error-msg "\n")))
				       (set! registration-done
					     (eq? registration-type 'bse-register-done))))))
      (register-request bse-server)
      (while (not registration-done)
	     (bse-context-iteration #t))
      (bse-signal-disconnect bse-server sigid))))

;; play argc/argv contents as bse files
(define (bse-scm-auto-play)
  (bse-server-register-blocking bse-server-register-core-plugins #f)
  (bse-server-register-blocking bse-server-register-scripts #f)
  (bse-server-register-blocking bse-server-register-ladspa-plugins #f)
  (map (lambda (file)
	 (let ((project (bse-server-use-new-project bse-server file))
	       (blimp "*"))
	   (bse-project-restore-from-file project file)
	   (bse-project-play project)
	   (display file)
	   (display ": -")
	   (while (bse-project-is-playing project)
		  (usleep 250000)
		  (display #\backspace)
		  (display blimp)
		  (if (string=? blimp "*")
		      (set! blimp "o")
		      (set! blimp "*")))
	   (bse-project-stop project)
	   (bse-item-unuse project)
	   (display #\backspace)
	   (display "done.")
	   (newline)
	   ))
       (cdr (command-line))))
