;; 
;; Copyright (C) 2003-2004 Stefan Westerfeld, stefan@space.twc.de
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
;; (bse-script-register <func> <options> <category> <blurb> <author> <license> ARGS...)
;;

(bse-script-register 'part-harmonic-transposer
		     ""
                     (N_ "/Part/Harmonic Transposer")
		     ;; FIXME: the description may be suboptimal; the problem is
		     ;; that I can't precisely describe details of musical theory
		     ;; in english. -- stw
		     (N_ "The harmonic transposer takes the selection of a part "
			 "and transposes it to different harmonies. If you for "
			 "instance have selected a measure filled with C major "
			 "chords, and enter \"C,Am,F,G\" as harmonic sequence, "
			 "the result will be four measures, filled with C major, "
			 "A minor, F major and G major chords. "
			 "\n\n"
			 "This also works for melodies, so you can transpose a "
			 "whole melody written in G major to D minor. The standard "
			 "scales used in church music (ionian, dorian, phrygian, "
			 "lydian, mixolydian, aeolian, locrian) are also supported: "
			 "it is for instance possible to write Ddorian or Caeolian. "
			 "The aeolian scale is equivalent to minor and the ionian "
			 "scale is equivalent to major. "
			 "\n\n"
			 "Since musically, there is no preference on whether to transpose up or "
			 "down it is possible to specify the first harmony that will be transposed "
			 "down (all harmonies below this will be transposed up). It is possible "
			 "to omit this value. Then all notes will be transposed up.")
		     "Stefan Westerfeld"
		     "Provided \"as is\", WITHOUT ANY WARRANTY"
		     (bse-param-part   (N_ "Part"))
		     (bse-param-string (N_ "Harmony Sequence") "C,Amin,F,G")
		     (bse-param-string (N_ "Transpose down starting at") "F"))

;; ------------------- parser for harmony strings ------------------------

;; parses a constant string (terminal symbols)
(define (parse-constant input match)
  (and
    input
    (let* ((str (car input))
	   (strlen (string-length str))
	   (matchlen (string-length match)))
      (and
	(<= matchlen strlen)
	(string=? (substring str 0 matchlen) match)
	(cons (substring str matchlen strlen) (cdr input))))))

;; FIXME: should return the shortest string
(define (parse-multiple input . alternatives)
  (if (null? alternatives)
    #f
    (if (car alternatives)
      (car alternatives)
      (apply parse-multiple input (cdr alternatives)))))

;; for generating parse results: adds a new element into the parse result list
(define (parse-result input result)
  (and input
    (cons (car input) (cons result (cdr input)))))

;; change the first element in the parse result list by applying function to it
(define (parse-change-result input function)
  (and input
    (cons (car input) (cons (function (cadr input)) (cddr input)))))

;; BaseNote := C D E F G A B H
(define (parse-basenote input)
  (parse-multiple input
    (parse-result (parse-constant input "C") 0)
    (parse-result (parse-constant input "D") 2)
    (parse-result (parse-constant input "E") 4)
    (parse-result (parse-constant input "F") 5)
    (parse-result (parse-constant input "G") 7)
    (parse-result (parse-constant input "A") 9)
    (parse-result (parse-constant input "B") 11)
    (parse-result (parse-constant input "H") 11)))

;; FullNote := # <BaseNote> | <BaseNote> # | <BaseNote> is | <BaseNote>
(define (parse-fullnote input)
  (parse-multiple input
    (parse-change-result (parse-basenote (parse-constant input "#")) 1+)
    (parse-change-result (parse-constant (parse-basenote input) "#") 1+)
    (parse-change-result (parse-constant (parse-basenote input) "is") 1+)
    (parse-basenote input)))

;; Harmony := <FullNote> maj | <FullNote> min | <FullNote> m | <FullNote>
(define (parse-harmony input)
  (let ((scale (lambda (shift) (lambda (note) (cons note shift))))
	(major (lambda (note) (cons note 0)))
        (minor (lambda (note) (cons note 5))))
    (parse-multiple input
      ;; http://en.wikipedia.org/wiki/Musical_mode
      ;; http://de.wikipedia.org/wiki/Kirchentonart
      (parse-change-result (parse-constant (parse-fullnote input) "ionian") (scale 0))
      (parse-change-result (parse-constant (parse-fullnote input) "dorian") (scale 1))
      (parse-change-result (parse-constant (parse-fullnote input) "phrygian") (scale 2))
      (parse-change-result (parse-constant (parse-fullnote input) "lydian") (scale 3))
      (parse-change-result (parse-constant (parse-fullnote input) "mixolydian") (scale 4))
      (parse-change-result (parse-constant (parse-fullnote input) "aeolian") (scale 5))
      (parse-change-result (parse-constant (parse-fullnote input) "locrian") (scale 6))
      ;; conventional harmonic major/minor (which is the same as ionian/aeolian in our notation)
      (parse-change-result (parse-constant (parse-fullnote input) "maj") major)
      (parse-change-result (parse-constant (parse-fullnote input) "min") minor)
      (parse-change-result (parse-constant (parse-fullnote input) "m") minor)
      (parse-change-result (parse-fullnote input) major))))

;; HarmonySeq := <Harmony> , <HarmonySeq> | <Harmony>
(define (parse-harmony-seq input)
  (if input
    (parse-multiple input
      (parse-harmony-seq (parse-constant (parse-harmony input) ","))
      (parse-harmony input))
   #f))

;; generate a list of harmonies from a string
(define (harmony-list-from-string str)
  (let* ((parse-result (parse-harmony-seq (cons str '()))))
    (if (and parse-result (= 0 (string-length (car parse-result))))
      (reverse (cdr parse-result))
      #f)))

;;---------------- harmonic transform functions ---------------------

;; white keys (starting at C) of a keyboard
(define harmony-scale '(0 2 4 5 7 9 11))

;; compute rotated version of white keys (starting at shift, where shift
;; is the number of the white key (i.e. 5 = fifth key = A)
(define (harmony-shift-scale shift)
  (map
    (lambda (note)
      (modulo
	(+ (- 12 (list-ref harmony-scale shift))
	  (list-ref
	    harmony-scale
	    (modulo
	      (+ (list-index harmony-scale note) shift)
	      (length harmony-scale))))
	12))
    harmony-scale))

;; all semitones
(define harmony-maj-maj '(0 1 2 3 4 5 6 7 8 9 10 11))

;; computes what to do with each semitone when transposing from
;; harmony srcscale to destscale
(define (harmony-shift srcscale destscale input)
  (map
    (lambda (note)
      (let* ((scale (harmony-shift-scale srcscale))
	     (len (length scale))
	     (shift (modulo (- destscale srcscale) len))
	     (index (list-index scale note))
	     (delta (modulo
		      (if index
			(- (list-ref scale (modulo (+ shift index) len)) (list-ref scale shift))
			note)
		      12)))
	delta))
      input))

;; prints a list of things, writing out strings, and displaying other values
;; concludes with a newline
(define (myprint . l)
  (if (null? l)
    (newline)
    (begin
      (if (string? (car l))
	(map write-char (string->list (car l)))
	(display (car l)))
      (apply myprint (cdr l)))))

;; transposes one midi note (integer) from one harmony in another
;; down indicates whether to go down rather than up
(define (harmony-transpose note from to down)
  (let* ((rnote	      (modulo (- note (car from)) 12))
	 (srcscale    (cdr from))
	 (destscale   (cdr to))
	 (stransform  (harmony-shift srcscale destscale harmony-maj-maj))
	 (delta	      (+
			(- (list-ref stransform rnote) rnote)
			(modulo (- (car to) (car from)) 12)
			(if down -12 0))))
    (+ note delta)))

;; harmonic transposer: implementation
(define (part-harmonic-transposer part harmony-string down-string)
  (if (not (bse-is-part part))
      (bse-exit-error 'text1 (_ "No valid part supplied")))
  (let* ((notes             (bse-part-list-selected-notes part))
	 (error-check-1     (if (< (length notes) 1) (bse-exit-error 'text1 (_ "No notes selected"))))
	 (get-note-start    (lambda (rec) (bse-rec-get rec 'tick)))
	 (get-note-end      (lambda (rec) (+ (get-note-start rec) (bse-rec-get rec 'duration))))
	 (start             (apply min (map get-note-start notes)))
	 (end               (apply max (map get-note-end notes)))
	 (len               (- end start))
	 (harmony-list-all  (harmony-list-from-string harmony-string))
	 (error-check-2     (if (not harmony-list-all) (bse-exit-error 'text1 (_ "Failed to parse harmony list"))))
	 (base-harmony      (car harmony-list-all))
	 (harmony-list      (cdr harmony-list-all))
	 (error-check-3     (if (< (length harmony-list) 1) (bse-exit-error 'text1 (_ "Harmony list is too short"))))
	 (down-harmony-list (harmony-list-from-string down-string))
	 (down-harmony      (if down-harmony-list (car (harmony-list-from-string down-string)) #f))
	 (process-note
	   (lambda (rec harmony offset)
	     (bse-part-insert-note-auto
		   part
		 (+ offset (bse-rec-get rec 'tick))
		 (bse-rec-get rec 'duration)
		 (harmony-transpose (bse-rec-get rec 'note) base-harmony harmony
		    (>=
		      (modulo (- (car harmony) (car base-harmony)) 12)
		      (if down-harmony
			(modulo (- (car harmony) (car down-harmony)) 12)
			12)))
		 (bse-rec-get rec 'fine-tune)
		 (bse-rec-get rec 'velocity))
		)))
    (letrec ((process-harmonies (lambda (note harmony-list offset)
				  (if (null? harmony-list)
				    #t
				    (begin
				      (process-note note (car harmony-list) offset)
				      (process-harmonies note (cdr harmony-list) (+ offset len)))))))
      (begin
	(bse-item-group-undo part "part-harmonic-transposer")
	(for-each
	  (lambda (note)
	    (process-harmonies note harmony-list len))
	  notes)
        (bse-item-ungroup-undo part)))))


;; vim:set sw=2 sts=2 ts=8:
