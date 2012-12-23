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
(use-modules (ice-9 getopt-long))
;; load all stock BSE plugins
(bse-server-register-blocking bse-server-register-core-plugins #f)
;; no script registration
;;(bse-server-register-blocking bse-server-register-scripts #f)
;; no LADSPA plugin registration
;;(bse-server-register-blocking bse-server-register-ladspa-plugins #f)
;; playback + recording procedure
(define (bse-2-wav bse-file wav-file seconds)
  (let* ((counter 0)
	 ;; create a new project
	 (project (bse-server-use-new-project bse-server bse-file))
	 ;; load file contents into project
	 (error (bse-project-restore-from-file project bse-file)))
    ;; check errors
    (if (bse-error-test error)
	(begin
	  (display (string-append bse-file ": "
				  "failed to load project: "
				  (bse-error-blurb error)
				  "\n")
		   (current-error-port))
	  (exit 1)))
    ;; record to WAV
    (display (string-append "Recording to WAV file: " wav-file "\n"))
    (bse-server-start-recording bse-server wav-file seconds)
    ;; play project and indicate progress
    (display (string-append "Playing " bse-file ": -"))
    (bse-project-auto-deactivate project 0)
    (bse-project-play project)
    (while (bse-project-is-playing project)
	   (usleep 10000)
	   (let ((rem           (remainder counter 50))
                 (display-blimp (lambda (blimp)
                                  (display #\backspace)
                                  (display blimp))))
             (if (= rem 0)  (display-blimp "*"))
             (if (= rem 25) (display-blimp "o")))
           (set! counter (1+ counter)))
    ;; cleanup
    (bse-project-stop project)
    (display #\backspace)
    (display "done.\n")
    ;; release the newly created project
    (bse-item-unuse project)))
;; Program usage
(define (exit-usage exit-code)
  (display "Usage: bse2wav [options...] <bse-file> <wav-file>\n")
  (display "  --help, -h           Show this usage information\n")
  (display "  --seconds <seconds>  Number of seconds to record\n")
  (exit exit-code))
;; Command line synopsis
(define
  command-synopsis
  '(
    (help    (single-char #\h) (value #f))
    (seconds (value #t))
    ))
;; Parse command line and exec
(let* ((options (getopt-long (command-line) command-synopsis))
       (max-seconds (option-ref options 'seconds "0"))
       (args (option-ref options '() '())))
  (if (option-ref options 'help #f)
      (exit-usage 0))
  (if (not (= 2 (length args)))
      (exit-usage 1))
  (apply bse-2-wav (append args (list (string->number max-seconds)))))
