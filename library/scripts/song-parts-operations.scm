;; 
;; Copyright (C) 2003 Stefan Westerfeld, stefan@space.twc.de
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
;; (bse-script-register <func> <options> <category> <blurb> <author> <license> ARGS...)
;;

(bse-script-register 'song-parts-crop
		     ""
                     "/Song/Crop parts (loop range)"
		     (string-append "Crops all parts within the loop range "
				    "and moves parts after the loop range backwards accordingly.")
		     "Stefan Westerfeld"
		     "GNU General Public License"
		     (bse-param-song "song"))

(bse-script-register 'song-parts-duplicate
		     ""
                     "/Song/Duplicate parts (loop range)"
		     (string-append "Duplicate all parts within the loop range "
				    "and moves parts after the loop range forward accordingly.")
		     "Stefan Westerfeld"
		     "GNU General Public License"
		     (bse-param-song "song"))

;; common code for duplicate & crop:
;; error checking, computing boundaries, undo, applying algorithm to each track
(define (song-parts-operation song operation)
  (if (not (bse-is-song song))
      (bse-script-exit 'error "no valid song supplied"))
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
        (for-each process-track (bse-container-list-items song))
        (bse-item-ungroup-undo song))
      (bse-script-exit 'error "loop range is empty"))))

;; algorithm for cropping parts
(define (song-parts-crop song)
  (song-parts-operation song
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
  (song-parts-operation song
    (lambda (track start end len)
      (for-each
        (lambda (track-part) (let ((tick (bse-rec-get track-part 'tick))
				   (part (bse-rec-get track-part 'part)))
				  (cond ((>= tick end)   (bse-track-insert-part track (+ tick len) part)
						         (bse-track-remove-tick track tick))
				        ((>= tick start) (bse-track-insert-part track (+ tick len) part)))))
        (reverse (bse-track-list-parts track))))))

;; vim:set sw=2 sts=2 ts=8:
