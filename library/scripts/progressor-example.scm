;; Copyright (C) 1999, 2002 Tim Janik
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

;; (bse-script-register 'script-function
;;                      "unstable"                          ; Options
;;                      "/Project/Toys/My Script"
;;                      "Blurb, describing what's up with this"
;;                      "Tim Janik"                         ; Author
;;                      "GNU General Public License"        ; License
;;                      (bse-param-string "Text" "Default")
;;                      (bse-param-bool   "Mark-me" #f)
;;                      (bse-param-irange "IntNum" '(16 -100 +100 5))
;;                      (bse-param-frange "FloatNum" '(42 0 1000 10))
;;                      (bse-param-note   "Note" "C-7"))

(bse-script-register 'progressor
		     ""
                     "/Project/Toys/Progressor..."
		     (string-append "Progressor takes two seed values and then starts progressing. "
				    "It doesn't do anything particularly usefull, other than "
				    "ticking the main program from time to time. It is a funny example though.")
                     "Tim Janik"
		     "Provided \"as is\", WITHOUT ANY WARRANTY;"
		     (bse-param-irange "N Iterations" '(512 0 65536 128))
		     (bse-param-irange "N Wait Spins" '(256 0 65536 64))
		     (bse-param-bool   "Update Percentage" #t))

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
