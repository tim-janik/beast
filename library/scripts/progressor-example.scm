; (bsw-script-register 'script-function
;                      "/Scripts/Category..."
;                      "Simple blurb line"
;                      "Long Help chunk, describing what's up with this"
;                      "Tim Janik (Author)"
;                      "Tim Janik 2002 (Copyright)"
;                      "May 2002"
;                      (bsw-param-string "Text" "Default")
;                      (bsw-param-bool   "Mark-me" #f)
;                      (bsw-param-irange "IntNum" '(16 -100 +100 5))
;                      (bsw-param-frange "FloatNum" '(42 0 1000 10))
;                      (bsw-param-note   "Note" "C-7"))

(bsw-script-register 'progressor
                     "/Scripts/Test/Progressor..."
		     "Progressor is a quick example script"
		     (string-append "Progressor takes two seed values and then starts progressing. "
				    "It doesn't do anything particularly usefull, other than "
				    "ticking the main program from time to time.")
		     "Tim Janik (Author)"
		     "Tim Janik 2002 (Copyright)"
		     "1999, 2002"
		     (bsw-param-irange "N Iterations" '(512 0 65536 128))
		     (bsw-param-irange "N Wait Spins" '(256 0 65536 64))
		     (bsw-param-bool   "Update Percentage" #t))

(define (progressor niter nwait uperc)
  (let ((total niter))
    (while (> niter 0)
	   (set! niter (- niter 1))
           (let ((busy-spin nwait))
             (while (> busy-spin 0)
                    (set! busy-spin (- busy-spin 1))))
	   ; (display (- 1.0 (/ (* 1.0 niter) total))) (newline)
	   (if uperc
	       (bsw-progress (- 1.0 (/ (* 1.0 niter) total)))
	       (bsw-progress -1)
	       ))
    ; final progress mark
    (bsw-progress 1)))

