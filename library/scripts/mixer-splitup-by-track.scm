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

(bse-script-register 'mixer-splitup-by-track
		     ""
                     (N_ "/Song/Assign Tracks to individual Mixer Busses")
		     (N_ "This script creates a new Mixer Bus for each track in the "
			 "song that is currently using the Master Bus as output.")
		     "Stefan Westerfeld"
		     "Provided \"as is\", WITHOUT ANY WARRANTY"
		     (bse-param-song (N_ "Song")))

;; function to get song for a track (improves readability)
(define (track-get-song track)
  (bse-item-get-parent track))

;; check whether the track is connected (only) to the Master Bus
(define (track-uses-master-bus? track)
  ;; compare the the list of output names with the list containing the name of the master bus
  (equal?
    (map
      (lambda (bus) (bse-item-get bus "uname"))
      (bse-item-get track "outputs"))
    (cons (bse-item-get (bse-song-get-master-bus (track-get-song track)) "uname") '())))

;; create new bus and assign track output to it, removing all other outputs
(define (assign-track-to-new-bus track)
  (let* ((new-bus (bse-song-create-bus (track-get-song track))))
    (for-each
      (lambda (bus) (bse-bus-disconnect-track bus track))
      (bse-item-get track "outputs"))
    (bse-bus-connect-track new-bus track)
    (bse-bus-connect-bus (bse-song-get-master-bus (track-get-song track)) new-bus)))

;; create a new mixer channel for each track assigned (only) to the master bus
(define (mixer-splitup-by-track song)
  (if (not (bse-is-song song))
      (bse-exit-error 'text1 (_ "No valid song supplied")))
  (if (bse-project-is-playing (bse-item-get-project song))
      (bse-exit-error 'text1 (_ "This script cannot be used while the project is playing")))
  (bse-item-group-undo song "mixer-splitup-by-track")
  (for-each (lambda (track)
              (if
                (and (bse-is-track track) (track-uses-master-bus? track))
                (assign-track-to-new-bus track)))
            (bse-container-list-children song))
  (bse-item-ungroup-undo song))

;; vim:set sw=2 sts=2 ts=8:
