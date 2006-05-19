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
;; usage: bsesh -s bsetowav.scm <bse-file> <wav-file>
;; converts the .bse file <bse-file> to the .wav file <wav-file>
;;

(define (convert-bse-to-wav bse-file wav-file)
  (begin
    (display "converting bse file ")
    (display bse-file)
    (display " to wave file ")
    (display wav-file)
    (display ":  ")
    (let ((project (bse-server-use-new-project bse-server bse-file))
	  (blimp "*"))
      (bse-project-restore-from-file project bse-file)
      (bse-item-set bse-server "wave-file" wav-file)
      (usleep 10000)
      (bse-project-play project)
      (usleep 10000)
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
      (display " ")
      (newline)
      (display "done.")
      (newline)
      )))

(bse-server-register-blocking bse-server-register-core-plugins #f)
;;(bse-server-register-blocking bse-server-register-scripts #f)
;;(bse-server-register-blocking bse-server-register-ladspa-plugins #f)
;;(display "commandline:")(display (command-line))(newline)
(apply convert-bse-to-wav (cdr (command-line)))
