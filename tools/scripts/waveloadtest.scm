;; CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
;;
;; usage: bsescm -s waveloadtest.scm <wav-files>
;; checks whether wave files load properly with BSE
;;
(define (clear-wave-repo wave-repo)
  (map
	(lambda (arg) (bse-wave-repo-remove-wave wave-repo arg))
	(bse-container-list-children wave-repo)))
(define (test-load-wave wave-repo wave-file)
  (let*
    ((error (bse-wave-repo-load-file wave-repo wave-file)))
      (display
	    (if
	      (bse-error-test error)
	      (string-append "FAILED: " wave-file ": " (bse-error-blurb error))
		  (string-append "OK:     " wave-file)))
	  (newline)
	  (flush-all-ports) ;; allows 'bsescm -s waveloadtest.scm * |tee ...' constructions work properly
	  (clear-wave-repo wave-repo)))
(define test-load-waves
  (lambda wave-files
    (let*
      ((project (bse-server-use-new-project bse-server "bar"))
	   (wave-repo (bse-project-get-wave-repo project)))
	  (map
	    (lambda (wave-file) (test-load-wave wave-repo wave-file))
	    wave-files))))
(apply test-load-waves (cdr (command-line)))
