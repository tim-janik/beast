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

(bse-script-register 'mixer-splitup-by-track
		     ""
                     (N_ "/Song/Assign Tracks to individual Mixer Busses")
		     (N_ "This script will create a new Mixer Bus for each track "
			 "in the song that is currently using the Master Bus as output")
		     "Stefan Westerfeld"
		     "GNU General Public License"
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
  (bse-item-group-undo song "mixer-splitup-by-track")
  (for-each (lambda (track)
              (if
                (and (bse-is-track track) (track-uses-master-bus? track))
                (assign-track-to-new-bus track)))
            (bse-container-list-children song))
  (bse-item-ungroup-undo song))

;; vim:set sw=2 sts=2 ts=8:
