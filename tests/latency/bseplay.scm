;; bseplay.scm - silently play .bse files		-*- scheme -*-
;; Copyright (C) 2001-2002 Tim Janik
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

;; play argc/argv contents as bse files
(define (bse-play)
  (bse-server-register-blocking bse-server-register-core-plugins #f)
  ; (bse-server-register-blocking bse-server-register-scripts #f)
  ; (bse-server-register-blocking bse-server-register-ladspa-plugins #f)
  (map (lambda (file)
	 (let ((project (bse-server-use-new-project bse-server file))
	       (blimp "*"))
	   (bse-project-restore-from-file project file)
	   (bse-project-play project)
	   ;; (display file)
	   ;; (display ": -")
	   (while (bse-project-is-playing project)
		  (usleep 250000)
		  ;; (display #\backspace)
		  ;; (display blimp)
		  (if (string=? blimp "*")
		      (set! blimp "o")
		      (set! blimp "*")))
	   (bse-project-stop project)
	   (bse-item-unuse project)
	   ;; (display #\backspace)
	   ;; (display "done.")
	   ;; (newline)
	   ))
       (cdr (command-line))))
(bse-play)
