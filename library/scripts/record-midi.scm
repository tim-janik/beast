;; Copyright (C) 2002,2005 Tim Janik
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

(bse-script-register 'record-midi
		     ""
                     (N_ "/Part/Record Midi...")
		     (N_ "Record midi events, currently being played on the default external "
			 "midi device, into a given song part. The MIDI recorder is still work in progress.")
		     "Tim Janik"
		     "Provided \"as is\", WITHOUT ANY WARRANTY;"
		     (bse-param-part (N_ "Part"))
		     (bse-param-bool (N_ "Start Now") #t))

(define (record-midi part start-now)
  (define do-recording #f)
  (define (record-note start duration note freq fine-tune velocity)
    (if do-recording
	(begin (display "MIDI Event recorded: ")
	       (display start)
	       (display " ")
	       (display duration)
	       (display " ")
	       (display note)
	       (display " ")
	       (display fine-tune)
	       (display " ")
	       (display velocity)
	       (newline)
	       (bse-part-insert-note-auto part start duration note fine-tune velocity))))
  (let ((midi-nofifier (bse-project-get-midi-notifier (bse-item-get-project part)))
	(note-vector (make-vector 128 0))
	(start-stamp 0)
	(stamp-ticks 0))
    (if (not (bse-is-item part))
	(bse-exit-error 'text1 (_ "No valid part object supplied")
			'text2 (_ "You probably want to start this script from a part editor.")))
    (bse-script-add-action "start" "Start Recording")
    (bse-script-add-action "stop" "Stop Recording")
    (bse-script-set-status (_ "Not currently recording...\n\n"
			      "The MIDI recorder is still work in progress.\n"
			      "Currently, you need a MIDI Synthesizer network "
			      "running, in order for this script to catch the "
			      "events currently being played back."))
    (bse-signal-connect (bse-script-janitor) "action::start" (lambda (proxy . rest)
							       (set! do-recording #t)
							       (bse-script-set-status "Currently recording...")))
    (bse-signal-connect (bse-script-janitor) "action::stop" (lambda (proxy . rest)
							      (set! do-recording #f)
							      (set! start-stamp 0)
							      (bse-script-set-status "Stopped recording...")))
    (bse-signal-connect midi-nofifier "midi-event"
			(lambda (proxy event)
			  (let ((etype     (bse-rec-get event 'event-type))
				(channel   (bse-rec-get event 'channel))
				(stamp     (bse-rec-get event 'tick-stamp))
				(frequency (bse-rec-get event 'frequency))
				(velocity  (bse-rec-get event 'velocity)))
			    (if (= 0 stamp-ticks)
				;; timing->stamp_ticks maybe 0 before the first MIDI event
				(let ((timing (bse-part-get-timing part 0)))
				  (bse-rec-print timing) (newline)
				  (set! stamp-ticks (bse-rec-get timing 'stamp-ticks))))
			    (cond ((bse-choice-match? etype 'note-on)
				   (let ((note (bse-note-from-freq frequency)))
				     (vector-set! note-vector note stamp)
				     (if (= start-stamp 0)
					 (set! start-stamp stamp))))
				  ((bse-choice-match? etype 'note-off)
				   (let ((note (bse-note-from-freq frequency)))
				     (if (not (= 0 (vector-ref note-vector note)))
					 (let ((diff (- stamp (vector-ref note-vector note)))
					       (start (- (vector-ref note-vector note) start-stamp)))
					   (vector-set! note-vector note 0)
					   (record-note (* stamp-ticks start) (* stamp-ticks diff) note frequency 0 velocity)))))
				  (else
				   (display (string-append "MIDI Event ignored: "
							   (symbol->string etype) "("
							   (number->string channel) ") "
							   (number->string stamp) " "
							   (number->string frequency) " "
							   (number->string velocity)))
				   (newline))))))
    (while (or (bse-context-pending) #t)
	   (bse-context-iteration #t))))
