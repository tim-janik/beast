;; 
;; Copyright (C) 2004 Stefan Westerfeld, stefan@space.twc.de
;; 
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2 of the License, or
;; (at your option) any later version.
;; 
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;; 
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
;;

;;
;; usage: bsesh -s waveloadtest.scm <wav-files>
;; checks whether wave files load properly with BSE
;;

(define (clear-wave-repo w)
  (map (lambda (arg) (bse-wave-repo-remove-wave w arg)) (bse-container-list-children w)))

(define (test-load-wave wave-repo wave-file)
  (let*
    ((error (bse-wave-repo-load-file wave-repo wave-file)))
      (display
	    (if
	      (bse-test-error error)
	      (string-append "FAILED: " wave-file ": " (bse-error-blurb error))
		  (string-append "OK:     " wave-file)))
	  (newline)
	  (flush-all-ports))) ;; allows 'bsesh -s waveloadtest.scm * |tee ...' constructions work properly

(define test-load-waves
  (lambda wave-files
    (let*
      ((project (bse-server-use-new-project bse-server "bar"))
	   (wave-repo (bse-project-get-wave-repo project)))
	  (map
	    (lambda (wave-file) (test-load-wave wave-repo wave-file))
	    wave-files))))

(apply test-load-waves (cdr (command-line)))
