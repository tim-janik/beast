;; 
;; Copyright (C) 2007 Stefan Westerfeld, stefan@space.twc.de
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
;; usage: bsescm -s transpose_freq.scm <freq> <semitones> <fine-tune>
;;
;; mini script to calculate the effect of 12-TET transposing a frequency
;;
(define (transpose-freq freq semitones fine-tune)
  (let*
    ((total-transpose-cent (+ (* 100 (string->number semitones)) (string->number fine-tune)))
	 (new-freq (* (expt 2 (/ total-transpose-cent 1200)) (string->number freq))))
	(display new-freq)
	(newline)))
(apply transpose-freq (cdr (command-line)))
