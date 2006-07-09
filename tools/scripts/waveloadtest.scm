;; 
;; Copyright (C) 2004 Stefan Westerfeld, stefan@space.twc.de
;; 
;; This software is provided "as is"; redistribution and modification
;; is permitted, provided that the following disclaimer is retained.
;;
;; This software is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;; In no event shall the authors or contributors be liable for any
;; direct, indirect, incidental, special, exemplary, or consequential
;; damages (including, but not limited to, procurement of substitute
;; goods or services; loss of use, data, or profits; or business
;; interruption) however caused and on any theory of liability, whether
;; in contract, strict liability, or tort (including negligence or
;; otherwise) arising in any way out of the use of this software, even
;; if advised of the possibility of such damage.

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
