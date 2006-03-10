;; 
;; Copyright (C) 2003 Stefan Westerfeld, stefan@space.twc.de
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

(bse-script-register 'song-parts-crop
		     ""
                     (N_ "/Song/Crop parts (loop range)")
		     (N_ "Crops all parts within the loop range "
			 "and moves parts after the loop range backwards accordingly.")
		     "Stefan Westerfeld"
		     "GNU General Public License"
		     (bse-param-song (N_ "Song")))

(bse-script-register 'song-parts-duplicate
		     ""
                     (N_ "/Song/Duplicate parts (loop range)")
		     (N_ "Duplicate all parts within the loop range "
			 "and moves parts after the loop range forward accordingly.")
		     "Stefan Westerfeld"
		     "GNU General Public License"
		     (bse-param-song (N_ "Song")))

;; common code for duplicate & crop:
;; error checking, computing boundaries, undo, applying algorithm to each track
(define (song-parts-operation song errtitle operation)
  (if (not (bse-is-song song))
      (bse-exit-error 'text1 (_ "No valid song supplied")))
  (let* ((marker1 (max 0 (bse-item-get song "loop-left")))  ; handle loop marker -1 (unset) as 0 (set to start)
         (marker2 (max 0 (bse-item-get song "loop-right")))
	 (start   (min marker1 marker2))                    ; sort markers
	 (end     (max marker1 marker2))
	 (len     (- end start))
	 (process-track
	   (lambda (track)
	     (if (bse-is-track track)
	       (operation track start end len)))))
    (if (> end start)
      (begin
	(bse-item-group-undo song "song-parts-operation")
        (for-each process-track (bse-container-list-children song))
        (bse-item-ungroup-undo song))
      (bse-exit-message 'warning
			'text1 errtitle
			'text2 (_ "The loop range of the specified song contains no parts "
				  "or is unset, so no parts can be identified to operate on.")
			'check (_ "Show messages about empty part range")))))

;; algorithm for cropping parts
(define (song-parts-crop song)
  (song-parts-operation song (_ "Failed to crop part range.")
    (lambda (track start end len)
      (for-each
        (lambda (track-part) (let ((tick (bse-rec-get track-part 'tick))
				   (part (bse-rec-get track-part 'part)))
				  (cond ((>= tick end)   (bse-track-insert-part track (- tick len) part)
						         (bse-track-remove-tick track tick))
				        ((>= tick start) (bse-track-remove-tick track tick)))))
        (bse-track-list-parts track)))))

;; algorithm for duplication of parts
(define (song-parts-duplicate song)
  (song-parts-operation song (_ "Failed to duplicate part range.")
    (lambda (track start end len)
      (for-each
        (lambda (track-part) (let ((tick (bse-rec-get track-part 'tick))
				   (part (bse-rec-get track-part 'part)))
				  (cond ((>= tick end)   (bse-track-insert-part track (+ tick len) part)
						         (bse-track-remove-tick track tick))
				        ((>= tick start) (bse-track-insert-part track (+ tick len) part)))))
        (reverse (bse-track-list-parts track))))))

;; vim:set sw=2 sts=2 ts=8:
