;; 
;; Copyright (C) 2004-2005 Stefan Westerfeld, stefan@space.twc.de
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
;; usage: bsesh -s noteplaytest.scm <instrument> <note> <time_ms>
;;
;; plays instrument at the specified frequency for the specified amount of time in ms
;;

(define (load-wave wave-repo wave-file)
  (let*
    ((error (bse-wave-repo-load-file wave-repo wave-file)))
      (display
	    (if
	      (bse-test-error error)
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
