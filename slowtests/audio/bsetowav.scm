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
    (bse-server-register-blocking bse-server-register-core-plugins #f)
    (bse-server-register-blocking bse-server-register-scripts #f)
    (bse-server-register-blocking bse-server-register-ladspa-plugins #f)
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
      (display "done.")
      (newline)
      )))

(apply convert-bse-to-wav (cdr (command-line)))
