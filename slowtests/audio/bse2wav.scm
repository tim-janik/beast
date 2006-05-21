;; -*- scheme -*-
;; Copyright (C) 2006 Tim Janik
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

;; load all stock BSE plugins
(bse-server-register-blocking bse-server-register-core-plugins #f)
;; no script registration
;;(bse-server-register-blocking bse-server-register-scripts #f)
;; no LADSPA plugin registration
;;(bse-server-register-blocking bse-server-register-ladspa-plugins #f)

(define (bse-2-wav bse-file wav-file)
  (let* ((blimp "*")
	 ;; create a new project
	 (project (bse-server-use-new-project bse-server bse-file))
	 ;; load file contents into project
	 (error (bse-project-restore-from-file project bse-file)))
    ;; check errors
    (if (bse-test-error error)
	(begin
	  (display (string-append bse-file ": "
				  "failed to load project: "
				  (bse-error-blurb error)
				  "\n")
		   (current-error-port))
	  (exit 1)))
    ;; record to WAV
    (display (string-append "Recording to WAV file: " wav-file "\n"))
    (bse-item-set bse-server "wave-file" wav-file)
    ;; play project and indicate progress
    (display (string-append "Playing " bse-file ": -"))
    (bse-project-play project)
    (while (bse-project-is-playing project)
	   (usleep 250000)
	   (display #\backspace)
	   (display blimp)
	   (if (string=? blimp "*")
	       (set! blimp "o")
	       (set! blimp "*")))
    ;; cleanup
    (bse-project-stop project)
    (display #\backspace)
    (display "done.\n")
    ;; release the newly created project
    (bse-item-unuse project)))

;; check command line and exec
(let
    ((args (cdr (command-line))))
  (if (not (= 2 (length args)))
      (begin
	(display
	 "Usage: bsesh -s bse2wav.scm <bse-file> <wav-file>\n"
	 (current-error-port))
	(exit 1))
      (apply bse-2-wav args)))
