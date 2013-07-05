;; CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0

;;
;; usage: bsescm -s noteplaytest.scm <instrument> <note> <time_ms>
;;
;; plays instrument at the specified frequency for the specified amount of time in ms
;;

(define (load-wave wave-repo wave-file)
  (let*
    ((error (bse-wave-repo-load-file wave-repo wave-file)))
      (display
	    (if
	      (bse-error-test error)
	      (string-append "FAILED: " wave-file ": " (bse-error-blurb error))
	      (string-append "OK:     " wave-file)))
	  (newline)))

(define (play-project project)
  (begin
    (display "playing project: ")
    (bse-server-register-blocking bse-server-register-core-plugins #f)
    (bse-server-register-blocking bse-server-register-scripts #f)
    (bse-server-register-blocking bse-server-register-ladspa-plugins #f)
    (let ((blimp "*"))
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
      (display "done.")
      (newline)
      )))

(define (test-play-note instrument note time_ms)
  (let*
    ((project (bse-server-use-new-project bse-server "testproject"))
     (wave-repo (bse-project-get-wave-repo project))
     (song (bse-project-create-song project "testsong"))
     (track (bse-song-create-track song))
     (part (bse-song-create-part song))
     (part_id (bse-track-insert-part track 0 part))
     (note_id (bse-part-insert-note-auto part 0 (/ (* (string->number time_ms) (* 2 384)) 1000) (string->number note) 0 1)))
    (load-wave wave-repo instrument)
    (bse-song-ensure-master-bus song)
    (bse-track-ensure-output track)
    (bse-item-set track "wave" (car (bse-container-list-children wave-repo)))
    (play-project project)))

(apply test-play-note (cdr (command-line)))
