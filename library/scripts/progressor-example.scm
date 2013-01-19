;; CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
;; (bse-script-register 'script-function
;;                      "unstable"                          ; Options
;;                      (N_ "/Project/Toys/My Script")
;;                      (N_ "Blurb, describing what's up with this")
;;                      "Tim Janik"                         ; Author
;;                      "GNU General Public License"        ; License
;;                      (bse-param-string (N_ "Text") "Default")
;;                      (bse-param-bool   (N_ "Mark-me") #f)
;;                      (bse-param-irange (N_ "IntNum") '(16 -100 +100 5))
;;                      (bse-param-frange (N_ "FloatNum") '(42 0 1000 10))
;;                      (bse-param-note   (_ "Note") "C-7"))
(bse-script-register 'progressor
		     ""
                     (N_ "/Project/Toys/Progressor...")
		     (N_ "Progressor takes two seed values and then starts progressing. "
			 "It doesn't do anything particularly usefull, other than "
			 "ticking the main program from time to time. It is a funny example though.")
                     "Tim Janik"
		     "Provided \"as is\", WITHOUT ANY WARRANTY;"
		     (bse-param-irange (N_ "N Iterations") '(512 0 65536 128))
		     (bse-param-irange (N_ "N Wait Spins") '(256 0 65536 64))
		     (bse-param-bool   (N_ "Update Percentage") #t))
(define (progressor niter nwait uperc)
  (do ((i niter (- i 1)))
      ((<= i 0))
    (do ((busy-spin nwait (- busy-spin 1)))
	((<= busy-spin 0)))
    ;; (display (- 1.0 (/ (* 1.0 i) niter))) (newline)
    (bse-script-progress (if uperc
			     (- 1.0 (/ (* 1.0 i) niter))
			     -1)))
  ;; final progress mark
  (bse-script-progress 1))
