(bse-script-register 'record-midi
                     "/Part/Record Midi..."
		     "Record midi events"
		     (string-append "Record midi events, currently being played on the default external "
				    "midi device, into a given song part. The MIDI recorder is still work in progress.")
		     "Tim Janik"
		     "(C) Tim Janik, GPL"
		     "2002"
		     (bse-param-part "Part")
		     (bse-param-bool "Start Now" #t))

(define (record-midi part start-now)
  (define do-recording #f)
  (define (record-note note freq start duration)
    (if do-recording
	(begin (display "rec-note: ")
	       (display note)
	       (display " ")
	       (display freq)
	       (display " ")
	       (display start)
	       (display " ")
	       (display duration)
	       (newline)
	       (bse-part-insert-note part start duration freq 0))))
  (let ((mrec (bse-server-get-midi-notifier bse-server))
	(note-vector (make-vector 128 0))
	(start-stamp 0))
    (if (not (bse-is-item part))
	(bse-script-exit 'error
			 (string-append "record-midi: not a valid part object supplied.\n"
					"You probably want to start this script from a part editor")))
    (bse-script-add-action "start" "Start Recording")
    (bse-script-add-action "stop" "Stop Recording")
    (bse-script-set-msg 'info (string-append "Not currently recording...\n\n"
					     "The MIDI recorder is still work in progress.\n"
					     "Currently, you need a MIDI Synthesizer network "
					     "running, in order for this script to catch the "
					     "events currently being played back."))
    (bse-signal-connect (bse-script-control) "action::start" (lambda (proxy . rest)
							       (set! do-recording #t)
							       (bse-script-set-msg 'info "Currently recording...")))
    (bse-signal-connect (bse-script-control) "action::stop" (lambda (proxy . rest)
							      (set! do-recording #f)
							      (set! start-stamp 0)
							      (bse-script-set-msg 'info "Stopped recording...")))
    (bse-signal-connect mrec "midi-event"
			(lambda (proxy event)
			  (let ((status  (bse-record-get event 'status))
				(channel (bse-record-get event 'channel))
				(stamp   (bse-record-get event 'stamp))
				(data1   (bse-record-get event 'data1))
				(data2   (bse-record-get event 'data2)))
			    (cond ((bse-enum-match? status 'note-on)
				   (let ((note (bse-note-from-freq data1)))
				     (vector-set! note-vector note stamp)
				     (if (= start-stamp 0)
					 (set! start-stamp stamp))))
				  ((bse-enum-match? status 'note-off)
				   (let ((note (bse-note-from-freq data1)))
				     (if (not (= 0 (vector-ref note-vector note)))
					 (let ((diff (- stamp (vector-ref note-vector note)))
					       (start (- (vector-ref note-vector note) start-stamp)))
					   (vector-set! note-vector note 0)
					   (record-note note data1 start diff)))))
				  (else
				   (display (string-append "MIDI Event ignored: "
							   status " "
							   (number->string channel) " "
							   (number->string stamp) " "
							   (number->string data1) " "
							   (number->string data2)))
				   (newline))))))
    (while (or (bse-context-pending) #t)
	   (bse-context-iteration #t))))
